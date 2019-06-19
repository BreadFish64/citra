/*
 * Copyright 2019 Citra Emulator Project
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

package org.citra_emu.citra.config;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.google.android.material.tabs.TabLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.viewpager.widget.ViewPager;

public class SettingsFragment extends Fragment {
    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        ViewPager pager = new ViewPager(getContext());
        pager.setId(View.generateViewId());
        pager.setAdapter(new SettingsPagerAdapter(getChildFragmentManager()));
        TabLayout tabLayout = new TabLayout(getContext());
        tabLayout.setupWithViewPager(pager);
        LinearLayout layout = new LinearLayout(getContext());
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.addView(tabLayout);
        layout.addView(pager);
        return layout;
    }
}
