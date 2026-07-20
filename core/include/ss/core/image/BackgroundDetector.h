#pragma once

#include <QImage>

namespace ss::core::image {

// Support for the common case where a sprite was exported on a flat
// canvas color instead of with real alpha transparency: sample the
// background color from the image's corners, then hand back a copy with
// every matching pixel made fully transparent so it flows through the
// same "blank cell" path as genuinely transparent PNGs
// (see quantize::ReductionResult::kBlankIndex).
class BackgroundDetector {
public:
    // Samples the four corner pixels; the most common corner color
    // (ties broken toward the top-left corner) is taken as the
    // background. Every pixel matching that color exactly is made fully
    // transparent (alpha = 0) in the returned copy. `image` itself is
    // left untouched — callers that also need the original for display
    // (e.g. a sprite reference panel) can keep using it.
    static QImage removeSolidBackground(const QImage& image);
};

} // namespace ss::core::image
