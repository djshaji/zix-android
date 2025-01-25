LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := zix
LOCAL_LDFLAGS := -llog
LOCAL_LDLIBS := -llog 
LOCAL_C_INCLUDES :=  ../include/
LOCAL_SRC_FILES := allocator.c btree.c bump_allocator.c digest.c errno_status.c filesystem.c hash.c path.c ring.c status.c string_view.c system.c tree.c
include $(BUILD_STATIC_LIBRARY)

