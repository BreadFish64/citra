/*
 * Copyright 2019 Citra Emulator Project
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

package org.citra_emu.citra.config;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import org.citra_emu.citra.CitraApplication;

public class ConfigNative {
    private final SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(
        CitraApplication.getApplication().getApplicationContext());

    public static native void Reload();

    boolean getBool(String preferenceKey) {
        return preferences.getBoolean(preferenceKey, false);
    }

    int getInt(String preferenceKey) {
        return preferences.getInt(preferenceKey, 0);
    }

    float getFloat(String preferenceKey) {
        return preferences.getFloat(preferenceKey, 0);
    }

    String getString(String preferenceKey) {
        return preferences.getString(preferenceKey, "");
    }
}
