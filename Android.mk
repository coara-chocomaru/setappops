LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := mount_helper.cpp
LOCAL_MODULE := mount_helper
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -O3 -Wall -Werror
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
include $(BUILD_EXECUTABLE)
