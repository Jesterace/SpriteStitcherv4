#pragma once

#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"

#include <QString>

#include <vector>

namespace ss::core::pdf {

struct ExportOptions {
    QString patternName;
    // Aida fabric counts (stitches/inch) to show finished-size estimates
    // for in the header.
    std::vector<int> fabricCounts = {14, 16, 18};
};

// Renders a "Wooper style" chart: colored cells with overlaid symbols,
// light-gray grid, red center lines, ruler numbers every 10 stitches, a
// header with dimensions/counts/finished-size estimates, and a DMC legend
// (symbol, code, stitch count, swatch). Auto-tiles across multiple pages
// for patterns too large to read at a legible cell size on one page.
//
// Uses libharu with base-14 Helvetica and a single-byte standard encoding
// exclusively — never Qt's PDF facilities, and never a CID/Identity-H font
// path — because Pattern Keeper cannot parse two-byte-encoded text. Do not
// introduce QPdfWriter or multi-byte font encodings into this path.
class PdfExporter {
public:
    static bool exportToPdf(const pattern::PatternModel& pattern,
                             const dmc::DmcTable& table,
                             const QString& outputPath,
                             const ExportOptions& options,
                             QString* errorOut = nullptr);
};

} // namespace ss::core::pdf
