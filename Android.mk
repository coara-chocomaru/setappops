LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := SetAppops
LOCAL_SRC_FILES := SetAppops.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -O2 -std=c99
include $(BUILD_EXECUTABLE)
