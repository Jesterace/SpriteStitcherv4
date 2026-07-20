#include "ss/core/quantize/ColorReducer.h"

#include <algorithm>
#include <unordered_map>

namespace ss::core::quantize {

namespace {

struct ColorCount {
    QRgb color;
    int count;
};

struct Box {
    std::vector<ColorCount> colors; // entries from the histogram belonging to this box
    long long totalPixels = 0;

    void recompute() {
        totalPixels = 0;
        for (const auto& c : colors) totalPixels += c.count;
    }

    // Inclusive channel ranges across the colors currently in the box.
    void channelRanges(int& rMin, int& rMax, int& gMin, int& gMax, int& bMin, int& bMax) const {
        rMin = gMin = bMin = 256;
        rMax = gMax = bMax = -1;
        for (const auto& c : colors) {
            const int r = qRed(c.color), g = qGreen(c.color), b = qBlue(c.color);
            rMin = std::min(rMin, r); rMax = std::max(rMax, r);
            gMin = std::min(gMin, g); gMax = std::max(gMax, g);
            bMin = std::min(bMin, b); bMax = std::max(bMax, b);
        }
    }
};

QRgb averageColor(const Box& box) {
    long long r = 0, g = 0, b = 0, total = 0;
    for (const auto& c : box.colors) {
        r += static_cast<long long>(qRed(c.color)) * c.count;
        g += static_cast<long long>(qGreen(c.color)) * c.count;
        b += static_cast<long long>(qBlue(c.color)) * c.count;
        total += c.count;
    }
    if (total == 0) return qRgb(0, 0, 0);
    return qRgb(static_cast<int>(r / total), static_cast<int>(g / total), static_cast<int>(b / total));
}

// Splits `box` in place along its longest channel axis at the weighted
// median, returning the new second box. Assumes box.colors.size() > 1.
Box splitBox(Box& box) {
    int rMin, rMax, gMin, gMax, bMin, bMax;
    box.channelRanges(rMin, rMax, gMin, gMax, bMin, bMax);
    const int rRange = rMax - rMin, gRange = gMax - gMin, bRange = bMax - bMin;

    auto channelOf = [&](QRgb c) {
        if (rRange >= gRange && rRange >= bRange) return qRed(c);
        if (gRange >= rRange && gRange >= bRange) return qGreen(c);
        return qBlue(c);
    };

    std::sort(box.colors.begin(), box.colors.end(), [&](const ColorCount& a, const ColorCount& b2) {
        return channelOf(a.color) < channelOf(b2.color);
    });

    box.recompute();
    const long long half = box.totalPixels / 2;

    long long running = 0;
    size_t splitIdx = 0;
    for (size_t i = 0; i < box.colors.size(); ++i) {
        running += box.colors[i].count;
        if (running >= half) {
            splitIdx = i + 1;
            break;
        }
    }
    splitIdx = std::clamp(splitIdx, size_t{1}, box.colors.size() - 1);

    Box second;
    second.colors.assign(box.colors.begin() + static_cast<long>(splitIdx), box.colors.end());
    box.colors.resize(splitIdx);

    box.recompute();
    second.recompute();
    return second;
}

std::unordered_map<QRgb, int> buildHistogram(const QImage& image) {
    std::unordered_map<QRgb, int> hist;
    hist.reserve(1024);
    for (int y = 0; y < image.height(); ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            hist[row[x] | 0xFF000000]++; // ignore alpha for palette purposes
        }
    }
    return hist;
}

} // namespace

ReductionResult ColorReducer::reduceExact(const QImage& image) {
    ReductionResult result;
    result.width = image.width();
    result.height = image.height();
    result.paletteIndex.resize(static_cast<size_t>(result.width) * result.height);

    std::unordered_map<QRgb, int> colorToIndex;
    colorToIndex.reserve(1024);

    for (int y = 0; y < image.height(); ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb color = row[x] | 0xFF000000;
            auto it = colorToIndex.find(color);
            int idx;
            if (it == colorToIndex.end()) {
                idx = static_cast<int>(result.palette.size());
                result.palette.push_back(color);
                colorToIndex.emplace(color, idx);
            } else {
                idx = it->second;
            }
            result.paletteIndex[static_cast<size_t>(y) * result.width + x] = idx;
        }
    }
    return result;
}

ReductionResult ColorReducer::reduceMedianCut(const QImage& image, int targetColors) {
    ReductionResult result;
    result.width = image.width();
    result.height = image.height();
    result.paletteIndex.resize(static_cast<size_t>(result.width) * result.height);

    const auto hist = buildHistogram(image);

    if (targetColors < 1) targetColors = 1;

    Box root;
    root.colors.reserve(hist.size());
    for (const auto& [color, count] : hist) {
        root.colors.push_back({color, count});
    }
    root.recompute();

    std::vector<Box> boxes;
    boxes.push_back(std::move(root));

    while (static_cast<int>(boxes.size()) < targetColors) {
        // Split the box with the most distinct colors (the one splitting is
        // still meaningful for); skip boxes that can't be split further.
        auto it = std::max_element(boxes.begin(), boxes.end(), [](const Box& a, const Box& b) {
            return a.colors.size() < b.colors.size();
        });
        if (it == boxes.end() || it->colors.size() <= 1) break;

        Box second = splitBox(*it);
        boxes.push_back(std::move(second));
    }

    result.palette.reserve(boxes.size());
    std::unordered_map<QRgb, int> colorToIndex;
    colorToIndex.reserve(hist.size());

    for (size_t boxIdx = 0; boxIdx < boxes.size(); ++boxIdx) {
        result.palette.push_back(averageColor(boxes[boxIdx]));
        for (const auto& c : boxes[boxIdx].colors) {
            colorToIndex[c.color] = static_cast<int>(boxIdx);
        }
    }

    for (int y = 0; y < image.height(); ++y) {
        const QRgb* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb color = row[x] | 0xFF000000;
            result.paletteIndex[static_cast<size_t>(y) * result.width + x] = colorToIndex.at(color);
        }
    }

    return result;
}

} // namespace ss::core::quantize
