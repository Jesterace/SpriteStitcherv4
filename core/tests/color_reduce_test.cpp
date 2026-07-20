#include "ss/core/quantize/ColorReducer.h"

#include <QImage>

#include <cstdio>

using namespace ss::core::quantize;

int main() {
    int failures = 0;

    // Exact mode: 2x2 image, 3 distinct colors (one repeated).
    QImage img(2, 2, QImage::Format_ARGB32);
    img.setPixel(0, 0, qRgb(255, 0, 0));
    img.setPixel(1, 0, qRgb(0, 255, 0));
    img.setPixel(0, 1, qRgb(255, 0, 0));
    img.setPixel(1, 1, qRgb(0, 0, 255));

    const auto exact = ColorReducer::reduceExact(img);
    if (exact.palette.size() != 3) {
        std::fprintf(stderr, "FAIL exact: expected 3 palette colors, got %zu\n", exact.palette.size());
        ++failures;
    }
    if (exact.paletteIndex[0] != exact.paletteIndex[2]) {
        std::fprintf(stderr, "FAIL exact: repeated color did not share a palette index\n");
        ++failures;
    }
    if (exact.paletteIndex[1] == exact.paletteIndex[3]) {
        std::fprintf(stderr, "FAIL exact: distinct colors incorrectly shared a palette index\n");
        ++failures;
    }

    // Median-cut: 100x1 grayscale gradient reduced to 4 colors.
    QImage grad(100, 1, QImage::Format_ARGB32);
    for (int x = 0; x < 100; ++x) {
        const int v = x * 255 / 99;
        grad.setPixel(x, 0, qRgb(v, v, v));
    }
    const auto reduced = ColorReducer::reduceMedianCut(grad, 4);
    if (reduced.palette.size() != 4) {
        std::fprintf(stderr, "FAIL median-cut: expected 4 palette colors, got %zu\n", reduced.palette.size());
        ++failures;
    }
    for (int idx : reduced.paletteIndex) {
        if (idx < 0 || idx >= static_cast<int>(reduced.palette.size())) {
            std::fprintf(stderr, "FAIL median-cut: out-of-range palette index %d\n", idx);
            ++failures;
            break;
        }
    }

    // Requesting more colors than exist in the source should just yield the
    // exact count, not pad or crash.
    const auto smallExact = ColorReducer::reduceMedianCut(img, 100);
    if (smallExact.palette.size() != 3) {
        std::fprintf(stderr, "FAIL median-cut: expected 3 palette colors when target exceeds source variety, got %zu\n",
                     smallExact.palette.size());
        ++failures;
    }

    // Transparent pixels are background, not a color: excluded from the
    // palette entirely and marked with kBlankIndex.
    {
        QImage withAlpha(2, 2, QImage::Format_ARGB32);
        withAlpha.setPixel(0, 0, qRgba(255, 0, 0, 255));
        withAlpha.setPixel(1, 0, qRgba(0, 0, 0, 0));   // fully transparent
        withAlpha.setPixel(0, 1, qRgba(0, 255, 0, 255));
        withAlpha.setPixel(1, 1, qRgba(10, 10, 10, 5)); // near-fully transparent

        const auto result = ColorReducer::reduceExact(withAlpha);
        if (result.palette.size() != 2) {
            std::fprintf(stderr, "FAIL alpha/exact: expected 2 palette colors (transparent pixels excluded), got %zu\n",
                         result.palette.size());
            ++failures;
        }
        if (result.paletteIndex[1] != ReductionResult::kBlankIndex) {
            std::fprintf(stderr, "FAIL alpha/exact: fully transparent pixel was not marked blank\n");
            ++failures;
        }
        if (result.paletteIndex[3] != ReductionResult::kBlankIndex) {
            std::fprintf(stderr, "FAIL alpha/exact: near-transparent pixel was not marked blank\n");
            ++failures;
        }
        if (result.paletteIndex[0] == ReductionResult::kBlankIndex || result.paletteIndex[2] == ReductionResult::kBlankIndex) {
            std::fprintf(stderr, "FAIL alpha/exact: opaque pixel incorrectly marked blank\n");
            ++failures;
        }

        const auto medianResult = ColorReducer::reduceMedianCut(withAlpha, 4);
        if (medianResult.palette.size() != 2) {
            std::fprintf(stderr, "FAIL alpha/median-cut: expected 2 palette colors, got %zu\n",
                         medianResult.palette.size());
            ++failures;
        }
        if (medianResult.paletteIndex[1] != ReductionResult::kBlankIndex ||
            medianResult.paletteIndex[3] != ReductionResult::kBlankIndex) {
            std::fprintf(stderr, "FAIL alpha/median-cut: transparent pixels were not marked blank\n");
            ++failures;
        }
    }

    // All-transparent source shouldn't crash and should mark every pixel
    // blank with an empty (or harmlessly unused) palette.
    {
        QImage allTransparent(3, 3, QImage::Format_ARGB32);
        allTransparent.fill(qRgba(0, 0, 0, 0));
        const auto result = ColorReducer::reduceMedianCut(allTransparent, 8);
        for (int idx : result.paletteIndex) {
            if (idx != ReductionResult::kBlankIndex) {
                std::fprintf(stderr, "FAIL all-transparent: expected every pixel blank, got index %d\n", idx);
                ++failures;
                break;
            }
        }
    }

    if (failures) {
        std::fprintf(stderr, "%d failures\n", failures);
        return 1;
    }
    std::printf("All color reduction tests passed.\n");
    return 0;
}
