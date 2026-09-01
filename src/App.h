#pragma once

#include "ResizeEngine.h"
#include "Settings.h"

#include <shellapi.h>
#include <windows.h>

namespace resize_symmetrically {

constexpr UINT kMessageShowSettings = WM_APP + 41;
constexpr wchar_t kCoordinatorClassName[] = L"ResizeSymmetrically.Coordinator";

class App {
public:
    explicit App(HINSTANCE instance);
    ~App();

    int Run(bool startupLaunch);

private:
    static LRESULT CALLBACK CoordinatorProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SettingsProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    bool RegisterClasses();
    bool CreateCoordinator();
    bool AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu(POINT point);
    void ShowSettings();
    void CreateSettingsControls();
    void LayoutSettingsControls(UINT dpi);
    void ApplyLanguage();
    void ApplyControlFont(HWND control) const;
    void HandleSettingsCommand(WORD id, WORD notification);
    void ShowSaveError();
    LRESULT OnCoordinatorMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT OnSettingsMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE instance_ = nullptr;
    HWND coordinator_ = nullptr;
    HWND settingsWindow_ = nullptr;
    HWND modifierLabel_ = nullptr;
    HWND modifierCombo_ = nullptr;
    HWND autostartCheck_ = nullptr;
    HWND languageLabel_ = nullptr;
    HWND languageCombo_ = nullptr;
    HWND helpLabel_ = nullptr;
    HWND closeButton_ = nullptr;
    HFONT uiFont_ = nullptr;
    HICON icon_ = nullptr;
    NOTIFYICONDATAW trayIcon_{};
    UINT taskbarCreatedMessage_ = 0;
    AppSettings settings_{};
    ResizeEngine resizeEngine_{};
};

}  // namespace resize_symmetrically
