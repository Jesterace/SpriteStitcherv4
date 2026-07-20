#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"
#include "ss/core/project/ProjectSerializer.h"
#include "ss/core/quantize/ColorReducer.h"

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

} // namespace

int main() {
    QImage sprite(4, 4, QImage::Format_ARGB32);
    sprite.setPixel(0, 0, qRgb(0, 0, 0));
    sprite.setPixel(1, 0, qRgb(255, 255, 255));
    sprite.setPixel(2, 0, qRgb(0, 0, 0));
    sprite.setPixel(3, 0, qRgb(255, 255, 255));
    for (int y = 1; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            sprite.setPixel(x, y, qRgb(0, 0, 0));
        }
    }

    dmc::DmcTable table;
    dmc::DmcMatcher matcher(table);
    const auto reduction = quantize::ColorReducer::reduceExact(sprite);

    pattern::PatternModel original;
    original.buildFromReduction(reduction, matcher, "Round Trip Test");
    // Recolor one cell so the symbol map has to survive a non-trivial state.
    original.setCellColor(2, 2, "701");

    QTemporaryDir tmpDir;
    expect(tmpDir.isValid(), "could not create temp dir");
    const QString path = tmpDir.filePath("test.sstitch");

    QString error;
    const bool saved = project::ProjectSerializer::save(original, sprite, path, &error);
    expect(saved, qPrintable(QStringLiteral("save failed: %1").arg(error)));
    expect(QFile::exists(path), "project file was not created");

    auto loaded = project::ProjectSerializer::load(path, &error);
    expect(loaded.has_value(), qPrintable(QStringLiteral("load failed: %1").arg(error)));
    if (!loaded) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }

    expect(loaded->name == original.name(), "name did not round-trip");
    expect(loaded->width == original.width(), "width did not round-trip");
    expect(loaded->height == original.height(), "height did not round-trip");
    expect(!loaded->spriteImage.isNull(), "sprite image did not round-trip");
    expect(loaded->spriteImage.size() == sprite.size(), "sprite image size did not round-trip");

    pattern::PatternModel restored;
    restored.loadFromSaved(loaded->width, loaded->height, loaded->name, loaded->cellCodes, loaded->symbolMap);

    bool allCellsMatch = true;
    bool allSymbolsMatch = true;
    for (int y = 0; y < original.height(); ++y) {
        for (int x = 0; x < original.width(); ++x) {
            if (restored.cellAt(x, y).dmcCode != original.cellAt(x, y).dmcCode) allCellsMatch = false;
            if (restored.cellAt(x, y).symbol != original.cellAt(x, y).symbol) allSymbolsMatch = false;
        }
    }
    expect(allCellsMatch, "restored cell colors do not match the original pattern");
    expect(allSymbolsMatch, "restored symbols do not match the original pattern (symbol map not stable)");

    // Malformed / unsupported input must fail cleanly, not crash.
    {
        const QString badPath = tmpDir.filePath("not_json.sstitch");
        QFile bad(badPath);
        expect(bad.open(QIODevice::WriteOnly), "could not open malformed test file for writing");
        const qint64 written = bad.write("not actually json");
        expect(written > 0, "could not write malformed test file");
        bad.close();

        QString badError;
        auto badResult = project::ProjectSerializer::load(badPath, &badError);
        expect(!badResult.has_value(), "loading garbage data should fail");
        expect(!badError.isEmpty(), "failed load should report an error message");
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All project serializer tests passed.\n");
    return 0;
}
