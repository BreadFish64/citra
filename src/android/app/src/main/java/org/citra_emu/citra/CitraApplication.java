// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package org.citra_emu.citra;

import android.app.Application;
import android.content.Context;

public class CitraApplication extends Application {
    private static CitraApplication citraApplication;

    static {System.loadLibrary("citra-android");}

    @Override
    public void onCreate() {
        super.onCreate();
        citraApplication = this;
    }

    public static Context getAppContext(){ return citraApplication.getApplicationContext();}


}
