#include "native.h"

std::string CitraJNI::GetJString(JNIEnv* env, jstring jstr){
    std::string result = "";
    if (!jstr)
        return result;

    const char* s = env->GetStringUTFChars(jstr, nullptr);
    result = s;
    env->ReleaseStringUTFChars(jstr, s);
    return result;
}

extern "C" {
void CitraJNI::Java_org_citra_1emu_citra_ui_main_MainActivity_initUserPath(JNIEnv* env, jclass type,
                                                                           jstring path){
    FileUtil::GetUserPath(FileUtil::UserPath::UserDir, CitraJNI::GetJString(env, path));
}
}