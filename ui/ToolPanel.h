#pragma once

#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"

#include <QWidget>

class QTableWidget;

namespace ss::ui {

// The one contextual dock: DMC palette for the current pattern. Color swap
// and cell-edit sections are added in a later phase; kept as a single
// sectioned panel rather than separate toolbars/docks per the UI design
// rules for this app.
class ToolPanel : public QWidget {
    Q_OBJECT

public:
    explicit ToolPanel(QWidget* parent = nullptr);

    void setPattern(core::pattern::PatternModel* pattern);
    void setDmcTable(const core::dmc::DmcTable* table) { m_dmcTable = table; }

public slots:
    void refresh();

private:
    core::pattern::PatternModel* m_pattern = nullptr;
    const core::dmc::DmcTable* m_dmcTable = nullptr;
    QTableWidget* m_table;
};

} // namespace ss::ui
