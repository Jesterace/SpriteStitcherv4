#pragma once

#include "ss/core/color/Lab.h"
#include "ss/core/dmc/DmcColor.h"
#include "ss/core/dmc/DmcTable.h"

#include <vector>

namespace ss::core::dmc {

// Finds the nearest DMC floss to an arbitrary RGB color using CIEDE2000
// distance in Lab space. Lab values for the whole table are precomputed
// once at construction so repeated matching (e.g. per-pixel) is cheap.
class DmcMatcher {
public:
    explicit DmcMatcher(const DmcTable& table);

    DmcColor match(unsigned char r, unsigned char g, unsigned char b) const;

private:
    struct Entry {
        DmcColor color;
        color::Lab lab;
    };

    std::vector<Entry> m_entries;
};

} // namespace ss::core::dmc
