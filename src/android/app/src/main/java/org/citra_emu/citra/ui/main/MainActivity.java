// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package org.citra_emu.citra.ui.main;

import android.Manifest;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;

import org.citra_emu.citra.FileUtil;
import org.citra_emu.citra.PermissionUtil;
import org.citra_emu.citra.R;

public final class MainActivity extends AppCompatActivity {
    // Java enums suck
    public static class PermissionCodes {
        public static final int INIT_USER_PATH = 0;
    }

    private DrawerLayout mDrawerLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionbar = getSupportActionBar();
        actionbar.setDisplayHomeAsUpEnabled(true);
        actionbar.setHomeAsUpIndicator(R.drawable.ic_menu);

        mDrawerLayout = findViewById(R.id.drawer_layout);

        PermissionUtil.verifyPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                        PermissionCodes.INIT_USER_PATH);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                mDrawerLayout.openDrawer(GravityCompat.START);
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
            case PermissionCodes.INIT_USER_PATH:
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    initUserPath(FileUtil.getUserPath(this));
                } else {
                    // just end it all
                    AlertDialog dialog = new AlertDialog.Builder(this).create();
                    dialog.setTitle("Permission Error");
                    dialog.setMessage("Citra requires storage permissions to function.");
                    dialog.setCancelable(false);
                    dialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK",
                                           (dialogInterface, which) -> {
                                               moveTaskToBack(true);
                                               android.os.Process
                                                       .killProcess(android.os.Process.myPid());
                                               System.exit(1);
                                           });
                    dialog.show();
                }
        }
    }

    private static native void initUserPath(String path);
}
