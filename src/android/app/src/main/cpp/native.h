#include <string>
#include <jni.h>

#include "../../../../../common/file_util.h"

namespace CitraJNI {
JavaVM* jvm;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    CitraJNI::jvm = vm;
    return JNI_VERSION_1_6;
}

std::string GetJString(JNIEnv* env, jstring jstr);

extern "C" {
JNICALL void Java_org_citra_1emu_citra_ui_main_MainActivity_initUserPath(JNIEnv* env, jclass type,
                                                                     jstring path);
}
}
