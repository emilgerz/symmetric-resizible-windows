#include "App.h"

#include "Localization.h"
#include "resource.h"

#include <commctrl.h>
#include <windowsx.h>

#include <array>
#include <string>

namespace resize_symmetrically {
namespace {

constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr UINT kTraySettingsCommand = 40001;
constexpr UINT kTrayExitCommand = 40002;

constexpr int kModifierLabelId = 2001;
constexpr int kModifierComboId = 2002;
constexpr int kAutostartCheckId = 2003;
constexpr int kLanguageLabelId = 2004;
constexpr int kLanguageComboId = 2005;
constexpr int kHelpLabelId = 2006;
constexpr int kCloseButtonId = 2007;

constexpr wchar_t kSettingsClassName[] = L"ResizeSymmetrically.Settings";

int Scale(int value, UINT dpi) {
    return MulDiv(value, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
}

void SetText(HWND window, std::wstring_view text) {
    SetWindowTextW(window, std::wstring(text).c_str());
}

}  // namespace

App::App(HINSTANCE instance) : instance_(instance), settings_(LoadSettings()) {
    icon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        0, 0, LR_DEFAULTSIZE | LR_SHARED));
}

App::~App() {
    resizeEngine_.Stop();
    RemoveTrayIcon();
    if (uiFont_) {
        DeleteObject(uiFont_);
    }
}

int App::Run(bool startupLaunch) {
    if (!RegisterClasses() || !CreateCoordinator()) {
        return 1;
    }
    if (!resizeEngine_.Start(coordinator_, settings_.modifier)) {
        MessageBoxW(nullptr,
            IsRussianLanguage(settings_.language)
                ? L"Не удалось установить системные перехватчики ввода."
                : L"Could not install the system input hooks.",
            Text(TextId::AppTitle, settings_.language).data(), MB_OK | MB_ICONERROR);
        return 2;
    }
    if (!AddTrayIcon()) {
        return 3;
    }
    if (!startupLaunch) {
        ShowSettings();
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!settingsWindow_ || !IsDialogMessageW(settingsWindow_, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return static_cast<int>(message.wParam);
}

bool App::RegisterClasses() {
    WNDCLASSEXW coordinatorClass{sizeof(coordinatorClass)};
    coordinatorClass.lpfnWndProc = CoordinatorProc;
    coordinatorClass.hInstance = instance_;
    coordinatorClass.hIcon = icon_;
    coordinatorClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    coordinatorClass.lpszClassName = kCoordinatorClassName;
    if (!RegisterClassExW(&coordinatorClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    WNDCLASSEXW settingsClass{sizeof(settingsClass)};
    settingsClass.style = CS_HREDRAW | CS_VREDRAW;
    settingsClass.lpfnWndProc = SettingsProc;
    settingsClass.hInstance = instance_;
    settingsClass.hIcon = icon_;
    settingsClass.hIconSm = icon_;
    settingsClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    settingsClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    settingsClass.lpszClassName = kSettingsClassName;
    return RegisterClassExW(&settingsClass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool App::CreateCoordinator() {
    coordinator_ = CreateWindowExW(WS_EX_TOOLWINDOW, kCoordinatorClassName, L"",
        WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, instance_, this);
    if (!coordinator_) {
        return false;
    }
    taskbarCreatedMessage_ = RegisterWindowMessageW(L"TaskbarCreated");
    return true;
}

bool App::AddTrayIcon() {
    trayIcon_ = {};
    trayIcon_.cbSize = sizeof(trayIcon_);
    trayIcon_.hWnd = coordinator_;
    trayIcon_.uID = kTrayIconId;
    trayIcon_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    trayIcon_.uCallbackMessage = kTrayCallbackMessage;
    trayIcon_.hIcon = icon_;
    wcscpy_s(trayIcon_.szTip, Text(TextId::AppTitle, settings_.language).data());
    if (!Shell_NotifyIconW(NIM_ADD, &trayIcon_)) {
        return false;
    }
    trayIcon_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &trayIcon_);
    return true;
}

void App::RemoveTrayIcon() {
    if (trayIcon_.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &trayIcon_);
        trayIcon_.hWnd = nullptr;
    }
}

void App::ShowTrayMenu(POINT point) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    AppendMenuW(menu, MF_STRING | MF_DEFAULT, kTraySettingsCommand,
        Text(TextId::Settings, settings_.language).data());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kTrayExitCommand, Text(TextId::Exit, settings_.language).data());
    SetForegroundWindow(coordinator_);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        point.x, point.y, 0, coordinator_, nullptr);
    DestroyMenu(menu);
    if (command == kTraySettingsCommand) {
        ShowSettings();
    } else if (command == kTrayExitCommand) {
        DestroyWindow(coordinator_);
    }
}

void App::ShowSettings() {
    if (!settingsWindow_) {
        const UINT dpi = GetDpiForSystem();
        RECT bounds{0, 0, Scale(460, dpi), Scale(270, dpi)};
        AdjustWindowRectExForDpi(&bounds, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            FALSE, 0, dpi);
        settingsWindow_ = CreateWindowExW(WS_EX_APPWINDOW, kSettingsClassName,
            Text(TextId::AppTitle, settings_.language).data(),
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
            coordinator_, nullptr, instance_, this);
        if (!settingsWindow_) {
            return;
        }
    }
    ShowWindow(settingsWindow_, SW_SHOWNORMAL);
    SetForegroundWindow(settingsWindow_);
}

void App::CreateSettingsControls() {
    modifierLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kModifierLabelId)), instance_, nullptr);
    modifierCombo_ = CreateWindowExW(0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kModifierComboId)), instance_, nullptr);
    autostartCheck_ = CreateWindowExW(0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAutostartCheckId)), instance_, nullptr);
    languageLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLanguageLabelId)), instance_, nullptr);
    languageCombo_ = CreateWindowExW(0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLanguageComboId)), instance_, nullptr);
    helpLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kHelpLabelId)), instance_, nullptr);
    closeButton_ = CreateWindowExW(0, L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        0, 0, 0, 0, settingsWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCloseButtonId)), instance_, nullptr);

    std::array controls{modifierLabel_, modifierCombo_, autostartCheck_, languageLabel_,
        languageCombo_, helpLabel_, closeButton_};
    for (HWND control : controls) {
        ApplyControlFont(control);
    }
    Button_SetCheck(autostartCheck_, IsAutostartEnabled() ? BST_CHECKED : BST_UNCHECKED);
    ApplyLanguage();
    LayoutSettingsControls(GetDpiForWindow(settingsWindow_));
}

void App::LayoutSettingsControls(UINT dpi) {
    if (!settingsWindow_) {
        return;
    }
    const int margin = Scale(24, dpi);
    const int labelWidth = Scale(170, dpi);
    const int comboWidth = Scale(210, dpi);
    const int rowHeight = Scale(28, dpi);
    const int comboHeight = Scale(200, dpi);
    int y = Scale(28, dpi);
    MoveWindow(modifierLabel_, margin, y + Scale(5, dpi), labelWidth, rowHeight, TRUE);
    MoveWindow(modifierCombo_, margin + labelWidth, y, comboWidth, comboHeight, TRUE);
    y += Scale(48, dpi);
    MoveWindow(autostartCheck_, margin, y, Scale(380, dpi), rowHeight, TRUE);
    y += Scale(46, dpi);
    MoveWindow(languageLabel_, margin, y + Scale(5, dpi), labelWidth, rowHeight, TRUE);
    MoveWindow(languageCombo_, margin + labelWidth, y, comboWidth, comboHeight, TRUE);
    y += Scale(50, dpi);
    MoveWindow(helpLabel_, margin, y, Scale(390, dpi), Scale(42, dpi), TRUE);
    MoveWindow(closeButton_, Scale(330, dpi), Scale(222, dpi), Scale(105, dpi), Scale(32, dpi), TRUE);
}

void App::ApplyLanguage() {
    if (!settingsWindow_) {
        return;
    }
    SetText(settingsWindow_, Text(TextId::AppTitle, settings_.language));
    SetText(modifierLabel_, Text(TextId::Modifier, settings_.language));
    SetText(autostartCheck_, Text(TextId::Autostart, settings_.language));
    SetText(languageLabel_, Text(TextId::Language, settings_.language));
    SetText(helpLabel_, Text(TextId::GestureHelp, settings_.language));
    SetText(closeButton_, Text(TextId::Close, settings_.language));

    SendMessageW(modifierCombo_, CB_RESETCONTENT, 0, 0);
    for (TextId id : {TextId::Alt, TextId::Ctrl, TextId::Shift, TextId::Win}) {
        SendMessageW(modifierCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Text(id, settings_.language).data()));
    }
    SendMessageW(modifierCombo_, CB_SETCURSEL, static_cast<WPARAM>(settings_.modifier), 0);

    SendMessageW(languageCombo_, CB_RESETCONTENT, 0, 0);
    for (TextId id : {TextId::AutoLanguage, TextId::Russian, TextId::English}) {
        SendMessageW(languageCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Text(id, settings_.language).data()));
    }
    SendMessageW(languageCombo_, CB_SETCURSEL, static_cast<WPARAM>(settings_.language), 0);
}

void App::ApplyControlFont(HWND control) const {
    if (control && uiFont_) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont_), TRUE);
    }
}

void App::HandleSettingsCommand(WORD id, WORD notification) {
    if (id == kModifierComboId && notification == CBN_SELCHANGE) {
        const LRESULT selection = SendMessageW(modifierCombo_, CB_GETCURSEL, 0, 0);
        if (selection >= 0 && selection <= static_cast<LRESULT>(ModifierKey::Win)) {
            settings_.modifier = static_cast<ModifierKey>(selection);
            if (!SaveSettings(settings_)) ShowSaveError();
            resizeEngine_.SetModifier(settings_.modifier);
        }
    } else if (id == kLanguageComboId && notification == CBN_SELCHANGE) {
        const LRESULT selection = SendMessageW(languageCombo_, CB_GETCURSEL, 0, 0);
        if (selection >= 0 && selection <= static_cast<LRESULT>(Language::English) &&
            static_cast<Language>(selection) != settings_.language) {
            settings_.language = static_cast<Language>(selection);
            if (!SaveSettings(settings_)) ShowSaveError();
            ApplyLanguage();
        }
    } else if (id == kAutostartCheckId && notification == BN_CLICKED) {
        const bool enabled = Button_GetCheck(autostartCheck_) == BST_CHECKED;
        if (!SetAutostartEnabled(enabled)) {
            Button_SetCheck(autostartCheck_, enabled ? BST_UNCHECKED : BST_CHECKED);
            ShowSaveError();
        }
    } else if (id == kCloseButtonId && notification == BN_CLICKED) {
        ShowWindow(settingsWindow_, SW_HIDE);
    }
}

void App::ShowSaveError() {
    MessageBoxW(settingsWindow_, Text(TextId::SaveError, settings_.language).data(),
        Text(TextId::AppTitle, settings_.language).data(), MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK App::CoordinatorProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->OnCoordinatorMessage(window, message, wParam, lParam)
               : DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK App::SettingsProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        app->settingsWindow_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->OnSettingsMessage(window, message, wParam, lParam)
               : DefWindowProcW(window, message, wParam, lParam);
}

LRESULT App::OnCoordinatorMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == taskbarCreatedMessage_ && taskbarCreatedMessage_ != 0) {
        AddTrayIcon();
        return 0;
    }
    switch (message) {
    case kMessageApplyResize:
        resizeEngine_.ApplyPendingResize();
        return 0;
    case kMessageShowSettings:
        ShowSettings();
        return 0;
    case kTrayCallbackMessage: {
        const UINT event = LOWORD(lParam);
        if (event == WM_LBUTTONDBLCLK || event == NIN_SELECT || event == NIN_KEYSELECT) {
            ShowSettings();
        } else if (event == WM_CONTEXTMENU || event == WM_RBUTTONUP) {
            POINT point{};
            GetCursorPos(&point);
            ShowTrayMenu(point);
        }
        return 0;
    }
    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

LRESULT App::OnSettingsMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        const UINT dpi = GetDpiForWindow(window);
        LOGFONTW font{};
        font.lfHeight = -MulDiv(9, dpi, 72);
        wcscpy_s(font.lfFaceName, L"Segoe UI");
        uiFont_ = CreateFontIndirectW(&font);
        CreateSettingsControls();
        return 0;
    }
    case WM_COMMAND:
        HandleSettingsCommand(LOWORD(wParam), HIWORD(wParam));
        return 0;
    case WM_DPICHANGED: {
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOACTIVATE | SWP_NOZORDER);
        if (uiFont_) DeleteObject(uiFont_);
        LOGFONTW font{};
        font.lfHeight = -MulDiv(9, HIWORD(wParam), 72);
        wcscpy_s(font.lfFaceName, L"Segoe UI");
        uiFont_ = CreateFontIndirectW(&font);
        std::array controls{modifierLabel_, modifierCombo_, autostartCheck_, languageLabel_,
            languageCombo_, helpLabel_, closeButton_};
        for (HWND control : controls) ApplyControlFont(control);
        LayoutSettingsControls(HIWORD(wParam));
        return 0;
    }
    case WM_CLOSE:
        ShowWindow(window, SW_HIDE);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace resize_symmetrically
