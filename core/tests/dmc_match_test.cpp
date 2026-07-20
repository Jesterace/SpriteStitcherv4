#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/dmc/DmcTable.h"

#include <cstdio>
#include <string>

using namespace ss::core::dmc;

int main() {
    DmcTable table;
    int failures = 0;

    if (table.colors().size() != 454) {
        std::fprintf(stderr, "FAIL: expected 454 DMC colors, got %zu\n", table.colors().size());
        ++failures;
    }

    DmcMatcher matcher(table);

    auto check = [&](unsigned char r, unsigned char g, unsigned char b, const char* expectedCode) {
        const DmcColor m = matcher.match(r, g, b);
        if (std::string(m.code) != expectedCode) {
            std::fprintf(stderr, "FAIL: rgb(%d,%d,%d) matched %s, expected %s\n",
                         r, g, b, m.code, expectedCode);
            ++failures;
        } else {
            std::printf("OK: rgb(%d,%d,%d) -> %s (%s)\n", r, g, b, m.code, m.name);
        }
    };

    check(0, 0, 0, "310");        // pure black
    check(255, 255, 255, "B5200"); // pure white

    // Every table entry's own RGB should match a table entry with identical
    // RGB (itself, or an exact duplicate if one exists) — distance 0 must
    // win over every other entry.
    for (const auto& c : table.colors()) {
        const DmcColor m = matcher.match(c.r, c.g, c.b);
        if (m.r != c.r || m.g != c.g || m.b != c.b) {
            std::fprintf(stderr, "FAIL: self-match for %s (%d,%d,%d) returned %s (%d,%d,%d)\n",
                         c.code, c.r, c.g, c.b, m.code, m.r, m.g, m.b);
            ++failures;
        }
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All DMC match tests passed.\n");
    return 0;
}
