#pragma once

#include <QString>

namespace ss::core::pattern {

// One stitch. An empty `dmcCode` means the cell is unset/blank (no stitch).
struct PatternCell {
    QString dmcCode;
    QString symbol;
};

} // namespace ss::core::pattern
