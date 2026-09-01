#include "ResizeEngine.h"

#include <dwmapi.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace resize_symmetrically {
namespace {

ResizeEngine* g_engine = nullptr;
constexpr DWORD kWindowMessageTimeoutMs = 40;

DWORD ProcessIntegrityLevel(DWORD processId) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return MAXDWORD;
    }
    HANDLE token = nullptr;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token)) {
        CloseHandle(process);
        return MAXDWORD;
    }

    DWORD required = 0;
    GetTokenInformation(token, TokenIntegrityLevel, nullptr, 0, &required);
    std::array<BYTE, 256> local{};
    BYTE* buffer = local.data();
    HANDLE allocation = nullptr;
    if (required > local.size()) {
        allocation = HeapAlloc(GetProcessHeap(), 0, required);
        buffer = static_cast<BYTE*>(allocation);
    }

    DWORD level = MAXDWORD;
    if (buffer && GetTokenInformation(token, TokenIntegrityLevel, buffer, required, &required)) {
        const auto* label = reinterpret_cast<const TOKEN_MANDATORY_LABEL*>(buffer);
        const PSID sid = label->Label.Sid;
        const DWORD count = *GetSidSubAuthorityCount(sid);
        level = *GetSidSubAuthority(sid, count - 1);
    }

    if (allocation) {
        HeapFree(GetProcessHeap(), 0, allocation);
    }
    CloseHandle(token);
    CloseHandle(process);
    return level;
}

bool IsKeyDownMessage(WPARAM message) {
    return message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
}

bool IsKeyUpMessage(WPARAM message) {
    return message == WM_KEYUP || message == WM_SYSKEYUP;
}

bool IsLeftVariant(DWORD vk) {
    return vk == VK_LMENU || vk == VK_LCONTROL || vk == VK_LSHIFT || vk == VK_LWIN;
}

bool IsRightVariant(DWORD vk) {
    return vk == VK_RMENU || vk == VK_RCONTROL || vk == VK_RSHIFT || vk == VK_RWIN;
}

}  // namespace

ResizeEngine::ResizeEngine() {
    InitializeCriticalSection(&pendingLock_);
}

ResizeEngine::~ResizeEngine() {
    Stop();
    DeleteCriticalSection(&pendingLock_);
}

bool ResizeEngine::Start(HWND coordinatorWindow, ModifierKey modifier) {
    if (thread_) {
        return true;
    }
    coordinatorWindow_ = coordinatorWindow;
    modifier_.store(modifier);
    readyEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!readyEvent_) {
        return false;
    }

    g_engine = this;
    thread_ = CreateThread(nullptr, 0, ThreadEntry, this, 0, &threadId_);
    if (!thread_) {
        g_engine = nullptr;
        CloseHandle(readyEvent_);
        readyEvent_ = nullptr;
        return false;
    }

    const DWORD wait = WaitForSingleObject(readyEvent_, 3000);
    CloseHandle(readyEvent_);
    readyEvent_ = nullptr;
    if (wait != WAIT_OBJECT_0 || !mouseHook_ || !keyboardHook_) {
        Stop();
        return false;
    }
    SetModifier(modifier);
    return true;
}

void ResizeEngine::Stop() {
    if (!thread_) {
        return;
    }
    PostThreadMessageW(threadId_, WM_QUIT, 0, 0);
    WaitForSingleObject(thread_, 3000);
    CloseHandle(thread_);
    thread_ = nullptr;
    threadId_ = 0;
    gestureActive_.store(false);
    g_engine = nullptr;
}

void ResizeEngine::SetModifier(ModifierKey modifier) {
    modifier_.store(modifier);
    modifierState_.store(0);
    suppressNextModifierRelease_.store(false);

    auto physical = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    unsigned state = 0;
    switch (modifier) {
    case ModifierKey::Alt:
        if (physical(VK_LMENU)) state |= 1;
        if (physical(VK_RMENU)) state |= 2;
        break;
    case ModifierKey::Ctrl:
        if (physical(VK_LCONTROL)) state |= 1;
        if (physical(VK_RCONTROL)) state |= 2;
        break;
    case ModifierKey::Shift:
        if (physical(VK_LSHIFT)) state |= 1;
        if (physical(VK_RSHIFT)) state |= 2;
        break;
    case ModifierKey::Win:
        if (physical(VK_LWIN)) state |= 1;
        if (physical(VK_RWIN)) state |= 2;
        break;
    }
    modifierState_.store(state);
}

DWORD WINAPI ResizeEngine::ThreadEntry(void* context) {
    return static_cast<ResizeEngine*>(context)->RunHookThread();
}

DWORD ResizeEngine::RunHookThread() {
    MSG message{};
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    const HINSTANCE module = GetModuleHandleW(nullptr);
    mouseHook_ = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, module, 0);
    keyboardHook_ = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, module, 0);
    if (readyEvent_) {
        SetEvent(readyEvent_);
    }

    if (!mouseHook_ || !keyboardHook_) {
        if (mouseHook_) UnhookWindowsHookEx(mouseHook_);
        if (keyboardHook_) UnhookWindowsHookEx(keyboardHook_);
        mouseHook_ = nullptr;
        keyboardHook_ = nullptr;
        return 1;
    }

    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    UnhookWindowsHookEx(mouseHook_);
    UnhookWindowsHookEx(keyboardHook_);
    mouseHook_ = nullptr;
    keyboardHook_ = nullptr;
    return 0;
}

LRESULT CALLBACK ResizeEngine::MouseHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_engine) {
        const auto& data = *reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        const LRESULT handled = g_engine->HandleMouse(wParam, data);
        if (handled != 0) {
            return handled;
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK ResizeEngine::KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_engine) {
        const auto& data = *reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        const LRESULT handled = g_engine->HandleKeyboard(wParam, data);
        if (handled != 0) {
            return handled;
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT ResizeEngine::HandleMouse(WPARAM message, const MSLLHOOKSTRUCT& data) {
    if (message == WM_LBUTTONDOWN && !session_.active) {
        if (IsModifierDown() && BeginSession(data.pt)) {
            return 1;
        }
        return 0;
    }

    if (!session_.active) {
        return 0;
    }

    if (message == WM_MOUSEMOVE) {
        if (IsModifierDown()) {
            if (session_.suspended) {
                // Rebase the gesture so pressing the modifier again resumes
                // from the frozen rectangle without a visual jump.
                session_.initialRect = session_.currentRect;
                session_.initialCursor = data.pt;
                session_.suspended = false;
                return 1;
            }
            const RECT rect = CalculateSymmetricRect(
                session_.initialRect,
                session_.initialCursor,
                data.pt,
                session_.edge,
                session_.constraints);
            session_.currentRect = rect;
            QueueResize(rect, session_.target);
        } else {
            session_.suspended = true;
        }
        return 1;
    }
    if (message == WM_LBUTTONUP) {
        EndSession();
        return 1;
    }
    return 0;
}

LRESULT ResizeEngine::HandleKeyboard(WPARAM message, const KBDLLHOOKSTRUCT& data) {
    if ((data.flags & LLKHF_INJECTED) != 0 || !MatchesSelectedModifier(data.vkCode)) {
        return 0;
    }

    if (IsKeyDownMessage(message)) {
        UpdateModifierState(data.vkCode, true);
        return 0;
    }
    if (!IsKeyUpMessage(message)) {
        return 0;
    }

    UpdateModifierState(data.vkCode, false);
    if (gestureActive_.load() && !IsModifierDown()) {
        session_.suspended = true;
    }
    const ModifierKey modifier = modifier_.load();
    const bool needsSuppression = modifier == ModifierKey::Alt || modifier == ModifierKey::Win;
    if (needsSuppression && (gestureActive_.load() || suppressNextModifierRelease_.exchange(false))) {
        const HWND target = lastTarget_.load();
        if (target && IsWindow(target)) {
            const UINT keyMessage = modifier == ModifierKey::Alt ? WM_SYSKEYUP : WM_KEYUP;
            const LPARAM keyFlags = modifier == ModifierKey::Alt ? 0xE0000001 : 0xC0000001;
            PostMessageW(target, keyMessage, data.vkCode, keyFlags);
            PostMessageW(target, WM_CANCELMODE, 0, 0);
        }
        return 1;
    }
    return 0;
}

bool ResizeEngine::BeginSession(POINT cursor) {
    HWND target = WindowFromPoint(cursor);
    if (!target) {
        return false;
    }
    target = GetAncestor(target, GA_ROOT);

    RECT rect{};
    RECT monitorBounds{};
    if (!IsSupportedTarget(target, rect, monitorBounds)) {
        return false;
    }

    DWORD_PTR hitTest = HTNOWHERE;
    const LPARAM coordinates = MAKELPARAM(
        static_cast<WORD>(static_cast<SHORT>(cursor.x)),
        static_cast<WORD>(static_cast<SHORT>(cursor.y)));
    if (!SendMessageTimeoutW(target, WM_NCHITTEST, 0, coordinates,
            SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
            kWindowMessageTimeoutMs, &hitTest)) {
        return false;
    }
    const ResizeEdge edge = EdgeFromHitTest(static_cast<LRESULT>(hitTest));
    if (edge == ResizeEdge::None) {
        return false;
    }

    const UINT dpi = GetDpiForWindow(target);
    MINMAXINFO info{};
    info.ptMinTrackSize = {
        GetSystemMetricsForDpi(SM_CXMINTRACK, dpi ? dpi : USER_DEFAULT_SCREEN_DPI),
        GetSystemMetricsForDpi(SM_CYMINTRACK, dpi ? dpi : USER_DEFAULT_SCREEN_DPI)};
    info.ptMaxTrackSize = {
        GetSystemMetricsForDpi(SM_CXMAXTRACK, dpi ? dpi : USER_DEFAULT_SCREEN_DPI),
        GetSystemMetricsForDpi(SM_CYMAXTRACK, dpi ? dpi : USER_DEFAULT_SCREEN_DPI)};
    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(target, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info),
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
        kWindowMessageTimeoutMs, &ignored);

    session_.active = true;
    session_.suspended = false;
    session_.target = target;
    session_.edge = edge;
    session_.initialRect = rect;
    session_.currentRect = rect;
    session_.initialCursor = cursor;
    session_.constraints.minimum = {info.ptMinTrackSize.x, info.ptMinTrackSize.y};
    session_.constraints.maximum = {info.ptMaxTrackSize.x, info.ptMaxTrackSize.y};
    session_.constraints.screenBounds = monitorBounds;
    lastTarget_.store(target);
    gestureActive_.store(true);
    return true;
}

void ResizeEngine::EndSession() {
    if (!session_.active) {
        return;
    }
    lastTarget_.store(session_.target);
    session_.active = false;
    gestureActive_.store(false);
    const ModifierKey modifier = modifier_.load();
    if (IsModifierDown() && (modifier == ModifierKey::Alt || modifier == ModifierKey::Win)) {
        suppressNextModifierRelease_.store(true);
    }
}

bool ResizeEngine::IsModifierDown() const {
    return modifierState_.load() != 0;
}

bool ResizeEngine::MatchesSelectedModifier(DWORD vkCode) const {
    switch (modifier_.load()) {
    case ModifierKey::Alt:
        return vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU;
    case ModifierKey::Ctrl:
        return vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL;
    case ModifierKey::Shift:
        return vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT;
    case ModifierKey::Win:
        return vkCode == VK_LWIN || vkCode == VK_RWIN;
    }
    return false;
}

void ResizeEngine::UpdateModifierState(DWORD vkCode, bool down) {
    unsigned bit = IsRightVariant(vkCode) ? 2U : 1U;
    if (!IsLeftVariant(vkCode) && !IsRightVariant(vkCode)) {
        bit = 1U;
    }
    if (down) {
        modifierState_.fetch_or(bit);
    } else {
        modifierState_.fetch_and(~bit);
    }
}

void ResizeEngine::QueueResize(const RECT& rect, HWND target) {
    bool post = false;
    EnterCriticalSection(&pendingLock_);
    pendingRect_ = rect;
    pendingTarget_ = target;
    if (!pendingMessagePosted_) {
        pendingMessagePosted_ = true;
        post = true;
    }
    LeaveCriticalSection(&pendingLock_);
    if (post) {
        PostMessageW(coordinatorWindow_, kMessageApplyResize, 0, 0);
    }
}

void ResizeEngine::ApplyPendingResize() {
    RECT rect{};
    HWND target = nullptr;
    EnterCriticalSection(&pendingLock_);
    rect = pendingRect_;
    target = pendingTarget_;
    pendingMessagePosted_ = false;
    LeaveCriticalSection(&pendingLock_);

    if (!target || !IsWindow(target)) {
        return;
    }
    SetWindowPos(target, nullptr, rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_ASYNCWINDOWPOS);
}

bool ResizeEngine::IsSupportedTarget(HWND window, RECT& rect, RECT& monitorBounds) const {
    if (!window || !IsWindowVisible(window) || !IsWindowEnabled(window) ||
        IsIconic(window) || IsZoomed(window)) {
        return false;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == 0 || processId == GetCurrentProcessId() || IsHigherIntegrityProcess(window)) {
        return false;
    }

    const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    if ((style & WS_THICKFRAME) == 0 || (style & WS_CHILD) != 0) {
        return false;
    }

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) {
        return false;
    }
    if (!GetWindowRect(window, &rect)) {
        return false;
    }

    MONITORINFO monitor{sizeof(monitor)};
    const HMONITOR handle = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    if (!GetMonitorInfoW(handle, &monitor)) {
        return false;
    }
    monitorBounds = monitor.rcMonitor;

    // Borderless monitor-sized windows are treated as fullscreen even if they
    // retained WS_THICKFRAME internally.
    if (rect.left <= monitor.rcMonitor.left && rect.top <= monitor.rcMonitor.top &&
        rect.right >= monitor.rcMonitor.right && rect.bottom >= monitor.rcMonitor.bottom &&
        (style & WS_CAPTION) == 0) {
        return false;
    }
    return true;
}

bool ResizeEngine::IsHigherIntegrityProcess(HWND window) const {
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    const DWORD targetLevel = ProcessIntegrityLevel(processId);
    const DWORD currentLevel = ProcessIntegrityLevel(GetCurrentProcessId());
    if (targetLevel == MAXDWORD || currentLevel == MAXDWORD) {
        return true;
    }
    return targetLevel > currentLevel;
}

}  // namespace resize_symmetrically
