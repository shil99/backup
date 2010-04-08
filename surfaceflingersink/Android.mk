LOCAL_PATH:= $(call my-dir)

# build surfaceflinger test application
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	surfaceflinger_wrap.cpp \
	wrap_test.cpp  
	
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
	libui

LOCAL_C_INCLUDES := \
	frameworks/base/include \
	$(JNI_H_INCLUDE) \
	$(call include-path-for, graphics corecg libmedia) \
    $(LOCAL_PATH) 

LOCAL_MODULE:= surfacetest

include $(BUILD_EXECUTABLE)


