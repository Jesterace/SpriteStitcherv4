#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"
#include "ss/core/quantize/ColorReducer.h"

#include <QImage>

#include <cstdio>

using namespace ss::core;

int main() {
    int failures = 0;

    QImage img(2, 2, QImage::Format_ARGB32);
    img.setPixel(0, 0, qRgb(0, 0, 0));       // -> DMC 310 Black
    img.setPixel(1, 0, qRgb(255, 255, 255)); // -> DMC B5200
    img.setPixel(0, 1, qRgb(0, 0, 0));
    img.setPixel(1, 1, qRgb(255, 255, 255));

    const auto reduction = quantize::ColorReducer::reduceExact(img);
    dmc::DmcTable table;
    dmc::DmcMatcher matcher(table);

    pattern::PatternModel model;
    model.buildFromReduction(reduction, matcher, "test");

    if (model.width() != 2 || model.height() != 2) {
        std::fprintf(stderr, "FAIL: unexpected dimensions %dx%d\n", model.width(), model.height());
        ++failures;
    }

    int total = 0;
    for (int v : model.colorCounts()) total += v;
    if (total != 4) {
        std::fprintf(stderr, "FAIL: color counts sum to %d, expected 4\n", total);
        ++failures;
    }

    if (model.cellAt(0, 0).dmcCode != QStringLiteral("310")) {
        std::fprintf(stderr, "FAIL: expected (0,0) to be DMC 310, got %s\n",
                     qPrintable(model.cellAt(0, 0).dmcCode));
        ++failures;
    }
    if (model.cellAt(0, 0).symbol.isEmpty()) {
        std::fprintf(stderr, "FAIL: cell symbol was not assigned\n");
        ++failures;
    }

    model.swapColor(QStringLiteral("310"), QStringLiteral("801"));
    if (model.cellAt(0, 0).dmcCode != QStringLiteral("801")) {
        std::fprintf(stderr, "FAIL: swapColor did not update (0,0)\n");
        ++failures;
    }
    if (model.cellAt(0, 1).dmcCode != QStringLiteral("801")) {
        std::fprintf(stderr, "FAIL: swapColor did not update (0,1)\n");
        ++failures;
    }
    if (model.cellAt(1, 0).dmcCode == QStringLiteral("801")) {
        std::fprintf(stderr, "FAIL: swapColor incorrectly touched an unrelated cell\n");
        ++failures;
    }

    model.setCellColor(1, 1, QStringLiteral("700"));
    if (model.cellAt(1, 1).dmcCode != QStringLiteral("700") || model.cellAt(1, 1).symbol.isEmpty()) {
        std::fprintf(stderr, "FAIL: setCellColor did not update (1,1) properly\n");
        ++failures;
    }
    // The other pre-existing white cell must be untouched by a single-cell edit.
    if (model.cellAt(1, 0).dmcCode != QStringLiteral("B5200")) {
        std::fprintf(stderr, "FAIL: setCellColor incorrectly touched an unrelated cell\n");
        ++failures;
    }

    // Blank/unstitched cells: a transparent source pixel must produce an
    // empty dmcCode and no symbol, must be excluded from colorCounts(),
    // and must be counted by blankCellCount().
    {
        QImage withBackground(2, 2, QImage::Format_ARGB32);
        withBackground.setPixel(0, 0, qRgba(0, 0, 0, 255));   // -> DMC 310 Black
        withBackground.setPixel(1, 0, qRgba(0, 0, 0, 0));     // transparent -> blank
        withBackground.setPixel(0, 1, qRgba(0, 0, 0, 255));
        withBackground.setPixel(1, 1, qRgba(0, 0, 0, 0));

        const auto bgReduction = quantize::ColorReducer::reduceExact(withBackground);
        pattern::PatternModel bgModel;
        bgModel.buildFromReduction(bgReduction, matcher, "bg-test");

        if (!bgModel.cellAt(1, 0).dmcCode.isEmpty()) {
            std::fprintf(stderr, "FAIL blank: transparent pixel should produce an empty dmcCode, got %s\n",
                         qPrintable(bgModel.cellAt(1, 0).dmcCode));
            ++failures;
        }
        if (!bgModel.cellAt(1, 0).symbol.isEmpty()) {
            std::fprintf(stderr, "FAIL blank: blank cell should have no symbol\n");
            ++failures;
        }
        if (bgModel.blankCellCount() != 2) {
            std::fprintf(stderr, "FAIL blank: expected blankCellCount()==2, got %d\n", bgModel.blankCellCount());
            ++failures;
        }
        int bgTotal = 0;
        for (int v : bgModel.colorCounts()) bgTotal += v;
        if (bgTotal != 2) {
            std::fprintf(stderr, "FAIL blank: colorCounts() should exclude blanks (expected sum 2, got %d)\n", bgTotal);
            ++failures;
        }

        // Manual erase via setCellColor("") must also leave no symbol and
        // not register a bogus DMC code in the symbol map.
        bgModel.setCellColor(0, 0, QString());
        if (!bgModel.cellAt(0, 0).dmcCode.isEmpty() || !bgModel.cellAt(0, 0).symbol.isEmpty()) {
            std::fprintf(stderr, "FAIL blank: manual erase via setCellColor(\"\") did not blank the cell\n");
            ++failures;
        }
        if (bgModel.symbolMap().contains(QString())) {
            std::fprintf(stderr, "FAIL blank: symbol map should never contain an entry for the empty code\n");
            ++failures;
        }
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All pattern model tests passed.\n");
    return 0;
}
