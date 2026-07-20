#include "ss/core/color/ColorDistance.h"

#include <cmath>

namespace ss::core::color {

namespace {
constexpr double kPi = 3.14159265358979323846;

double deg2rad(double deg) { return deg * kPi / 180.0; }
double rad2deg(double rad) { return rad * 180.0 / kPi; }
} // namespace

// Standard CIEDE2000 (Sharma, Wu, Dalal 2005) with kL = kC = kH = 1.
double ciede2000(const Lab& lab1, const Lab& lab2) {
    const double l1 = lab1.l, a1 = lab1.a, b1 = lab1.b;
    const double l2 = lab2.l, a2 = lab2.a, b2 = lab2.b;

    const double c1 = std::sqrt(a1 * a1 + b1 * b1);
    const double c2 = std::sqrt(a2 * a2 + b2 * b2);
    const double cBar = (c1 + c2) / 2.0;

    const double cBar7 = std::pow(cBar, 7.0);
    const double g = 0.5 * (1.0 - std::sqrt(cBar7 / (cBar7 + std::pow(25.0, 7.0))));

    const double a1p = a1 * (1.0 + g);
    const double a2p = a2 * (1.0 + g);

    const double c1p = std::sqrt(a1p * a1p + b1 * b1);
    const double c2p = std::sqrt(a2p * a2p + b2 * b2);

    auto hueAngle = [](double a, double b) {
        if (a == 0.0 && b == 0.0) return 0.0;
        double h = rad2deg(std::atan2(b, a));
        return h < 0.0 ? h + 360.0 : h;
    };

    const double h1p = hueAngle(a1p, b1);
    const double h2p = hueAngle(a2p, b2);

    const double deltaLp = l2 - l1;
    const double deltaCp = c2p - c1p;

    double deltahp = 0.0;
    if (c1p * c2p != 0.0) {
        deltahp = h2p - h1p;
        if (deltahp > 180.0) deltahp -= 360.0;
        else if (deltahp < -180.0) deltahp += 360.0;
    }
    const double deltaHp = 2.0 * std::sqrt(c1p * c2p) * std::sin(deg2rad(deltahp) / 2.0);

    const double lBarp = (l1 + l2) / 2.0;
    const double cBarp = (c1p + c2p) / 2.0;

    double hBarp;
    if (c1p * c2p == 0.0) {
        hBarp = h1p + h2p;
    } else if (std::fabs(h1p - h2p) <= 180.0) {
        hBarp = (h1p + h2p) / 2.0;
    } else if (h1p + h2p < 360.0) {
        hBarp = (h1p + h2p + 360.0) / 2.0;
    } else {
        hBarp = (h1p + h2p - 360.0) / 2.0;
    }

    const double t = 1.0
        - 0.17 * std::cos(deg2rad(hBarp - 30.0))
        + 0.24 * std::cos(deg2rad(2.0 * hBarp))
        + 0.32 * std::cos(deg2rad(3.0 * hBarp + 6.0))
        - 0.20 * std::cos(deg2rad(4.0 * hBarp - 63.0));

    const double deltaTheta = 30.0 * std::exp(-std::pow((hBarp - 275.0) / 25.0, 2.0));
    const double cBarp7 = std::pow(cBarp, 7.0);
    const double rc = 2.0 * std::sqrt(cBarp7 / (cBarp7 + std::pow(25.0, 7.0)));
    const double sl = 1.0 + (0.015 * std::pow(lBarp - 50.0, 2.0))
        / std::sqrt(20.0 + std::pow(lBarp - 50.0, 2.0));
    const double sc = 1.0 + 0.045 * cBarp;
    const double sh = 1.0 + 0.015 * cBarp * t;
    const double rt = -std::sin(deg2rad(2.0 * deltaTheta)) * rc;

    const double lTerm = deltaLp / sl;
    const double cTerm = deltaCp / sc;
    const double hTerm = deltaHp / sh;

    return std::sqrt(lTerm * lTerm + cTerm * cTerm + hTerm * hTerm + rt * cTerm * hTerm);
}

} // namespace ss::core::color
