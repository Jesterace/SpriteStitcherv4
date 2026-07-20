#pragma once

#include "ss/core/pattern/PatternModel.h"

#include <QImage>
#include <QMap>
#include <QString>

#include <optional>
#include <vector>

namespace ss::core::project {

// Native project format: a single JSON file with the full pattern state
// (grid, symbol assignments) plus the originally imported sprite image
// (embedded as base64 PNG), so a project reopens exactly as it was left —
// including the sprite reference panel. Entirely separate from PDF export.
class ProjectSerializer {
public:
    struct LoadResult {
        QString name;
        int width = 0;
        int height = 0;
        std::vector<QString> cellCodes; // row-major, width*height entries
        QMap<QString, QString> symbolMap;
        QImage spriteImage; // may be null if the project predates this field
    };

    static bool save(const pattern::PatternModel& pattern, const QImage& spriteImage,
                      const QString& path, QString* errorOut = nullptr);

    static std::optional<LoadResult> load(const QString& path, QString* errorOut = nullptr);
};

} // namespace ss::core::project
