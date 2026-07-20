#pragma once

namespace ss::core::dmc {

// Plain floss data point. `code` and `name` point at static string literals
// compiled into the binary (see DmcTableData.cpp), so DmcColor is trivially
// copyable with no ownership concerns.
struct DmcColor {
    const char* code;
    const char* name;
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

} // namespace ss::core::dmc
