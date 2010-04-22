#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>

/* MIPS assembly version of function validation and benchmark */

//#define __QEMU__

#ifdef __QEMU__  /*QEMU runs slow*/

#define SkFloatToFixed_VALIDATE_TIMES    600000
#define SkFixedMul_VALIDATE_TIMES    600000
#define SkFractMul_VALIDATE_TIMES    600000

/* Chage these values as you want */
#define SkFloatToFixed_BENCHMARK_TIMES    5000000
#define SkFixedMul_BENCHMARK_TIMES        5000000
#define SkFractMul_BENCHMARK_TIMES        5000000

#else /* real board */

#define SkFloatToFixed_VALIDATE_TIMES    40000000
#define SkFixedMul_VALIDATE_TIMES    40000000
#define SkFractMul_VALIDATE_TIMES    40000000

/* Chage these values as you want */
#define SkFloatToFixed_BENCHMARK_TIMES    400000000
#define SkFixedMul_BENCHMARK_TIMES        400000000
#define SkFractMul_BENCHMARK_TIMES        400000000
#endif


#define SkASSERT(cond) 



typedef int32_t             SkFract;
typedef int32_t             SkFixed;
#define SK_Fixed1           (1 << 16)
#define SkFloatToFixed(x)   ((SkFixed)((x) * SK_Fixed1))


#define SkExtractSign(n)    ((int32_t)(n) >> 31)
static inline int32_t SkApplySign(int32_t n, int32_t sign) {
     SkASSERT(sign == 0 || sign == -1);
     return (n ^ sign) - sign;
}

SkFixed SkFixedMul(SkFixed a, SkFixed b) {
#if 0
    Sk64    tmp;

    tmp.setMul(a, b);
    tmp.shiftRight(16);
    return tmp.fLo;
#elif defined(SkLONGLONG)
    return static_cast<SkFixed>((SkLONGLONG)a * b >> 16);
#else
    int sa = SkExtractSign(a);
    int sb = SkExtractSign(b);
    // now make them positive
    a = SkApplySign(a, sa);
    b = SkApplySign(b, sb);

    uint32_t    ah = a >> 16;
    uint32_t    al = a & 0xFFFF;
    uint32_t bh = b >> 16;
    uint32_t bl = b & 0xFFFF;

    uint32_t R = ah * b + al * bh + (al * bl >> 16);

    return SkApplySign(R, sa ^ sb);
#endif
}

#define SkFixedMulAdd(x, y, a)  (SkFixedMul(x, y) + (a))

SkFract SkFractMul(SkFract a, SkFract b) {
#if 0
    Sk64 tmp;
    tmp.setMul(a, b);
    return tmp.getFract();
#elif defined(SkLONGLONG)
    return static_cast<SkFract>((SkLONGLONG)a * b >> 30);
#else
    int sa = SkExtractSign(a);
    int sb = SkExtractSign(b);
    // now make them positive
    a = SkApplySign(a, sa);
    b = SkApplySign(b, sb);

    uint32_t ah = a >> 16;
    uint32_t al = a & 0xFFFF;
    uint32_t bh = b >> 16;
    uint32_t bl = b & 0xFFFF;

    uint32_t A = ah * bh;
    uint32_t B = ah * bl + al * bh;
    uint32_t C = al * bl;

    /*  [  A  ]
           [  B  ]
              [  C  ]
    */
    uint32_t Lo = C + (B << 16);
    uint32_t Hi = A + (B >>16) + (Lo < C);

    SkASSERT((Hi >> 29) == 0);  // else overflow

    int32_t R = (Hi << 2) + (Lo >> 30);

    return SkApplySign(R, sa ^ sb);
#endif
}


/*The assembly MIPS version*/
#if (defined (__mips__) && defined(__MIPSEL__))
	/* This guy does not handle NaN or other obscurities, but is faster than
       than (int)(x*65536) when we only have software floats
    */
    inline SkFixed SkFloatToFixed_mips(float x)
    {
        register int32_t t0,t1,t2,t3;
        SkFixed res;
        
        asm("srl    %[t0],%[x],31  \r\n"       /* t0 <- sign bit */
            "srl    %[t1],%[x],23  \r\n"
            "andi   %[t1],%[t1],0xff  \r\n"       /*get the e*/
            "li     %[t2],0x8e \r\n"
            "subu   %[t1],%[t2],%[t1] \r\n"      /*t1=127+15-e*/
            "sll    %[t2],%[x],8 \r\n"            /* mantissa<<8 */
            "lui	%[t3],0x8000 \r\n"
            "or	    %[t2],%[t2],%[t3] \r\n"       /* add the missing 1 */
            "srl    %[res],%[t2],%[t1] \r\n"      /* scale to 16.16 */
            "sltiu  %[t3],%[t1],32 \r\n"          /* t3=1 if t1<32, else t3=0. t1>=32 means the float value is too small.*/
            "subu   %[t2],$zero,%[res] \r\n"
            "movn   %[res],%[t2],%[t0] \r\n"      /*if negate?*/
            "or     %[t1],%[x],$zero \r\n"         /*a0=0?*/
            "movz   %[res],$zero,%[t1] \r\n"
			"movz   %[res],$zero,%[t3] \r\n"       /*t3=0 then res=0*/
            : [res]"=&r"(res),[t0]"=&r"(t0),[t1]"=&r"(t1),[t2]"=&r"(t2),[t3]"=&r"(t3)
            : [x] "r" (x)
            );
            return res;
    }
    inline SkFixed SkFixedMul_mips(SkFixed x, SkFixed y)
    {
        SkFixed res;
    	int32_t t0;
    	asm("mult   %[x],%[y] \r\n"
        	"mflo   %[res] \r\n"
        	"mfhi   %[t0] \r\n"
        	"srl    %[res],%[res],16 \r\n"
        	"sll    %[t0],%[t0],16 \r\n"
        	"or     %[res],%[res],%[t0] \r\n"
        	: [res]"=&r"(res),[t0]"=&r"(t0)
        	: [x] "r" (x), [y] "r" (y)
        	: "%hi","%lo"
        	);
        	return res;
    }
    inline SkFixed SkFixedMulAdd_mips(SkFixed x, SkFixed y, SkFixed a)
    {
        SkFixed res;
    	int32_t t0;
    	asm("mult   %[x],%[y] \r\n"
        	"mflo   %[res] \r\n"
        	"mfhi   %[t0] \r\n"
        	"srl    %[res],%[res],16 \r\n"
        	"sll    %[t0],%[t0],16 \r\n"
        	"or     %[res],%[res],%[t0] \r\n"
        	"add    %[res],%[res],%[a] \r\n"
        	: [res]"=&r"(res),[t0]"=&r"(t0)
        	: [x] "r" (x), [y] "r" (y), [a] "r" (a)
        	: "%hi","%lo"
        	);
        	return res;
    }
    inline SkFixed SkFractMul_mips(SkFixed x, SkFixed y)
    {
        SkFixed res;
    	int32_t t0;
    	asm("mult   %[x],%[y] \r\n"
        	"mfhi   %[t0] \r\n"
        	"mflo   %[res] \r\n"
        	"srl    %[res],%[res],30 \r\n"
        	"sll    %[t0],%[t0],2 \r\n"
        	"or     %[res],%[res],%[t0] \r\n"
        	: [res]"=&r"(res),[t0]"=&r"(t0)
        	: [x] "r" (x), [y] "r" (y)
        	: "%hi","%lo"
        	);
        	return res;
    }
#endif

#define VALIDATION_START do { \
							printf("\n[validation start] %s\n",__FUNCTION__); \
							} while (0)
#define VALIDATION_END do { \
							printf("[validation pass] %s\n",__FUNCTION__); \
							} while (0)

#define TEST_ASSERT_SkFloatToFixed(resc,resasm,a0) \
do { \
if (resc != resasm) \
	{\
	if ((abs(resc-resasm)!=1) ) { \
	    printf("resc %x  resasm %x a0 %f \n",resc,resasm,a0);\
		printf("FAILED \n"); \
		exit(-1);\
	} \
    }\
}\
while (0)

#define TEST_SkFloatToFixed(a0) \
do { \
	resc = SkFloatToFixed(a0); \
	resasm = SkFloatToFixed_mips(a0);\
	TEST_ASSERT_SkFloatToFixed(resc,resasm,a0);\
}\
while (0)

void validate_SkFloatToFixed()
{
	SkFixed resc,resasm;
    float a;
    int i;
    
    /*the mini value of a is 0x37800000 (0.000015) */
    
    VALIDATION_START;
    
    a = 32768.000000;
    TEST_SkFloatToFixed(a);
    
    a = 211.277786;
    TEST_SkFloatToFixed(a);

    a = 0.0;
    TEST_SkFloatToFixed(a);

    a = 32767.0;
    TEST_SkFloatToFixed(a);

    a = -32768.0;
    TEST_SkFloatToFixed(a);

    a = -32767.0;
    TEST_SkFloatToFixed(a);
    
    for (i=0;i<SkFloatToFixed_VALIDATE_TIMES;i++)
	{
	    srand(i);
		
	    a=(((32767+1.0))*rand()/(RAND_MAX+1.0));
        TEST_SkFloatToFixed(a);

        a=0-(((32767+1.0))*rand()/(RAND_MAX+1.0));
        TEST_SkFloatToFixed(a);

        a=(((100+1.0))*rand()/(RAND_MAX+1.0));
        TEST_SkFloatToFixed(a);
        
        srand((unsigned) time(NULL));
        
        a=(((32767+1.0))*rand()/(RAND_MAX+1.0));
        TEST_SkFloatToFixed(a);

        a=0-(((32767+1.0))*rand()/(RAND_MAX+1.0));
        TEST_SkFloatToFixed(a);

        a=(((100+1.0))*rand()/(RAND_MAX+1.0));
        TEST_SkFloatToFixed(a);

	}
	
	VALIDATION_END;
}

#define TEST_ASSERT_SkFixedMul(resc,resasm,a0,a1) \
do { \
if (resc != resasm) \
	{\
	if ((resc-resasm)!=1) { \
		printf("resc %x  resasm %x a0 %x %d a1 %x %d\n",resc,resasm,a0,a0,a1,a1);\
		printf("FAILED \n"); \
		exit(-1);\
	} \
    }\
}\
while (0)

#define TEST_SkFixedMul(a0,a1) \
do { \
	resc = SkFixedMul(a0,a1); \
	resasm = SkFixedMul_mips(a0,a1);\
	TEST_ASSERT_SkFixedMul(resc,resasm,a0,a1);\
}\
while (0)

void validate_SkFixedMul()
{

    SkFixed resc,resasm;
    SkFixed a,b;
    int i;

	VALIDATION_START;
	    
	srand(time(NULL));
	for (i=0;i<SkFixedMul_VALIDATE_TIMES;i++)
	{
		a = rand();
        b = rand();
        TEST_SkFixedMul(a,b);

        a = 0- rand();
        b = rand();
        TEST_SkFixedMul(a,b);

        a = rand();
        b = 0 - rand();
        TEST_SkFixedMul(a,b);
        
        a = 0 - rand();
        b = 0 - rand();
        TEST_SkFixedMul(a,b);
	}
	
	VALIDATION_END;

}

#define TEST_ASSERT_SkFractMul(resc,resasm,a0,a1) \
do { \
if (resc != resasm) \
	{\
	if ((resc-resasm)!=1) { \
		printf("resc %x  resasm %x a0 %x %d a1 %x %d\n",resc,resasm,a0,a0,a1,a1);\
		printf("FAILED \n"); \
		exit(-1);\
	} \
    }\
}\
while (0)

#define TEST_SkFractMul(a0,a1) \
do { \
	resc = SkFractMul(a0,a1); \
	resasm = SkFractMul_mips(a0,a1);\
	TEST_ASSERT_SkFractMul(resc,resasm,a0,a1);\
}\
while (0)

void validate_SkFractMul()
{

    SkFixed resc,resasm;
    SkFixed a,b;
    int i;
    
    for (i=0;i<SkFixedMul_VALIDATE_TIMES;i++)
	{
		a = rand();
        b = rand();
        TEST_SkFractMul(a,b);

        a = 0- rand();
        b = rand();
        TEST_SkFractMul(a,b);

        a = rand();
        b = 0 - rand();
        TEST_SkFractMul(a,b);
        
        a = 0 - rand();
        b = 0 - rand();
        TEST_SkFractMul(a,b);
	}

    VALIDATION_END;

}

#define BENCH_LOGO do { \
						printf("\n[benchmark] %s start \n",__FUNCTION__); \
					  } while(0);
					  
#define BENCH_ASM do { \
						printf("[ASM version] time used: %d seconds. START:%d END:%d DUMMY:%d \n",time_e-time_s,time_e,time_s,resc); \
					 } while(0);
					 
#define BENCH_C do { \
						printf("[C version:] time used: %d seconds. START:%d END:%d DUMMY:%d \n",time_e-time_s,time_e,time_s,resc); \
				  } while(0);
				  
#define BENCH_END do { \
						printf("[benchmark] %s end \n",__FUNCTION__); \
					} while(0);
					 
void bench_SkFloatToFixed()
{
	SkFixed resc = 0;
    int i;
    uint32_t time_s,time_e;
    float x;

    srand(0);
    x=(((32767+1.0))*rand()/(RAND_MAX+1.0));
    
    BENCH_LOGO;
    
    
    time_s = time(NULL);
	for (i=0;i<SkFloatToFixed_BENCHMARK_TIMES;i++)
	{
		x++;   
        resc += SkFloatToFixed_mips(x);
	}
	time_e = time(NULL);
	BENCH_ASM;
	
	
	time_s = time(NULL);
	for (i=0;i<SkFloatToFixed_BENCHMARK_TIMES;i++)
	{
		x++;
        resc += SkFloatToFixed(x);
	}
	time_e = time(NULL);
    BENCH_C;
    
	BENCH_END;	
}

void bench_SkFixedMul()
{
	SkFixed a,b;
	SkFixed resc = 0;
	uint32_t time_s,time_e;
	int i;
	
	srand(0);
	a=(((32767+1.0))*rand()/(RAND_MAX+1.0));
    b=(((32767+1.0))*rand()/(RAND_MAX+1.0));
        
	BENCH_LOGO;
	
	time_s = time(NULL);
	for (i=0;i<SkFixedMul_BENCHMARK_TIMES;i++)
	{
		a++;
		b++;
        resc += SkFixedMul_mips(a,b);
	}
	time_e = time(NULL);
	BENCH_ASM;
	
	
	time_s = time(NULL);
	for (i=0;i<SkFixedMul_BENCHMARK_TIMES;i++)
	{
		a++;
		b++;
        resc += SkFixedMul(a,b);
	}
	time_e = time(NULL);
    BENCH_C;
	
	BENCH_END;
}


void bench_SkFractMul()
{
	SkFixed a,b;
	SkFixed resc = 0;
	uint32_t time_s,time_e;
	int i;
	
	srand(0);
    a=(((32767+1.0))*rand()/(RAND_MAX+1.0));
    b=(((32767+1.0))*rand()/(RAND_MAX+1.0));
        
	BENCH_LOGO;
	
	time_s = time(NULL);
	for (i=0;i<SkFractMul_BENCHMARK_TIMES;i++)
	{
		a++;
		b++;
        resc += SkFractMul_mips(a,b);
	}
	time_e = time(NULL);	
	BENCH_ASM;
	
	time_s = time(NULL);
	for (i=0;i<SkFractMul_BENCHMARK_TIMES;i++)
	{
		a++;
		b++;
        resc += SkFractMul(a,b);
	}
	time_e = time(NULL);
    BENCH_C;
	
	BENCH_END;
}

int main()
{
	/* validation */
	validate_SkFloatToFixed();
	validate_SkFixedMul();
	validate_SkFractMul();
		
	/* benchmark */
	bench_SkFloatToFixed();
	bench_SkFixedMul();
	bench_SkFractMul();
	
	return 1;
}
