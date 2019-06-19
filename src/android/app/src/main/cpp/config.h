// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "native_interface.h"
#include "core/settings.h"

class Config {
    using JConfig = JavaClass<TAG("citra_emu/citra/config/ConfigNative")>;

public:
    static Config config;

    Config();
    ~Config();

    void Reload();
    void Save();

private:
    JConfig jconfig;

    void ReadValues();
    void SaveValues();

    template<typename T>
    T ReadValue();
};

template <>
bool Config::ReadValue(){
    return false;
}

extern "C" {
JNICALL void Java_org_citra_1emu_citra_config_ConfigNative_Reload(JNIEnv* env, jclass type);
}
