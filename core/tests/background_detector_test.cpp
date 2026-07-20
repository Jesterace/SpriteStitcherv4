#include "ss/core/image/BackgroundDetector.h"

#include <QImage>

#include <cstdio>

using namespace ss::core::image;

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
    // Solid white background with a black+red sprite in the middle.
    {
        QImage img(4, 4, QImage::Format_ARGB32);
        img.fill(qRgba(255, 255, 255, 255));
        img.setPixel(1, 1, qRgba(0, 0, 0, 255));
        img.setPixel(2, 1, qRgba(200, 0, 0, 255));
        img.setPixel(1, 2, qRgba(200, 0, 0, 255));
        img.setPixel(2, 2, qRgba(0, 0, 0, 255));

        const QImage result = BackgroundDetector::removeSolidBackground(img);

        expect(qAlpha(result.pixel(0, 0)) == 0, "corner background pixel should become transparent");
        expect(qAlpha(result.pixel(3, 3)) == 0, "opposite corner background pixel should become transparent");
        expect(qAlpha(result.pixel(0, 2)) == 0, "edge background pixel should become transparent");
        expect(qAlpha(result.pixel(1, 1)) == 255, "sprite pixel (black) must stay opaque");
        expect(qAlpha(result.pixel(2, 1)) == 255, "sprite pixel (red) must stay opaque");
        expect((result.pixel(1, 1) & 0x00FFFFFF) == 0x000000, "sprite pixel color must be unchanged");

        // Original image must be untouched (caller may still need it, e.g.
        // for the sprite reference panel).
        expect(qAlpha(img.pixel(0, 0)) == 255, "removeSolidBackground must not mutate its input");
    }

    // Background color that happens to differ per corner (no clean
    // majority except via the tie-break toward the first/top-left corner)
    // shouldn't crash; whichever color it treats as background, the
    // result must still be well-formed (same size, valid alpha values).
    {
        QImage img(2, 2, QImage::Format_ARGB32);
        img.setPixel(0, 0, qRgba(255, 0, 0, 255));
        img.setPixel(1, 0, qRgba(0, 255, 0, 255));
        img.setPixel(0, 1, qRgba(0, 0, 255, 255));
        img.setPixel(1, 1, qRgba(255, 255, 0, 255));

        const QImage result = BackgroundDetector::removeSolidBackground(img);
        expect(result.size() == img.size(), "result size must match input size for a no-majority image");
        // The top-left corner's own color is always the tie-break winner,
        // so it must always end up transparent.
        expect(qAlpha(result.pixel(0, 0)) == 0, "tie-break should treat the top-left corner as background");
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All background detector tests passed.\n");
    return 0;
}
