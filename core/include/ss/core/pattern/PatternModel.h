#pragma once

#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/pattern/PatternCell.h"
#include "ss/core/quantize/ColorReducer.h"

#include <QMap>
#include <QObject>
#include <QPoint>
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

    // Restores a pattern from already-resolved DMC codes (project file
    // load) — no DMC matching involved. `cellCodes` is row-major,
    // width*height entries. `symbolMap` is honored as-is where possible
    // (kept stable across save/load) and only filled in for codes it's
    // missing.
    void loadFromSaved(int width, int height, const QString& name,
                        const std::vector<QString>& cellCodes,
                        const QMap<QString, QString>& symbolMap);

    int width() const { return m_width; }
    int height() const { return m_height; }
    QString name() const { return m_name; }
    void setName(const QString& name);

    bool isValidCoord(int x, int y) const;
    const PatternCell& cellAt(int x, int y) const;

    // Sets a single cell's DMC color (symbol is looked up/assigned from the
    // pattern's current symbol map).
    void setCellColor(int x, int y, const QString& dmcCode);

    // Sets exactly the given cells to one color in a single pass (one
    // symbol-map rebuild, one signal) rather than N setCellColor calls.
    // Exists mainly so undoing a color swap can restore precisely the
    // cells that swap touched, without re-scanning by color (which would
    // incorrectly also touch cells that already had the target color
    // before the swap ran).
    void setCellsColor(const std::vector<QPoint>& cells, const QString& dmcCode);

    // Replaces every occurrence of one DMC color with another across the
    // whole grid.
    void swapColor(const QString& fromCode, const QString& toCode);

    const QMap<QString, QString>& symbolMap() const { return m_symbolMap; }

    // DMC code -> number of stitches currently using it. Recomputed from
    // the grid each call. Blank/unstitched cells are excluded.
    QMap<QString, int> colorCounts() const;

    // Number of blank (unstitched/background) cells in the grid.
    int blankCellCount() const;

signals:
    void patternReset();
    void cellChanged(int x, int y);
    void cellsChanged(); // bulk change (setCellsColor); listeners should redraw everything
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
