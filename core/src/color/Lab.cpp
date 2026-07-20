#include "ss/core/color/Lab.h"

#include <cmath>

namespace ss::core::color {

namespace {

// D65 reference white, 2-degree observer.
constexpr double kRefX = 95.047;
constexpr double kRefY = 100.000;
constexpr double kRefZ = 108.883;

double srgbToLinear(double c) {
    c /= 255.0;
    return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

double labF(double t) {
    constexpr double kDelta = 6.0 / 29.0;
    if (t > kDelta * kDelta * kDelta) {
        return std::cbrt(t);
    }
    return t / (3.0 * kDelta * kDelta) + 4.0 / 29.0;
}

} // namespace

Lab rgbToLab(unsigned char r, unsigned char g, unsigned char b) {
    const double rl = srgbToLinear(r);
    const double gl = srgbToLinear(g);
    const double bl = srgbToLinear(b);

    // sRGB (linear) -> XYZ, D65.
    const double x = (rl * 0.4124564 + gl * 0.3575761 + bl * 0.1804375) * 100.0;
    const double y = (rl * 0.2126729 + gl * 0.7151522 + bl * 0.0721750) * 100.0;
    const double z = (rl * 0.0193339 + gl * 0.1191920 + bl * 0.9503041) * 100.0;

    const double fx = labF(x / kRefX);
    const double fy = labF(y / kRefY);
    const double fz = labF(z / kRefZ);

    Lab lab;
    lab.l = 116.0 * fy - 16.0;
    lab.a = 500.0 * (fx - fy);
    lab.b = 200.0 * (fy - fz);
    return lab;
}

} // namespace ss::core::color
