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

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All pattern model tests passed.\n");
    return 0;
}
