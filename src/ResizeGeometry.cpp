#include "ResizeGeometry.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace resize_symmetrically {
namespace {

struct AxisRange {
    long lower;
    long upper;
};

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

AxisRange ShiftIntoBounds(
    AxisRange range, long lowerBound, long upperBound) noexcept {
    if (range.lower < lowerBound) {
        const long shift = lowerBound - range.lower;
        range.lower += shift;
        range.upper += shift;
    }
    if (range.upper > upperBound) {
        const long shift = range.upper - upperBound;
        range.lower -= shift;
        range.upper -= shift;
    }
    return range;
}

AxisRange CenteredRange(
    long centerSum,
    long size,
    long minimum,
    long maximum,
    long lowerBound,
    long upperBound) noexcept {
    size = ClampDimension(size, minimum, maximum, centerSum);
    AxisRange range{(centerSum - size) / 2, 0};
    range.upper = range.lower + size;
    return ShiftIntoBounds(range, lowerBound, upperBound);
}

AxisRange CalculateAxis(
    long initialLower,
    long initialUpper,
    long delta,
    bool draggingLower,
    long minimum,
    long maximum,
    long screenLower,
    long screenUpper) noexcept {
    const long centerSum = initialLower + initialUpper;
    minimum = std::max(1L, minimum);
    maximum = std::max(minimum, maximum);

    // A window that was already partly off-screen must not jump when the
    // gesture begins, but it is never allowed to move farther off-screen.
    const long lowerBound = std::min(initialLower, screenLower);
    const long upperBound = std::max(initialUpper, screenUpper);
    const long available = std::max(1L, upperBound - lowerBound);
    maximum = std::min(maximum, available);
    minimum = std::min(minimum, maximum);

    const std::int64_t rawLower64 = static_cast<std::int64_t>(initialLower) +
        (draggingLower ? delta : -static_cast<std::int64_t>(delta));
    const std::int64_t rawUpper64 = static_cast<std::int64_t>(initialUpper) +
        (draggingLower ? -static_cast<std::int64_t>(delta) : delta);

    // When shrinking crosses or reaches the minimum size, retaining the
    // original center gives stable behaviour and prevents edge inversion.
    const std::int64_t rawSize64 = rawUpper64 - rawLower64;
    if (rawSize64 <= minimum) {
        return CenteredRange(centerSum, minimum, minimum, maximum, lowerBound, upperBound);
    }

    const long rawLower = static_cast<long>(std::clamp<std::int64_t>(
        rawLower64, std::numeric_limits<long>::min(), std::numeric_limits<long>::max()));
    const long rawUpper = static_cast<long>(std::clamp<std::int64_t>(
        rawUpper64, std::numeric_limits<long>::min(), std::numeric_limits<long>::max()));
    AxisRange candidate{
        std::max(rawLower, lowerBound),
        std::min(rawUpper, upperBound)};
    if (candidate.upper <= candidate.lower) {
        return CenteredRange(centerSum, minimum, minimum, maximum, lowerBound, upperBound);
    }

    const long candidateSize = candidate.upper - candidate.lower;
    if (candidateSize < minimum) {
        return CenteredRange(centerSum, minimum, minimum, maximum, lowerBound, upperBound);
    }
    if (candidateSize <= maximum) {
        return candidate;
    }

    // Apply the target window's maximum tracking size to the actual visible
    // rectangle. If one screen edge was reached first, keep it pinned and let
    // the other edge continue until the final size reaches this maximum.
    const bool lowerPinned = rawLower < lowerBound;
    const bool upperPinned = rawUpper > upperBound;
    if (lowerPinned || upperPinned) {
        bool anchorLower = lowerPinned && !upperPinned;
        if (lowerPinned && upperPinned) {
            const long lowerDistance = std::max(0L, initialLower - lowerBound);
            const long upperDistance = std::max(0L, upperBound - initialUpper);
            anchorLower = lowerDistance <= upperDistance;
        }
        AxisRange limited = anchorLower
            ? AxisRange{lowerBound, lowerBound + maximum}
            : AxisRange{upperBound - maximum, upperBound};
        return ShiftIntoBounds(limited, lowerBound, upperBound);
    }
    return CenteredRange(centerSum, maximum, minimum, maximum, lowerBound, upperBound);
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
    if (AffectsHorizontal(edge)) {
        const long delta = currentCursor.x - initialCursor.x;
        const bool draggingLeft = edge == ResizeEdge::Left ||
            edge == ResizeEdge::TopLeft || edge == ResizeEdge::BottomLeft;
        const AxisRange horizontal = CalculateAxis(
            initialRect.left, initialRect.right, delta, draggingLeft,
            constraints.minimum.cx,
            std::max(initialWidth, constraints.maximum.cx),
            constraints.screenBounds.left, constraints.screenBounds.right);
        result.left = horizontal.lower;
        result.right = horizontal.upper;
    }

    if (AffectsVertical(edge)) {
        const long delta = currentCursor.y - initialCursor.y;
        const bool draggingTop = edge == ResizeEdge::Top ||
            edge == ResizeEdge::TopLeft || edge == ResizeEdge::TopRight;
        const AxisRange vertical = CalculateAxis(
            initialRect.top, initialRect.bottom, delta, draggingTop,
            constraints.minimum.cy,
            std::max(initialHeight, constraints.maximum.cy),
            constraints.screenBounds.top, constraints.screenBounds.bottom);
        result.top = vertical.lower;
        result.bottom = vertical.upper;
    }

    return result;
}

}  // namespace resize_symmetrically
