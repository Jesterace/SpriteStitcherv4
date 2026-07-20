#include "ss/core/pdf/ChartPageLayout.h"

#include <algorithm>
#include <cmath>

namespace ss::core::pdf {

namespace {

int legendColumnsFor(int entryCount, double contentWidth) {
    const int columns = std::max(1, static_cast<int>(contentWidth / ChartPageLayout::kLegendColumnWidthPt));
    return std::min(columns, std::max(1, entryCount));
}

double legendHeightFor(int entryCount, int columns) {
    const int rowsPerColumn = columns > 0 ? static_cast<int>(std::ceil(double(entryCount) / columns)) : 0;
    return ChartPageLayout::kLegendTitleHeightPt + rowsPerColumn * ChartPageLayout::kLegendRowHeightPt;
}

} // namespace

ChartLayout ChartPageLayout::compute(int patternWidth, int patternHeight, int legendEntryCount,
                                      const PageGeometry& page, double marginPt,
                                      double minCellPt, double maxCellPt) {
    ChartLayout layout;
    if (patternWidth <= 0 || patternHeight <= 0) {
        return layout;
    }

    const double contentWidth = page.widthPt - 2.0 * marginPt;
    const double contentHeight = page.heightPt - 2.0 * marginPt;

    const int legendColumns = legendColumnsFor(legendEntryCount, contentWidth);
    const double legendHeight = legendHeightFor(legendEntryCount, legendColumns);

    layout.legendColumns = legendColumns;
    layout.legendRowsPerColumn = legendColumns > 0
        ? static_cast<int>(std::ceil(double(legendEntryCount) / legendColumns))
        : 0;

    // Try to fit header + chart + legend together on one page first.
    const double singlePageChartWidth = contentWidth - kRulerMarginPt;
    const double singlePageChartHeight = contentHeight - kHeaderHeightPt - legendHeight - kRulerMarginPt;

    const double fitCellW = singlePageChartWidth / patternWidth;
    const double fitCellH = singlePageChartHeight / patternHeight;
    const double fitCell = std::min({fitCellW, fitCellH, maxCellPt});

    if (fitCell >= minCellPt && singlePageChartHeight > 0) {
        layout.singlePage = true;
        layout.cellPt = fitCell;
        layout.tilesX = 1;
        layout.tilesY = 1;
        Tile tile;
        tile.startCol = 0;
        tile.endCol = patternWidth;
        tile.startRow = 0;
        tile.endRow = patternHeight;
        tile.tileCol = 0;
        tile.tileRow = 0;
        layout.tiles.push_back(tile);
        return layout;
    }

    // Doesn't fit: dedicate page 1 to header+legend, tile the chart across
    // as many pages as needed at a fixed, guaranteed-readable cell size.
    layout.singlePage = false;
    layout.cellPt = minCellPt;

    const double tileChartWidth = contentWidth - kRulerMarginPt;
    const double tileChartHeight = contentHeight - kRulerMarginPt - kTileCaptionHeightPt;

    const int maxColsPerTile = std::max(1, static_cast<int>(tileChartWidth / minCellPt));
    const int maxRowsPerTile = std::max(1, static_cast<int>(tileChartHeight / minCellPt));

    layout.tilesX = static_cast<int>(std::ceil(double(patternWidth) / maxColsPerTile));
    layout.tilesY = static_cast<int>(std::ceil(double(patternHeight) / maxRowsPerTile));

    const int colsPerTile = static_cast<int>(std::ceil(double(patternWidth) / layout.tilesX));
    const int rowsPerTile = static_cast<int>(std::ceil(double(patternHeight) / layout.tilesY));

    for (int ty = 0; ty < layout.tilesY; ++ty) {
        for (int tx = 0; tx < layout.tilesX; ++tx) {
            Tile tile;
            tile.startCol = tx * colsPerTile;
            tile.endCol = std::min(patternWidth, tile.startCol + colsPerTile);
            tile.startRow = ty * rowsPerTile;
            tile.endRow = std::min(patternHeight, tile.startRow + rowsPerTile);
            tile.tileCol = tx;
            tile.tileRow = ty;
            layout.tiles.push_back(tile);
        }
    }

    return layout;
}

} // namespace ss::core::pdf
