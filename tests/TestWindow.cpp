#include <windows.h>

#include <string>

namespace {

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize = {360, 220};
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT rect{};
        GetWindowRect(window, &rect);
        const long centerX2 = rect.left + rect.right;
        const long centerY2 = rect.top + rect.bottom;
        const std::wstring text =
            L"Resize Symmetrically integration window\r\n\r\n"
            L"Minimum tracking size: 360 x 220\r\n"
            L"Outer rectangle: [" + std::to_wstring(rect.left) + L", " +
            std::to_wstring(rect.top) + L", " + std::to_wstring(rect.right) + L", " +
            std::to_wstring(rect.bottom) + L"]\r\nCenter x2/y2: " +
            std::to_wstring(centerX2) + L" / " + std::to_wstring(centerY2);
        SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, text.c_str(), -1, &paint.rcPaint, DT_LEFT | DT_TOP | DT_WORDBREAK);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_WINDOWPOSCHANGED:
        InvalidateRect(window, nullptr, TRUE);
        return DefWindowProcW(window, message, wParam, lParam);
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    constexpr wchar_t className[] = L"ResizeSymmetrically.TestWindow";
    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = className;
    if (!RegisterClassExW(&windowClass)) return 1;

    HWND window = CreateWindowExW(0, className, L"Resize Symmetrically — Test Window",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 480,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 2;
    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
