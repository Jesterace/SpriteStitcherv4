#include "ss/core/dmc/DmcMatcher.h"

#include "ss/core/color/ColorDistance.h"

#include <limits>

namespace ss::core::dmc {

DmcMatcher::DmcMatcher(const DmcTable& table) {
    m_entries.reserve(table.colors().size());
    for (const auto& c : table.colors()) {
        Entry e;
        e.color = c;
        e.lab = color::rgbToLab(c.r, c.g, c.b);
        m_entries.push_back(e);
    }
}

DmcColor DmcMatcher::match(unsigned char r, unsigned char g, unsigned char b) const {
    const color::Lab target = color::rgbToLab(r, g, b);

    double bestDist = std::numeric_limits<double>::max();
    const Entry* best = nullptr;
    for (const auto& e : m_entries) {
        const double d = color::ciede2000(target, e.lab);
        if (d < bestDist) {
            bestDist = d;
            best = &e;
        }
    }
    return best->color;
}

} // namespace ss::core::dmc
