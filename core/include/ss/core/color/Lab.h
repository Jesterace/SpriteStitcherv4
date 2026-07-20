#pragma once

namespace ss::core::color {

struct Lab {
    double l = 0.0;
    double a = 0.0;
    double b = 0.0;
};

// Converts 8-bit sRGB to CIE L*a*b* (D65 white point).
Lab rgbToLab(unsigned char r, unsigned char g, unsigned char b);

} // namespace ss::core::color
