#pragma once

#include <QImage>
#include <QString>

#include <optional>

namespace ss::core::image {

// Loads sprite art from disk and normalizes it to ARGB32 for downstream
// color reduction / DMC matching. Supports whatever Qt's plugin set
// provides (PNG/JPEG/BMP at minimum).
class ImageLoader {
public:
    struct Result {
        QImage image;
    };

    static std::optional<Result> load(const QString& path, QString* errorOut = nullptr);
};

} // namespace ss::core::image
