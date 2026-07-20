#include "ss/core/project/ProjectSerializer.h"

#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace ss::core::project {

namespace {
constexpr int kFormatVersion = 1;
}

bool ProjectSerializer::save(const pattern::PatternModel& pattern, const QImage& spriteImage,
                              const QString& path, QString* errorOut) {
    QJsonObject root;
    root["formatVersion"] = kFormatVersion;
    root["name"] = pattern.name();
    root["width"] = pattern.width();
    root["height"] = pattern.height();

    QJsonArray cells;
    for (int y = 0; y < pattern.height(); ++y) {
        for (int x = 0; x < pattern.width(); ++x) {
            cells.append(pattern.cellAt(x, y).dmcCode);
        }
    }
    root["cells"] = cells;

    QJsonObject symbolMapObj;
    const auto& symbolMap = pattern.symbolMap();
    for (auto it = symbolMap.constBegin(); it != symbolMap.constEnd(); ++it) {
        symbolMapObj[it.key()] = it.value();
    }
    root["symbolMap"] = symbolMapObj;

    if (!spriteImage.isNull()) {
        QByteArray imageBytes;
        QBuffer buffer(&imageBytes);
        buffer.open(QIODevice::WriteOnly);
        spriteImage.save(&buffer, "PNG");

        QJsonObject spriteObj;
        spriteObj["format"] = "png";
        spriteObj["data"] = QString::fromLatin1(imageBytes.toBase64());
        root["spriteImage"] = spriteObj;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut) *errorOut = QStringLiteral("Could not open %1 for writing: %2").arg(path, file.errorString());
        return false;
    }

    const QJsonDocument doc(root);
    const qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    if (written < 0) {
        if (errorOut) *errorOut = QStringLiteral("Failed to write %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

std::optional<ProjectSerializer::LoadResult> ProjectSerializer::load(const QString& path, QString* errorOut) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = QStringLiteral("Could not open %1: %2").arg(path, file.errorString());
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (errorOut) *errorOut = QStringLiteral("Not a valid project file: %1").arg(parseError.errorString());
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    const int formatVersion = root.value("formatVersion").toInt(-1);
    if (formatVersion != kFormatVersion) {
        if (errorOut) {
            *errorOut = QStringLiteral("Unsupported project file version (%1)").arg(formatVersion);
        }
        return std::nullopt;
    }

    LoadResult result;
    result.name = root.value("name").toString();
    result.width = root.value("width").toInt();
    result.height = root.value("height").toInt();

    if (result.width <= 0 || result.height <= 0) {
        if (errorOut) *errorOut = QStringLiteral("Project file has invalid dimensions");
        return std::nullopt;
    }

    const QJsonArray cells = root.value("cells").toArray();
    if (cells.size() != static_cast<qsizetype>(result.width) * result.height) {
        if (errorOut) *errorOut = QStringLiteral("Project file is corrupt: cell count does not match dimensions");
        return std::nullopt;
    }
    result.cellCodes.reserve(static_cast<size_t>(cells.size()));
    for (const auto& v : cells) {
        result.cellCodes.push_back(v.toString());
    }

    const QJsonObject symbolMapObj = root.value("symbolMap").toObject();
    for (auto it = symbolMapObj.constBegin(); it != symbolMapObj.constEnd(); ++it) {
        result.symbolMap.insert(it.key(), it.value().toString());
    }

    if (root.contains("spriteImage")) {
        const QJsonObject spriteObj = root.value("spriteImage").toObject();
        const QByteArray imageBytes = QByteArray::fromBase64(spriteObj.value("data").toString().toLatin1());
        result.spriteImage.loadFromData(imageBytes, "PNG");
    }

    return result;
}

} // namespace ss::core::project
