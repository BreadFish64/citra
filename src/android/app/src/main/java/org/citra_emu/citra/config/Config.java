/*
 * Copyright 2019 Citra Emulator Project
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

package org.citra_emu.citra.config;

import android.os.Bundle;
import android.text.InputFilter;
import android.view.View;
import android.widget.EditText;

import java.util.ArrayList;

import androidx.preference.EditTextPreferenceDialogFragmentCompat;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceFragmentCompat;

public abstract class Config extends PreferenceFragmentCompat {
    protected void generateListValues (String key){
        ListPreference preference = (ListPreference)findPreference(key);
        String[] values = new String[preference.getEntries().length];
        for (int i = 0; i < values.length; ++i)
            values[i] = String.valueOf(i);
        preference.setEntryValues(values);
    }

    public static class CustomEditTextPreferenceDialog extends EditTextPreferenceDialogFragmentCompat{
        private ArrayList<InputFilter> filters = new ArrayList<>();

        public ArrayList<InputFilter> getFilters() {
            return filters;
        }

        public static CustomEditTextPreferenceDialog newInstance(String key) {
            final CustomEditTextPreferenceDialog
                    fragment = new CustomEditTextPreferenceDialog();
            final Bundle b = new Bundle(1);
            b.putString(ARG_KEY, key);
            fragment.setArguments(b);
            return fragment;
        }

        // getEditText is deprecated in API level 28
        @Override
        protected void onBindDialogView(View view) {
            super.onBindDialogView(view);
            ((EditText)view.findViewById(android.R.id.edit)).setFilters((InputFilter[]) filters.toArray());
        }

    }
}
