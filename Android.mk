LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := enable_apk_install
LOCAL_SRC_FILES := test.c
LOCAL_CFLAGS := -Wall -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_64_BIT_ONLY := true
include $(BUILD_EXECUTABLE)
