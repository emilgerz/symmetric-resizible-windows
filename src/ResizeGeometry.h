#pragma once

#include <windows.h>

namespace resize_symmetrically {

enum class ResizeEdge {
    None,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

struct ResizeConstraints {
    SIZE minimum{1, 1};
    SIZE maximum{LONG_MAX, LONG_MAX};
    RECT screenBounds{};
};

[[nodiscard]] bool AffectsHorizontal(ResizeEdge edge) noexcept;
[[nodiscard]] bool AffectsVertical(ResizeEdge edge) noexcept;
[[nodiscard]] ResizeEdge EdgeFromHitTest(LRESULT hitTest) noexcept;

// Returns a rectangle whose center is identical to initialRect's center. The
// size is clamped to the target window's tracking limits and to the monitor
// that contained the window when the gesture started.
[[nodiscard]] RECT CalculateSymmetricRect(
    const RECT& initialRect,
    POINT initialCursor,
    POINT currentCursor,
    ResizeEdge edge,
    const ResizeConstraints& constraints) noexcept;

}  // namespace resize_symmetrically
