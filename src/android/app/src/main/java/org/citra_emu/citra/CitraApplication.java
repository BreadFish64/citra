// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package org.citra_emu.citra;

import android.app.Application;

public class CitraApplication extends Application {

    private static CitraApplication application;

    public static CitraApplication getApplication() {
        return application;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        System.loadLibrary("citra-android");
        application = test(this);
    }

    public static native CitraApplication test(CitraApplication app);

    public static CitraApplication test(){
        return getApplication();
    }
}
