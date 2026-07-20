#pragma once

#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/pattern/PatternCell.h"
#include "ss/core/quantize/ColorReducer.h"

#include <QMap>
#include <QObject>
#include <QString>

#include <vector>

namespace ss::core::pattern {

// Full state of a pattern: the stitch grid plus the DMC->symbol assignment
// currently in use. Owns no undo history itself (see core/undo) so it can
// be driven directly by tests or tools as well as the UI.
class PatternModel : public QObject {
    Q_OBJECT

public:
    explicit PatternModel(QObject* parent = nullptr);

    void reset(int width, int height, const QString& name = QString());

    // Builds a pattern by matching every color in a reduced image against
    // the DMC table.
    void buildFromReduction(const quantize::ReductionResult& reduction,
                             const dmc::DmcMatcher& matcher,
                             const QString& name = QString());

    int width() const { return m_width; }
    int height() const { return m_height; }
    QString name() const { return m_name; }
    void setName(const QString& name);

    bool isValidCoord(int x, int y) const;
    const PatternCell& cellAt(int x, int y) const;

    // Sets a single cell's DMC color (symbol is looked up/assigned from the
    // pattern's current symbol map).
    void setCellColor(int x, int y, const QString& dmcCode);

    // Replaces every occurrence of one DMC color with another across the
    // whole grid.
    void swapColor(const QString& fromCode, const QString& toCode);

    const QMap<QString, QString>& symbolMap() const { return m_symbolMap; }

    // DMC code -> number of stitches currently using it. Recomputed from
    // the grid each call.
    QMap<QString, int> colorCounts() const;

signals:
    void patternReset();
    void cellChanged(int x, int y);
    void colorSwapped(const QString& fromCode, const QString& toCode);

private:
    int m_width = 0;
    int m_height = 0;
    QString m_name;
    std::vector<PatternCell> m_cells;
    QMap<QString, QString> m_symbolMap;

    size_t indexOf(int x, int y) const { return static_cast<size_t>(y) * m_width + x; }
    void rebuildSymbolMapFromGrid();
};

} // namespace ss::core::pattern
