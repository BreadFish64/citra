// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <numeric>
#include <string>
#include <vector>
#include <core/frontend/applets/default_applets.h>
#include <jni.h>
#include "common/logging/log.h"
#include "fmt/format.h"

template <typename T>
struct JavaClass;

namespace CitraJNI {

extern "C" {
jint JNI_OnLoad(JavaVM* vm, void* reserved);
}

JNIEnv* GetEnv();

std::string GetJString(JNIEnv* env, jstring jstr);

template <typename T>
constexpr char TypeName[] = "";

template <typename T>
constexpr char TypeName<JavaClass<T>>[] = T::value;

template <>
constexpr char TypeName<void>[] = "V";

template <>
constexpr char TypeName<jboolean>[] = "Z";

template <>
constexpr char TypeName<bool>[] = "Z";

template <>
constexpr char TypeName<jbyte>[] = "B";

template <>
constexpr char TypeName<jchar>[] = "C";

template <>
constexpr char TypeName<jshort>[] = "S";

template <>
constexpr char TypeName<jint>[] = "I";

template <>
constexpr char TypeName<jlong>[] = "J";

template <>
constexpr char TypeName<jfloat>[] = "F";

template <>
constexpr char TypeName<jdouble>[] = "D";

template <typename T>
std::function<void()> StaticCaller;

template <>
constexpr auto StaticCaller<void> = &JNIEnv::CallStaticVoidMethod;

template <>
constexpr auto StaticCaller<jboolean> = &JNIEnv::CallStaticBooleanMethod;

template <>
constexpr auto StaticCaller<bool> = &JNIEnv::CallStaticBooleanMethod;

template <>
constexpr auto StaticCaller<jbyte> = &JNIEnv::CallStaticByteMethod;

template <>
constexpr auto StaticCaller<jchar> = &JNIEnv::CallStaticCharMethod;

template <>
constexpr auto StaticCaller<jshort> = &JNIEnv::CallStaticShortMethod;

template <>
constexpr auto StaticCaller<jint> = &JNIEnv::CallStaticIntMethod;

template <>
constexpr auto StaticCaller<jlong> = &JNIEnv::CallStaticLongMethod;

template <>
constexpr auto StaticCaller<jfloat> = &JNIEnv::CallStaticFloatMethod;

template <>
constexpr auto StaticCaller<jdouble> = &JNIEnv::CallStaticDoubleMethod;

template <typename T>
std::function<void()> MemberCaller;

template <>
constexpr auto MemberCaller<void> = &JNIEnv::CallVoidMethod;

template <>
constexpr auto MemberCaller<jboolean> = &JNIEnv::CallBooleanMethod;

template <>
constexpr auto MemberCaller<bool> = &JNIEnv::CallBooleanMethod;

template <>
constexpr auto MemberCaller<jbyte> = &JNIEnv::CallByteMethod;

template <>
constexpr auto MemberCaller<jchar> = &JNIEnv::CallCharMethod;

template <>
constexpr auto MemberCaller<jshort> = &JNIEnv::CallShortMethod;

template <>
constexpr auto MemberCaller<jint> = &JNIEnv::CallIntMethod;

template <>
constexpr auto MemberCaller<jlong> = &JNIEnv::CallLongMethod;

template <>
constexpr auto MemberCaller<jfloat> = &JNIEnv::CallFloatMethod;

template <>
constexpr auto MemberCaller<jdouble> = &JNIEnv::CallDoubleMethod;

template <typename Return, typename... Args>
std::string GetSignature() {
    const std::string_view ret_type = TypeName<Return>;
    const std::vector<std::string_view> arg_types = {TypeName<Args>...};
    std::string sig = "(";

    auto add_type = [&sig](const std::string_view& type) {
        // check if type is fully qualified
        if (type.size() != 1)
            sig += 'L';
        sig += type.data();
        if (type.size() != 1)
            sig += ';';
    };

    for (auto type : arg_types)
        add_type(type);

    sig += ')';

    add_type(ret_type);
    return sig;
}

} // namespace CitraJNI

template <char... xs>
struct Tag {
    static constexpr auto Name() {return {xs..., 0};};
};

template <typename C, C... xs>
auto operator""_tag() {
    return Tag<xs...>{};
}

#define TAG(x) decltype( x ## _tag )

template <typename Name>
class JavaClass {
public:
    template <typename... Args>
    JavaClass(Args&&... args){
        jmethodID constructor = GetMethodID<void, TAG("<init>"), Args...>();
        obj = CitraJNI::GetEnv()->NewObject(GetClass(), constructor, std::forward(args)...);
        obj = CitraJNI::GetEnv()->NewGlobalRef(obj);
        _jclass
    }

    ~JavaClass(){
        CitraJNI::GetEnv()->DeleteGlobalRef(obj);
    }

    static jclass GetClass() {
        static jclass clazz = CitraJNI::GetEnv()->FindClass(Name::value);
        return clazz;
    }

    template <typename Return, typename method, typename... Args>
    static Return StaticMethod(Args&&... args) {
        static jmethodID id = CitraJNI::GetEnv()->GetStaticMethodID(
            GetClass(), method::value, CitraJNI::GetSignature<Return, Args...>().data());
        return (CitraJNI::GetEnv()->*CitraJNI::StaticCaller<Return>)(GetClass(), id, std::forward(args)...);
    };

    template <typename Return, typename method, typename... Args>
    Return Method(Args&&... args) {
        jmethodID id = GetMethodID<Return, method, Args...>();
        return (CitraJNI::GetEnv()->*CitraJNI::MemberCaller<Return>)(obj, id, std::forward(args)...);
    };

    inline operator jobject(){ return obj; }

private:
    jobject obj;

    template <typename Return, typename method, typename... Args>
    static jmethodID GetMethodID(){
        static jmethodID id = CitraJNI::GetEnv()->GetMethodID(
                GetClass(), method::value, CitraJNI::GetSignature<Return, Args...>().data());
        return id;
    }
};
