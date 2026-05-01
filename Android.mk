LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := authencesn_exact
LOCAL_SRC_FILES := authencesn_exact.cpp
LOCAL_CFLAGS := -Wall -Wextra -std=c++11 -O2
LOCAL_LDFLAGS := -static -lz
include $(BUILD_EXECUTABLE)
