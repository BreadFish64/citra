/*
 * Copyright 2019 Citra Emulator Project
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

package org.citra_emu.citra.config;

import android.os.Bundle;

import org.citra_emu.citra.R;

import androidx.preference.EditTextPreference;

public class ConfigGraphics extends Config {
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.config_graphics);
        generateListValues("layout_option");
        generateListValues("resolution_factor");
        EditTextPreference frameLimit = (EditTextPreference) findPreference("frame_limit");

    }

    @Override
    public String toString() {
        return "Graphics";
    }
}
