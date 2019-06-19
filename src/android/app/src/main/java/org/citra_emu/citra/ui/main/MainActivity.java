// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package org.citra_emu.citra.ui.main;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.MenuItem;

import com.google.android.material.navigation.NavigationView;

import org.citra_emu.citra.R;
import org.citra_emu.citra.config.SettingsFragment;
import org.citra_emu.citra.utils.FileUtil;
import org.citra_emu.citra.utils.PermissionUtil;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;

public final class MainActivity extends AppCompatActivity {

    // Java enums suck
    private interface PermissionCodes { int INITIALIZE = 0; }

    private DrawerLayout mDrawerLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.main_toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionbar = getSupportActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);
        actionbar.setHomeAsUpIndicator(R.drawable.ic_menu);

        ((NavigationView)findViewById(R.id.nav_view))
            .setNavigationItemSelectedListener(this ::navItemListener);

        mDrawerLayout = findViewById(R.id.main_drawer_layout);

        PermissionUtil.verifyPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                        PermissionCodes.INITIALIZE);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            mDrawerLayout.openDrawer(GravityCompat.START);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private boolean navItemListener(@NonNull MenuItem item) {
        switch (item.getItemId()) {
        case R.id.settings:
            getSupportFragmentManager()
                .beginTransaction()
                .add(R.id.empty, new SettingsFragment())
                .addToBackStack(null)
                .commit();
            mDrawerLayout.closeDrawers();
            break;
        default:
            return false;
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
        case PermissionCodes.INITIALIZE:
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initUserPath(FileUtil.getUserPath().toString());
                initLogging();
            } else {
                AlertDialog.Builder dialog =
                    new AlertDialog.Builder(this)
                        .setTitle("Permission Error")
                        .setMessage("Citra requires storage permissions to function.")
                        .setCancelable(false)
                        .setPositiveButton("OK", (dialogInterface, which) -> {
                            PermissionUtil.verifyPermission(
                                MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                PermissionCodes.INITIALIZE);
                        });
                dialog.show();
            }
        }
    }

    private static native void initUserPath(String path);

    private static native void initLogging();
}
