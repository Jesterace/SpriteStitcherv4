#pragma once

#include "ss/core/pattern/PatternModel.h"

#include <QUndoCommand>

#include <vector>

namespace ss::core::undo {

// Recolors a single cell. Captures the previous color at construction time
// (before QUndoStack::push() invokes the first redo()).
class RecolorCellCommand : public QUndoCommand {
public:
    RecolorCellCommand(pattern::PatternModel* model, int x, int y, const QString& newCode);

    void redo() override;
    void undo() override;

private:
    pattern::PatternModel* m_model;
    int m_x;
    int m_y;
    QString m_oldCode;
    QString m_newCode;
};

// Replaces one DMC color with another across the whole pattern. Undo
// restores exactly the cells this command changed (captured up front) —
// not a blanket reverse-swap, which would be wrong if the target color
// already existed elsewhere in the pattern before the swap ran.
class SwapColorCommand : public QUndoCommand {
public:
    SwapColorCommand(pattern::PatternModel* model, const QString& fromCode, const QString& toCode);

    void redo() override;
    void undo() override;

private:
    pattern::PatternModel* m_model;
    QString m_fromCode;
    QString m_toCode;
    std::vector<QPoint> m_affectedCells;
};

} // namespace ss::core::undo
