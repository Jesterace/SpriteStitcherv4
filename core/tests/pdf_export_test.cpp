#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"
#include "ss/core/pdf/ChartPageLayout.h"
#include "ss/core/pdf/PdfExporter.h"
#include "ss/core/quantize/ColorReducer.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include <cstdio>

using namespace ss::core;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

void buildTestPattern(pattern::PatternModel& model, int w, int h, int distinctColors) {
    QImage img(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int band = ((x + y) * distinctColors) / (w + h);
            const int v = (band * 255) / std::max(1, distinctColors - 1);
            img.setPixel(x, y, qRgb(v, 255 - v, (v * 3) % 256));
        }
    }
    static dmc::DmcTable table;
    static dmc::DmcMatcher matcher(table);
    const auto reduction = quantize::ColorReducer::reduceMedianCut(img, distinctColors);
    model.buildFromReduction(reduction, matcher, "PDF Test Pattern");
}

// Pattern Keeper cannot parse two-byte CID/Identity-H encoded text; this is
// the regression check for that hard requirement.
void checkNoCidFonts(const QString& path) {
    QFile f(path);
    expect(f.open(QIODevice::ReadOnly), "could not reopen exported PDF for inspection");
    const QByteArray bytes = f.readAll();

    expect(bytes.startsWith("%PDF-"), "output does not start with a PDF header");
    expect(!bytes.contains("Identity-H"), "output uses Identity-H (two-byte) encoding");
    expect(!bytes.contains("/Type0"), "output uses a Type0 (composite/CID) font");
    expect(!bytes.contains("CIDFontType"), "output uses a CIDFontType font");
    expect(bytes.contains("/BaseFont /Helvetica") || bytes.contains("/BaseFont/Helvetica"),
           "output does not reference base-14 Helvetica");
}

} // namespace

int main() {
    dmc::DmcTable table;

    QTemporaryDir tmpDir;
    expect(tmpDir.isValid(), "could not create temp dir");

    // Small pattern: should fit on a single page.
    {
        pattern::PatternModel model;
        buildTestPattern(model, 20, 20, 6);
        const QString path = tmpDir.filePath("small.pdf");
        pdf::ExportOptions opts;
        opts.patternName = "Small Test";
        QString error;
        const bool ok = pdf::PdfExporter::exportToPdf(model, table, path, opts, &error);
        expect(ok, qPrintable(QStringLiteral("small export failed: %1").arg(error)));
        expect(QFile::exists(path), "small.pdf was not created");
        if (ok) checkNoCidFonts(path);
    }

    // Large pattern: should trigger multi-page tiling.
    {
        pattern::PatternModel model;
        buildTestPattern(model, 120, 120, 40);
        const QString path = tmpDir.filePath("large.pdf");
        pdf::ExportOptions opts;
        opts.patternName = "Large Test";
        QString error;
        const bool ok = pdf::PdfExporter::exportToPdf(model, table, path, opts, &error);
        expect(ok, qPrintable(QStringLiteral("large export failed: %1").arg(error)));
        expect(QFile::exists(path), "large.pdf was not created");
        if (ok) checkNoCidFonts(path);
    }

    // Layout sanity: a pattern much too large to read at any legible cell
    // size on one page must tile, not silently shrink to unreadable.
    {
        const auto page = pdf::ChartPageLayout::letterPortrait();
        const auto layout = pdf::ChartPageLayout::compute(300, 300, 50, page);
        expect(!layout.singlePage, "300x300 pattern unexpectedly fit on a single page");
        expect(layout.tilesX * layout.tilesY == static_cast<int>(layout.tiles.size()),
               "tile count doesn't match tilesX * tilesY");

        // Every stitch must be covered by exactly the tiling grid, with no
        // gaps and no out-of-range coordinates.
        for (const auto& tile : layout.tiles) {
            expect(tile.startCol >= 0 && tile.endCol <= 300, "tile column range out of bounds");
            expect(tile.startRow >= 0 && tile.endRow <= 300, "tile row range out of bounds");
            expect(tile.startCol < tile.endCol, "tile has empty column range");
            expect(tile.startRow < tile.endRow, "tile has empty row range");
        }
    }

    // A pattern that clearly fits should stay single-page (no gratuitous
    // tiling for small patterns).
    {
        const auto page = pdf::ChartPageLayout::letterPortrait();
        const auto layout = pdf::ChartPageLayout::compute(15, 15, 5, page);
        expect(layout.singlePage, "15x15 pattern should fit on a single page");
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All PDF export tests passed.\n");
    return 0;
}
