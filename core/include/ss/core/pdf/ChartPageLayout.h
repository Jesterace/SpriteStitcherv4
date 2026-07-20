#pragma once

#include <vector>

namespace ss::core::pdf {

struct PageGeometry {
    double widthPt = 612.0;  // US Letter portrait, 8.5x11in @ 72pt/in
    double heightPt = 792.0;
};

// A contiguous stitch range rendered on one chart page. Ranges are
// [start, end) in absolute pattern coordinates, never tile-local, so ruler
// numbers and center lines stay meaningful across a multi-page chart.
struct Tile {
    int startCol = 0;
    int endCol = 0;
    int startRow = 0;
    int endRow = 0;
    int tileCol = 0;
    int tileRow = 0;
};

// Geometry for one exported chart: how many pages the stitch grid needs,
// how large each cell is, and (when it fits) whether the header/legend can
// share the single chart page or need a dedicated info page.
struct ChartLayout {
    double cellPt = 0.0;
    int tilesX = 1;
    int tilesY = 1;
    std::vector<Tile> tiles; // row-major: tileRow-major, then tileCol
    bool singlePage = true;

    // Legend geometry, shared between layout and rendering so column math
    // only happens once.
    int legendColumns = 1;
    int legendRowsPerColumn = 0;
};

// Pure geometry — no libharu/Qt dependency — so pagination math can be unit
// tested without touching PDF generation at all.
class ChartPageLayout {
public:
    // Shared with the renderer so layout math and drawing never disagree.
    static constexpr double kRulerMarginPt = 18.0;
    static constexpr double kHeaderHeightPt = 60.0;       // header sharing a page with the chart
    static constexpr double kInfoHeaderHeightPt = 80.0;   // header alone on a dedicated info page
    static constexpr double kLegendTitleHeightPt = 14.0;
    static constexpr double kLegendRowHeightPt = 12.0;
    static constexpr double kLegendColumnWidthPt = 170.0;
    static constexpr double kTileCaptionHeightPt = 20.0;

    static PageGeometry letterPortrait() { return PageGeometry{612.0, 792.0}; }

    static ChartLayout compute(int patternWidth, int patternHeight, int legendEntryCount,
                                const PageGeometry& page, double marginPt = 36.0,
                                double minCellPt = 10.0, double maxCellPt = 18.0);
};

} // namespace ss::core::pdf
