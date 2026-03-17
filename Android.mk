LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := selinux_disable
LOCAL_SRC_FILES := selinux_disable.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CFLAGS := -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all
LOCAL_STRIP_MODULE := true

include $(BUILD_EXECUTABLE)
