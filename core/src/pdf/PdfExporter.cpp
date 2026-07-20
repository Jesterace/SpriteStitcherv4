#include "ss/core/pdf/PdfExporter.h"

#include "HpdfDocument.h"
#include "ss/core/pdf/ChartPageLayout.h"

#include <hpdf.h>
#include <hpdf_encoder.h>

#include <QFile>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

namespace ss::core::pdf {

namespace {

// Numeric-aware compare so "9" sorts before "10"; falls back to plain
// string comparison for lettered codes (White, Ecru, B5200, ...).
bool dmcCodeLessThan(const QString& a, const QString& b) {
    static const QRegularExpression numRe(QStringLiteral("^\\d+$"));
    const bool aNum = numRe.match(a).hasMatch();
    const bool bNum = numRe.match(b).hasMatch();
    if (aNum && bNum) return a.toInt() < b.toInt();
    if (aNum != bNum) return aNum; // numeric codes sort before lettered ones
    return a < b;
}

// HPDF_Page_TextOut does not implicitly wrap BeginText/EndText on this
// libharu build (confirmed by direct testing: calling it outside an active
// text object raises HPDF_PAGE_INVALID_GMODE), so every text draw goes
// through this wrapper rather than the raw API call.
void drawText(HPDF_Page page, double x, double y, const char* text) {
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, x, y, text);
    HPDF_Page_EndText(page);
}

double toPdfY(double topDownY, double pageHeight) { return pageHeight - topDownY; }

double rectBottomY(double topDownTop, double height, double pageHeight) {
    return pageHeight - topDownTop - height;
}

struct RGB {
    double r, g, b;
};

RGB colorForCode(const dmc::DmcTable& table, const QString& code) {
    if (auto c = table.findByCode(code.toStdString())) {
        return {c->r / 255.0, c->g / 255.0, c->b / 255.0};
    }
    return {0.82, 0.82, 0.82};
}

bool isLight(const RGB& c) {
    return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) > 0.6;
}

void drawHeader(HPDF_Page page, HPDF_Font regular, HPDF_Font bold, double pageHeight,
                double left, double top, const pattern::PatternModel& pattern,
                const ExportOptions& options, int totalStitches, int colorCount) {
    double cursorY = top;

    HPDF_Page_SetRGBFill(page, 0, 0, 0);
    HPDF_Page_SetFontAndSize(page, bold, 16);
    const QString title = options.patternName.isEmpty() ? QStringLiteral("Untitled Pattern") : options.patternName;
    drawText(page, left, toPdfY(cursorY + 14, pageHeight), title.toLatin1().constData());
    cursorY += 20;

    HPDF_Page_SetFontAndSize(page, regular, 10);
    const QString dims = QStringLiteral("%1 x %2 stitches   %3 total stitches   %4 colors")
                              .arg(pattern.width())
                              .arg(pattern.height())
                              .arg(totalStitches)
                              .arg(colorCount);
    drawText(page, left, toPdfY(cursorY + 10, pageHeight), dims.toLatin1().constData());
    cursorY += 16;

    QStringList fabricParts;
    for (int count : options.fabricCounts) {
        if (count <= 0) continue;
        const double wIn = double(pattern.width()) / count;
        const double hIn = double(pattern.height()) / count;
        fabricParts << QStringLiteral("%1ct: %2\" x %3\"").arg(count).arg(wIn, 0, 'f', 1).arg(hIn, 0, 'f', 1);
    }
    const QString fabricLine = fabricParts.join(QStringLiteral("   "));
    drawText(page, left, toPdfY(cursorY + 10, pageHeight), fabricLine.toLatin1().constData());
}

void drawLegend(HPDF_Page page, HPDF_Font regular, HPDF_Font bold, const dmc::DmcTable& table,
                 double pageHeight, double left, double top, double width,
                 const std::vector<QString>& sortedCodes, const QMap<QString, int>& counts,
                 const QMap<QString, QString>& symbols, int columns, int rowsPerColumn) {
    HPDF_Page_SetRGBFill(page, 0, 0, 0);
    HPDF_Page_SetFontAndSize(page, bold, 10);
    drawText(page, left, toPdfY(top + 10, pageHeight), "DMC Legend");

    const double rowTop0 = top + ChartPageLayout::kLegendTitleHeightPt;
    const double columnWidth = columns > 0 ? width / columns : width;
    const double swatchSize = 9.0;

    for (int i = 0; i < static_cast<int>(sortedCodes.size()); ++i) {
        const QString& code = sortedCodes[static_cast<size_t>(i)];
        const int col = rowsPerColumn > 0 ? i / rowsPerColumn : 0;
        const int row = rowsPerColumn > 0 ? i % rowsPerColumn : i;
        const double colX = left + col * columnWidth;
        const double rowY = rowTop0 + row * ChartPageLayout::kLegendRowHeightPt;

        const RGB rgb = colorForCode(table, code);
        const double swatchBottom = rectBottomY(rowY, swatchSize, pageHeight);
        HPDF_Page_SetRGBFill(page, rgb.r, rgb.g, rgb.b);
        HPDF_Page_Rectangle(page, colX, swatchBottom, swatchSize, swatchSize);
        HPDF_Page_Fill(page);
        HPDF_Page_SetRGBStroke(page, 0.5, 0.5, 0.5);
        HPDF_Page_SetLineWidth(page, 0.5);
        HPDF_Page_Rectangle(page, colX, swatchBottom, swatchSize, swatchSize);
        HPDF_Page_Stroke(page);

        HPDF_Page_SetRGBFill(page, 0, 0, 0);
        HPDF_Page_SetFontAndSize(page, regular, 8);
        const QString label =
            QStringLiteral("%1  %2  x%3").arg(symbols.value(code), code).arg(counts.value(code));
        drawText(page, colX + swatchSize + 4, toPdfY(rowY + 8, pageHeight), label.toLatin1().constData());
    }
}

void drawChartTile(HPDF_Page page, HPDF_Font regular, const dmc::DmcTable& table,
                    const pattern::PatternModel& pattern, const Tile& tile, double cellPt,
                    double pageHeight, double left, double top) {
    const int cols = tile.endCol - tile.startCol;
    const int rows = tile.endRow - tile.startRow;
    const double chartBottom = rectBottomY(top, rows * cellPt, pageHeight);

    for (int ry = 0; ry < rows; ++ry) {
        const int y = tile.startRow + ry;
        for (int rx = 0; rx < cols; ++rx) {
            const int x = tile.startCol + rx;
            const auto& cell = pattern.cellAt(x, y);
            const RGB rgb = colorForCode(table, cell.dmcCode);
            const double cellTopDown = top + ry * cellPt;
            const double cellLeft = left + rx * cellPt;

            HPDF_Page_SetRGBFill(page, rgb.r, rgb.g, rgb.b);
            HPDF_Page_Rectangle(page, cellLeft, rectBottomY(cellTopDown, cellPt, pageHeight), cellPt, cellPt);
            HPDF_Page_Fill(page);

            if (cellPt >= 8.0 && !cell.symbol.isEmpty()) {
                const double fontSize = cellPt * 0.6;
                HPDF_Page_SetFontAndSize(page, regular, fontSize);
                const QByteArray sym = cell.symbol.toLatin1();
                const double textWidth = HPDF_Page_TextWidth(page, sym.constData());
                const double textX = cellLeft + (cellPt - textWidth) / 2.0;
                const double textTopDown = cellTopDown + cellPt * 0.72;

                if (isLight(rgb)) HPDF_Page_SetRGBFill(page, 0, 0, 0);
                else HPDF_Page_SetRGBFill(page, 1, 1, 1);
                drawText(page, textX, toPdfY(textTopDown, pageHeight), sym.constData());
            }
        }
    }

    HPDF_Page_SetRGBStroke(page, 0.8, 0.8, 0.8);
    HPDF_Page_SetLineWidth(page, 0.4);
    for (int rx = 0; rx <= cols; ++rx) {
        const double lx = left + rx * cellPt;
        HPDF_Page_MoveTo(page, lx, chartBottom);
        HPDF_Page_LineTo(page, lx, chartBottom + rows * cellPt);
        HPDF_Page_Stroke(page);
    }
    for (int ry = 0; ry <= rows; ++ry) {
        const double pdfLy = toPdfY(top + ry * cellPt, pageHeight);
        HPDF_Page_MoveTo(page, left, pdfLy);
        HPDF_Page_LineTo(page, left + cols * cellPt, pdfLy);
        HPDF_Page_Stroke(page);
    }

    // Red center lines, both axes — a grid-line coordinate (possibly
    // fractional for odd dimensions), matching GridView's on-screen math so
    // the printed chart and the live view always agree.
    const double centerColLine = pattern.width() / 2.0;
    const double centerRowLine = pattern.height() / 2.0;
    HPDF_Page_SetRGBStroke(page, 0.78, 0.12, 0.12);
    HPDF_Page_SetLineWidth(page, 1.2);
    if (centerColLine >= tile.startCol && centerColLine <= tile.endCol) {
        const double lx = left + (centerColLine - tile.startCol) * cellPt;
        HPDF_Page_MoveTo(page, lx, chartBottom);
        HPDF_Page_LineTo(page, lx, chartBottom + rows * cellPt);
        HPDF_Page_Stroke(page);
    }
    if (centerRowLine >= tile.startRow && centerRowLine <= tile.endRow) {
        const double ly = toPdfY(top + (centerRowLine - tile.startRow) * cellPt, pageHeight);
        HPDF_Page_MoveTo(page, left, ly);
        HPDF_Page_LineTo(page, left + cols * cellPt, ly);
        HPDF_Page_Stroke(page);
    }

    HPDF_Page_SetRGBFill(page, 0, 0, 0);
    HPDF_Page_SetFontAndSize(page, regular, 7);
    for (int x = tile.startCol; x <= tile.endCol; ++x) {
        if (x % 10 != 0) continue;
        const double lx = left + (x - tile.startCol) * cellPt;
        const QByteArray label = QByteArray::number(x);
        const double tw = HPDF_Page_TextWidth(page, label.constData());
        drawText(page, lx - tw / 2.0, toPdfY(top - 6, pageHeight), label.constData());
    }
    for (int y = tile.startRow; y <= tile.endRow; ++y) {
        if (y % 10 != 0) continue;
        const double ly = top + (y - tile.startRow) * cellPt;
        const QByteArray label = QByteArray::number(y);
        const double tw = HPDF_Page_TextWidth(page, label.constData());
        drawText(page, left - tw - 4, toPdfY(ly + 3, pageHeight), label.constData());
    }
}

void drawTileCaption(HPDF_Page page, HPDF_Font bold, double pageHeight, double left, double top,
                      const Tile& tile, int tileIndex, int tileCount) {
    HPDF_Page_SetRGBFill(page, 0, 0, 0);
    HPDF_Page_SetFontAndSize(page, bold, 11);
    const QString caption = QStringLiteral("Page %1 of %2 -- columns %3-%4, rows %5-%6")
                                 .arg(tileIndex)
                                 .arg(tileCount)
                                 .arg(tile.startCol + 1)
                                 .arg(tile.endCol)
                                 .arg(tile.startRow + 1)
                                 .arg(tile.endRow);
    drawText(page, left, toPdfY(top + 12, pageHeight), caption.toLatin1().constData());
}

} // namespace

bool PdfExporter::exportToPdf(const pattern::PatternModel& pattern, const dmc::DmcTable& table,
                               const QString& outputPath, const ExportOptions& options, QString* errorOut) {
    auto fail = [&](const QString& msg) {
        if (errorOut) *errorOut = msg;
        return false;
    };

    if (pattern.width() <= 0 || pattern.height() <= 0) {
        return fail(QStringLiteral("Pattern is empty"));
    }

    HpdfDocument doc;
    if (doc.hasError()) return fail(doc.errorMessage());

    HPDF_Doc pdf = doc.handle();
    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);

    // Base-14 Helvetica with a single-byte standard encoding: never a
    // CID/Identity-H (two-byte) font path, which is what Pattern Keeper
    // cannot parse. This is the one rule this whole file exists to uphold.
    HPDF_Font regular = HPDF_GetFont(pdf, "Helvetica", HPDF_ENCODING_STANDARD);
    HPDF_Font bold = HPDF_GetFont(pdf, "Helvetica-Bold", HPDF_ENCODING_STANDARD);
    if (doc.hasError() || !regular || !bold) {
        return fail(doc.hasError() ? doc.errorMessage() : QStringLiteral("Failed to load base-14 Helvetica fonts"));
    }

    const auto counts = pattern.colorCounts();
    const auto& symbols = pattern.symbolMap();
    std::vector<QString> sortedCodes(counts.keyBegin(), counts.keyEnd());
    std::sort(sortedCodes.begin(), sortedCodes.end(), dmcCodeLessThan);

    int totalStitches = 0;
    for (int c : counts) totalStitches += c;

    const PageGeometry page = ChartPageLayout::letterPortrait();
    const double marginPt = 36.0;
    const ChartLayout layout = ChartPageLayout::compute(pattern.width(), pattern.height(),
                                                          static_cast<int>(sortedCodes.size()), page);

    const double contentLeft = marginPt;
    const double contentTop = marginPt;
    const double contentWidth = page.widthPt - 2.0 * marginPt;

    if (layout.tiles.empty()) {
        return fail(QStringLiteral("Failed to compute chart layout"));
    }

    if (layout.singlePage) {
        HPDF_Page pg = HPDF_AddPage(pdf);
        HPDF_Page_SetSize(pg, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

        drawHeader(pg, regular, bold, page.heightPt, contentLeft, contentTop, pattern, options, totalStitches,
                   static_cast<int>(sortedCodes.size()));

        const double chartTop = contentTop + ChartPageLayout::kHeaderHeightPt + ChartPageLayout::kRulerMarginPt;
        const double chartLeft = contentLeft + ChartPageLayout::kRulerMarginPt;
        drawChartTile(pg, regular, table, pattern, layout.tiles[0], layout.cellPt, page.heightPt, chartLeft, chartTop);

        const double legendTop = chartTop + pattern.height() * layout.cellPt + 10.0;
        drawLegend(pg, regular, bold, table, page.heightPt, contentLeft, legendTop, contentWidth, sortedCodes,
                   counts, symbols, layout.legendColumns, layout.legendRowsPerColumn);
    } else {
        HPDF_Page infoPage = HPDF_AddPage(pdf);
        HPDF_Page_SetSize(infoPage, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
        drawHeader(infoPage, regular, bold, page.heightPt, contentLeft, contentTop, pattern, options, totalStitches,
                   static_cast<int>(sortedCodes.size()));

        HPDF_Page_SetFontAndSize(infoPage, regular, 9);
        HPDF_Page_SetRGBFill(infoPage, 0, 0, 0);
        const QString tileNote = QStringLiteral("Chart spans %1 pages (%2 x %3 grid).")
                                      .arg(layout.tiles.size())
                                      .arg(layout.tilesX)
                                      .arg(layout.tilesY);
        drawText(infoPage, contentLeft,
                           toPdfY(contentTop + ChartPageLayout::kInfoHeaderHeightPt, page.heightPt),
                           tileNote.toLatin1().constData());

        const double legendTop = contentTop + ChartPageLayout::kInfoHeaderHeightPt + 16.0;
        drawLegend(infoPage, regular, bold, table, page.heightPt, contentLeft, legendTop, contentWidth, sortedCodes,
                   counts, symbols, layout.legendColumns, layout.legendRowsPerColumn);

        int tileIndex = 1;
        for (const Tile& tile : layout.tiles) {
            HPDF_Page tp = HPDF_AddPage(pdf);
            HPDF_Page_SetSize(tp, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
            drawTileCaption(tp, bold, page.heightPt, contentLeft, contentTop, tile, tileIndex,
                             static_cast<int>(layout.tiles.size()));

            const double chartTop = contentTop + ChartPageLayout::kTileCaptionHeightPt + ChartPageLayout::kRulerMarginPt;
            const double chartLeft = contentLeft + ChartPageLayout::kRulerMarginPt;
            drawChartTile(tp, regular, table, pattern, tile, layout.cellPt, page.heightPt, chartLeft, chartTop);
            ++tileIndex;
        }
    }

    if (doc.hasError()) {
        return fail(doc.errorMessage());
    }

    if (!doc.saveToFile(outputPath)) {
        QFile::remove(outputPath);
        return fail(doc.hasError() ? doc.errorMessage() : QStringLiteral("Failed to write PDF file"));
    }

    return true;
}

} // namespace ss::core::pdf
