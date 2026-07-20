#pragma once

#include <QImage>
#include <QRgb>

#include <vector>

namespace ss::core::quantize {

// Result of reducing a source image to a small palette. `paletteIndex` maps
// 1:1 onto the source image's pixels (row-major, width*height entries) so
// callers can rebuild a preview or feed straight into DMC matching without
// re-scanning the image. A value of kBlankIndex means the source pixel was
// (near-)fully transparent and should not be stitched at all — it's not a
// color, so it's deliberately excluded from `palette`.
struct ReductionResult {
    static constexpr int kBlankIndex = -1;

    int width = 0;
    int height = 0;
    std::vector<QRgb> palette;
    std::vector<int> paletteIndex;
};

class ColorReducer {
public:
    // 1:1 mapping — every distinct source color becomes its own palette
    // entry. Appropriate for pixel art that's already low-color.
    static ReductionResult reduceExact(const QImage& image);

    // Median-cut quantization to at most `targetColors` palette entries.
    // Deterministic (no RNG), suitable for photographic or noisy source art.
    static ReductionResult reduceMedianCut(const QImage& image, int targetColors);
};

} // namespace ss::core::quantize
