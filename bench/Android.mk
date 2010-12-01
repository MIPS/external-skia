
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  BlitBench.cpp \
	SkBenchmark.cpp \
	benchmain.cpp
# 	BitmapBench.cpp \
#  FPSBench.cpp \
# 	RepeatTileBench.cpp \
#  TestBench.cpp \
#  DecodeBench.cpp \
#	RectBench.cpp \
#	TextBench.cpp \

# additional optional class for this tool
LOCAL_SRC_FILES += \
    ../src/utils/SkNWayCanvas.cpp \
    ../src/utils/SkParse.cpp

LOCAL_SHARED_LIBRARIES := libcutils libskia
LOCAL_C_INCLUDES := \
    external/skia/include/config \
    external/skia/include/core \
    external/skia/include/images \
    external/skia/include/utils \
    external/skia/include/effects

#LOCAL_CFLAGS := 

LOCAL_MODULE := skia_bench

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
