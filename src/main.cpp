#include "App.h"
#include "Settings.h"

#include <commctrl.h>
#include <windows.h>

#include <string_view>

using resize_symmetrically::App;
using resize_symmetrically::kCoordinatorClassName;
using resize_symmetrically::kMessageShowSettings;

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const std::wstring_view arguments(commandLine);
    if (arguments.find(L"--uninstall-cleanup") != std::wstring_view::npos) {
        return resize_symmetrically::RemoveAllSettings() ? 0 : 1;
    }
    const bool startupLaunch = arguments.find(L"--startup") != std::wstring_view::npos;
    const bool exitExisting = arguments.find(L"--exit") != std::wstring_view::npos;

    HANDLE mutex = CreateMutexW(nullptr, TRUE,
        L"Local\\ResizeSymmetrically.Singleton.88C0E24B-28EA-457D-8897-D23F95DA5C5A");
    if (!mutex) {
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = nullptr;
        for (int attempt = 0; attempt < 30 && !existing; ++attempt) {
            existing = FindWindowW(kCoordinatorClassName, nullptr);
            if (!existing) Sleep(50);
        }
        if (existing) {
            if (exitExisting) {
                PostMessageW(existing, WM_CLOSE, 0, 0);
            } else if (!startupLaunch) {
                PostMessageW(existing, kMessageShowSettings, 0, 0);
            }
        }
        CloseHandle(mutex);
        return 0;
    }

    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    App app(instance);
    const int result = app.Run(startupLaunch);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return result;
}
