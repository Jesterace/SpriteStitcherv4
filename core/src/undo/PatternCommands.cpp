#include "ss/core/undo/PatternCommands.h"

namespace ss::core::undo {

RecolorCellCommand::RecolorCellCommand(pattern::PatternModel* model, int x, int y, const QString& newCode)
    : QUndoCommand(QObject::tr("Recolor cell")),
      m_model(model),
      m_x(x),
      m_y(y),
      m_oldCode(model->cellAt(x, y).dmcCode),
      m_newCode(newCode) {}

void RecolorCellCommand::redo() {
    m_model->setCellColor(m_x, m_y, m_newCode);
}

void RecolorCellCommand::undo() {
    m_model->setCellColor(m_x, m_y, m_oldCode);
}

SwapColorCommand::SwapColorCommand(pattern::PatternModel* model, const QString& fromCode, const QString& toCode)
    : QUndoCommand(QObject::tr("Swap %1 to %2").arg(fromCode, toCode)),
      m_model(model),
      m_fromCode(fromCode),
      m_toCode(toCode) {
    for (int y = 0; y < model->height(); ++y) {
        for (int x = 0; x < model->width(); ++x) {
            if (model->cellAt(x, y).dmcCode == fromCode) {
                m_affectedCells.emplace_back(x, y);
            }
        }
    }
}

void SwapColorCommand::redo() {
    m_model->swapColor(m_fromCode, m_toCode);
}

void SwapColorCommand::undo() {
    m_model->setCellsColor(m_affectedCells, m_fromCode);
}

} // namespace ss::core::undo
