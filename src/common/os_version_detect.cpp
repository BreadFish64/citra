#include <windows.h>
#include "os_version_detect.h"
float Common::GetWinVersion() {
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    return GetVersionEx(&osvi) ? (float)osvi.dwMajorVersion + ((float)osvi.dwMinorVersion / 10)
                               : 0.0;
}
std::string Common::WinVersiontoStr() {
    float WinVersion(GetWinVersion());
    if (WinVersion == 10.0f) {
        return "Windows 10";
    } else if (WinVersion == 6.3f) {
        return "Windows 8.1";
    } else if (WinVersion == 6.2f) {
        return "Windows 8";
    } else if (WinVersion == 6.1f) {
        return "Windows 7";
    } else if (WinVersion == 6.0f) {
        return "Windows Vista";
    } else if (WinVersion == 5.1f) {
        return "Windows XP";
    } else {
        return "Windows";
    }
}
