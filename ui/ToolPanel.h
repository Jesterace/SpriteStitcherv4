#pragma once

#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"

#include <QWidget>

class QTableWidget;
class QComboBox;
class QLabel;
class QPushButton;

namespace ss::ui {

// The one contextual dock, sectioned rather than split into separate
// toolbars/docks: DMC palette (click a row to pick the active color for
// painting cells), color swap, and the active-color indicator that doubles
// as cell-edit feedback.
class ToolPanel : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanel(QWidget* parent = nullptr);

    void setPattern(core::pattern::PatternModel* pattern);
    void setDmcTable(const core::dmc::DmcTable* table);

    // The DMC code to paint with. An empty string is meaningful here: it's
    // the eraser (paint cells as unstitched) — check hasActiveSelection()
    // first to tell "eraser selected" apart from "nothing selected yet",
    // since both otherwise look like an empty string.
    QString activeColor() const { return m_activeColor; }
    bool hasActiveSelection() const { return m_hasActiveSelection; }

public slots:
    void refresh();
    // Selects dmcCode as active; pass an empty string for the eraser.
    void setActiveColor(const QString& dmcCode);

signals:
    void activeColorChanged(const QString& dmcCode);
    void swapRequested(const QString& fromCode, const QString& toCode);

private:
    core::pattern::PatternModel* m_pattern = nullptr;
    const core::dmc::DmcTable* m_dmcTable = nullptr;

    QTableWidget* m_table;
    QLabel* m_activeColorLabel;
    QComboBox* m_swapToCombo;
    QPushButton* m_swapButton;

    QString m_activeColor;
    bool m_hasActiveSelection = false;

    void populateSwapCombo();
    void updateActiveColorLabel();
    void updateSwapButtonEnabled();
    void resetSelection();
};

} // namespace ss::ui
