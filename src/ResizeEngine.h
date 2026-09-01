#pragma once

#include "ResizeGeometry.h"
#include "Settings.h"

#include <windows.h>

#include <atomic>

namespace resize_symmetrically {

constexpr UINT kMessageApplyResize = WM_APP + 40;
constexpr UINT_PTR kResizeTimerId = 0x5253;

class ResizeEngine {
public:
    ResizeEngine();
    ~ResizeEngine();

    ResizeEngine(const ResizeEngine&) = delete;
    ResizeEngine& operator=(const ResizeEngine&) = delete;

    bool Start(HWND coordinatorWindow, ModifierKey modifier);
    void Stop();
    void SetModifier(ModifierKey modifier);
    void ApplyPendingResize();

private:
    struct Session {
        bool active = false;
        bool suspended = false;
        HWND target = nullptr;
        ResizeEdge edge = ResizeEdge::None;
        RECT initialRect{};
        RECT currentRect{};
        POINT initialCursor{};
        ResizeConstraints constraints{};
    };

    static DWORD WINAPI ThreadEntry(void* context);
    static LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam);

    DWORD RunHookThread();
    LRESULT HandleMouse(WPARAM message, const MSLLHOOKSTRUCT& data);
    LRESULT HandleKeyboard(WPARAM message, const KBDLLHOOKSTRUCT& data);
    bool BeginSession(POINT cursor);
    void EndSession();
    bool IsModifierDown() const;
    bool MatchesSelectedModifier(DWORD vkCode) const;
    void UpdateModifierState(DWORD vkCode, bool down);
    void QueueResize(const RECT& rect, HWND target);
    bool IsSupportedTarget(HWND window, RECT& rect, RECT& monitorBounds) const;
    bool IsHigherIntegrityProcess(HWND window) const;

    HWND coordinatorWindow_ = nullptr;
    HANDLE thread_ = nullptr;
    DWORD threadId_ = 0;
    HANDLE readyEvent_ = nullptr;
    HHOOK mouseHook_ = nullptr;
    HHOOK keyboardHook_ = nullptr;
    std::atomic<ModifierKey> modifier_{ModifierKey::Alt};
    std::atomic<unsigned> modifierState_{0};
    std::atomic<bool> gestureActive_{false};
    std::atomic<bool> cancelMenuOnModifierRelease_{false};
    std::atomic<HWND> lastTarget_{nullptr};
    Session session_{};

    CRITICAL_SECTION pendingLock_{};
    RECT pendingRect_{};
    HWND pendingTarget_ = nullptr;
    bool pendingDirty_ = false;
    bool pendingUpdateScheduled_ = false;
    ULONGLONG nextApplyTick_ = 0;
};

}  // namespace resize_symmetrically
