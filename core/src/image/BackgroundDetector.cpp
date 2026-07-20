#include "ss/core/image/BackgroundDetector.h"

#include <array>

namespace ss::core::image {

QImage BackgroundDetector::removeSolidBackground(const QImage& image) {
    if (image.isNull() || image.width() < 1 || image.height() < 1) {
        return image;
    }

    QImage src = image;
    if (src.format() != QImage::Format_ARGB32) {
        src = src.convertToFormat(QImage::Format_ARGB32);
    }

    const int w = src.width();
    const int h = src.height();
    const std::array<QRgb, 4> corners = {
        src.pixel(0, 0),
        src.pixel(w - 1, 0),
        src.pixel(0, h - 1),
        src.pixel(w - 1, h - 1),
    };

    // Most frequent corner color wins; ties broken toward the earliest
    // (top-left first) by iterating in order and using a strict `>`.
    QRgb background = corners[0];
    int bestCount = 0;
    for (QRgb candidate : corners) {
        int count = 0;
        for (QRgb c : corners) {
            if ((c & 0x00FFFFFF) == (candidate & 0x00FFFFFF)) ++count;
        }
        if (count > bestCount) {
            bestCount = count;
            background = candidate;
        }
    }

    QImage result = src;
    for (int y = 0; y < h; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            if ((row[x] & 0x00FFFFFF) == (background & 0x00FFFFFF)) {
                row[x] = qRgba(0, 0, 0, 0);
            }
        }
    }
    return result;
}

} // namespace ss::core::image
