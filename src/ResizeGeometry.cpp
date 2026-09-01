#include "ResizeGeometry.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace resize_symmetrically {
namespace {

long ClampDimension(long desired, long minimum, long maximum, long centerSum) noexcept {
    minimum = std::max(1L, minimum);
    maximum = std::max(minimum, maximum);
    desired = std::clamp(desired, minimum, maximum);

    // Keeping the dimension parity equal to left + right preserves an exact
    // half-pixel center even for odd-sized windows.
    if (((desired ^ centerSum) & 1L) != 0) {
        if (desired < maximum) {
            ++desired;
        } else if (desired > minimum) {
            --desired;
        }
    }
    return desired;
}

long SafeTwiceDistance(long centerSum, long boundary, bool lowerBoundary) noexcept {
    const std::int64_t value = lowerBoundary
        ? static_cast<std::int64_t>(centerSum) - 2LL * boundary
        : 2LL * boundary - static_cast<std::int64_t>(centerSum);
    return static_cast<long>(std::clamp<std::int64_t>(value, 1, LONG_MAX));
}

}  // namespace

bool AffectsHorizontal(ResizeEdge edge) noexcept {
    return edge == ResizeEdge::Left || edge == ResizeEdge::Right ||
        edge == ResizeEdge::TopLeft || edge == ResizeEdge::TopRight ||
        edge == ResizeEdge::BottomLeft || edge == ResizeEdge::BottomRight;
}

bool AffectsVertical(ResizeEdge edge) noexcept {
    return edge == ResizeEdge::Top || edge == ResizeEdge::Bottom ||
        edge == ResizeEdge::TopLeft || edge == ResizeEdge::TopRight ||
        edge == ResizeEdge::BottomLeft || edge == ResizeEdge::BottomRight;
}

ResizeEdge EdgeFromHitTest(LRESULT hitTest) noexcept {
    switch (hitTest) {
    case HTLEFT: return ResizeEdge::Left;
    case HTRIGHT: return ResizeEdge::Right;
    case HTTOP: return ResizeEdge::Top;
    case HTBOTTOM: return ResizeEdge::Bottom;
    case HTTOPLEFT: return ResizeEdge::TopLeft;
    case HTTOPRIGHT: return ResizeEdge::TopRight;
    case HTBOTTOMLEFT: return ResizeEdge::BottomLeft;
    case HTBOTTOMRIGHT: return ResizeEdge::BottomRight;
    default: return ResizeEdge::None;
    }
}

RECT CalculateSymmetricRect(
    const RECT& initialRect,
    POINT initialCursor,
    POINT currentCursor,
    ResizeEdge edge,
    const ResizeConstraints& constraints) noexcept {
    RECT result = initialRect;
    const long initialWidth = std::max(1L, initialRect.right - initialRect.left);
    const long initialHeight = std::max(1L, initialRect.bottom - initialRect.top);
    const long centerXSum = initialRect.left + initialRect.right;
    const long centerYSum = initialRect.top + initialRect.bottom;

    if (AffectsHorizontal(edge)) {
        const long delta = currentCursor.x - initialCursor.x;
        long desiredWidth = initialWidth;
        if (edge == ResizeEdge::Left || edge == ResizeEdge::TopLeft || edge == ResizeEdge::BottomLeft) {
            desiredWidth -= 2 * delta;
        } else {
            desiredWidth += 2 * delta;
        }

        const long fitLeft = SafeTwiceDistance(centerXSum, constraints.screenBounds.left, true);
        const long fitRight = SafeTwiceDistance(centerXSum, constraints.screenBounds.right, false);
        // Do not make a pre-existing off-screen window jump. Its initial size
        // becomes the ceiling until it is moved back on screen.
        const long screenMaximum = std::max(initialWidth, std::min(fitLeft, fitRight));
        const long maximum = std::min(
            std::max(initialWidth, constraints.maximum.cx),
            screenMaximum);
        const long minimum = std::min(std::max(1L, constraints.minimum.cx), maximum);
        const long width = ClampDimension(desiredWidth, minimum, maximum, centerXSum);
        result.left = (centerXSum - width) / 2;
        result.right = result.left + width;
    }

    if (AffectsVertical(edge)) {
        const long delta = currentCursor.y - initialCursor.y;
        long desiredHeight = initialHeight;
        if (edge == ResizeEdge::Top || edge == ResizeEdge::TopLeft || edge == ResizeEdge::TopRight) {
            desiredHeight -= 2 * delta;
        } else {
            desiredHeight += 2 * delta;
        }

        const long fitTop = SafeTwiceDistance(centerYSum, constraints.screenBounds.top, true);
        const long fitBottom = SafeTwiceDistance(centerYSum, constraints.screenBounds.bottom, false);
        const long screenMaximum = std::max(initialHeight, std::min(fitTop, fitBottom));
        const long maximum = std::min(
            std::max(initialHeight, constraints.maximum.cy),
            screenMaximum);
        const long minimum = std::min(std::max(1L, constraints.minimum.cy), maximum);
        const long height = ClampDimension(desiredHeight, minimum, maximum, centerYSum);
        result.top = (centerYSum - height) / 2;
        result.bottom = result.top + height;
    }

    return result;
}

}  // namespace resize_symmetrically
