#include "ss/core/image/ImageLoader.h"

namespace ss::core::image {

std::optional<ImageLoader::Result> ImageLoader::load(const QString& path, QString* errorOut) {
    QImage img(path);
    if (img.isNull()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Could not read image: %1").arg(path);
        }
        return std::nullopt;
    }

    if (img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }

    Result result;
    result.image = std::move(img);
    return result;
}

} // namespace ss::core::image
