#include "ToolPanel.h"

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace ss::ui {

namespace {
constexpr int kColSwatch = 0;
constexpr int kColCode = 1;
constexpr int kColName = 2;
constexpr int kColSymbol = 3;
constexpr int kColCount = 4;
} // namespace

ToolPanel::ToolPanel(QWidget* parent) : QWidget(parent) {
    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({tr(""), tr("DMC"), tr("Name"), tr("Sym"), tr("Count")});
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setSectionResizeMode(kColSwatch, QHeaderView::Fixed);
    m_table->setColumnWidth(kColSwatch, 24);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::Stretch);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(m_table);
}

void ToolPanel::setPattern(core::pattern::PatternModel* pattern) {
    if (m_pattern) {
        disconnect(m_pattern, nullptr, this, nullptr);
    }
    m_pattern = pattern;
    if (m_pattern) {
        connect(m_pattern, &core::pattern::PatternModel::patternReset, this, &ToolPanel::refresh);
        connect(m_pattern, &core::pattern::PatternModel::cellChanged, this, &ToolPanel::refresh);
        connect(m_pattern, &core::pattern::PatternModel::colorSwapped, this, &ToolPanel::refresh);
    }
    refresh();
}

void ToolPanel::refresh() {
    m_table->setRowCount(0);
    if (!m_pattern || !m_dmcTable) return;

    const auto counts = m_pattern->colorCounts();
    const auto& symbols = m_pattern->symbolMap();

    m_table->setRowCount(counts.size());
    int row = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it, ++row) {
        const QString& code = it.key();
        const int count = it.value();

        QColor swatchColor(210, 210, 210);
        QString name = tr("Unknown");
        if (auto c = m_dmcTable->findByCode(code.toStdString())) {
            swatchColor = QColor(c->r, c->g, c->b);
            name = QString::fromLatin1(c->name);
        }

        auto* swatchItem = new QTableWidgetItem();
        swatchItem->setBackground(swatchColor);
        m_table->setItem(row, kColSwatch, swatchItem);
        m_table->setItem(row, kColCode, new QTableWidgetItem(code));
        m_table->setItem(row, kColName, new QTableWidgetItem(name));
        m_table->setItem(row, kColSymbol, new QTableWidgetItem(symbols.value(code)));
        auto* countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, count);
        m_table->setItem(row, kColCount, countItem);
    }
}

} // namespace ss::ui
