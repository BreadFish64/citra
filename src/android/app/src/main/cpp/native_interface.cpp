// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "native_interface.h"

namespace CitraJNI {
JavaVM* jvm = nullptr;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEnv* GetEnv() {
    thread_local auto env = [] {
        JNIEnv* env;
        CitraJNI::jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        return env;
    }();
    return env;
}

std::string GetJString(JNIEnv* env, jstring jstr) {
    std::string result = "";
    if (!jstr)
        return result;

    const char* s = env->GetStringUTFChars(jstr, nullptr);
    result = s;
    env->ReleaseStringUTFChars(jstr, s);
    return result;
}
} // namespace CitraJNI

#include <chrono>

using CitraApplication = JavaClass<TAG("org/citra_emu/citra/CitraApplication")>;

extern "C" {
JNICALL jobject Java_org_citra_1emu_citra_CitraApplication_test(JNIEnv* env, jclass clazz,
                                                                jobject app) {
    auto test = CitraApplication::StaticMethod<TAG("test"), CitraApplication>();
}
}
