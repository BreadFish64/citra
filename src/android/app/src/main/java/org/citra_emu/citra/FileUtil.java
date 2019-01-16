package org.citra_emu.citra;

import android.app.Activity;
import android.os.Environment;

import java.io.File;

public class FileUtil {
    public static String getUserPath(Activity activity) {
        File storage = Environment.getExternalStorageDirectory();
        File userPath = new File(storage, "citra");
        if (!userPath.isDirectory())
            userPath.mkdir();
        return userPath.toString();
    }
}
