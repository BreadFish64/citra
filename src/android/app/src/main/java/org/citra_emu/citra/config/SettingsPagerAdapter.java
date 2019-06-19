/*
 * Copyright 2019 Citra Emulator Project
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

package org.citra_emu.citra.config;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;

public class SettingsPagerAdapter extends FragmentPagerAdapter {
    private Fragment[] pages = {new ConfigGraphics()};

    public SettingsPagerAdapter(FragmentManager fm) {
        super(fm);
    }

    @Override
    public int getCount() {
        return pages.length;
    }

    @Override
    public Fragment getItem(int position) {
        return pages[position];
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return getItem(position).toString();
    }
}
