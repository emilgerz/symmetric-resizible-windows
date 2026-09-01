#include "../src/ResizeGeometry.h"

#include <iostream>
#include <string_view>

using resize_symmetrically::CalculateSymmetricRect;
using resize_symmetrically::ResizeConstraints;
using resize_symmetrically::ResizeEdge;

namespace {
int failures = 0;

void ExpectRect(std::string_view name, const RECT& actual, const RECT& expected) {
    if (actual.left == expected.left && actual.top == expected.top &&
        actual.right == expected.right && actual.bottom == expected.bottom) {
        return;
    }
    std::cerr << name << ": expected [" << expected.left << ',' << expected.top << ','
              << expected.right << ',' << expected.bottom << "] but got ["
              << actual.left << ',' << actual.top << ',' << actual.right << ','
              << actual.bottom << "]\n";
    ++failures;
}

ResizeConstraints Constraints(RECT bounds, SIZE minimum = {1, 1}, SIZE maximum = {LONG_MAX, LONG_MAX}) {
    return {minimum, maximum, bounds};
}
}  // namespace

int main() {
    const RECT initial{100, 100, 500, 400};
    const POINT cursor{100, 250};
    const RECT screen{0, 0, 1920, 1080};

    ExpectRect("left grows symmetrically",
        CalculateSymmetricRect(initial, cursor, {50, 250}, ResizeEdge::Left, Constraints(screen)),
        {50, 100, 550, 400});
    ExpectRect("right grows symmetrically",
        CalculateSymmetricRect(initial, {500, 250}, {550, 250}, ResizeEdge::Right, Constraints(screen)),
        {50, 100, 550, 400});
    ExpectRect("top-left corner",
        CalculateSymmetricRect(initial, {100, 100}, {50, 50}, ResizeEdge::TopLeft, Constraints(screen)),
        {50, 50, 550, 450});
    ExpectRect("minimum tracking size",
        CalculateSymmetricRect(initial, cursor, {400, 250}, ResizeEdge::Left,
            Constraints(screen, {200, 100})),
        {200, 100, 400, 400});
    ExpectRect("screen edge clamp",
        CalculateSymmetricRect(initial, cursor, {-500, 250}, ResizeEdge::Left, Constraints(screen)),
        {0, 100, 600, 400});
    ExpectRect("negative monitor coordinates",
        CalculateSymmetricRect({-1800, 100, -1400, 400}, {-1800, 250}, {-2000, 250}, ResizeEdge::Left,
            Constraints({-1920, 0, 0, 1080})),
        {-1920, 100, -1280, 400});
    ExpectRect("odd center preserved",
        CalculateSymmetricRect({101, 100, 500, 400}, {101, 250}, {100, 250}, ResizeEdge::Left,
            Constraints(screen)),
        {100, 100, 501, 400});
    ExpectRect("uninvolved axis stays fixed",
        CalculateSymmetricRect(initial, {500, 250}, {520, 900}, ResizeEdge::Right, Constraints(screen)),
        {80, 100, 520, 400});

    if (failures != 0) {
        std::cerr << failures << " geometry test(s) failed.\n";
        return 1;
    }
    std::cout << "All geometry tests passed.\n";
    return 0;
}
