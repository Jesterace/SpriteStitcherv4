#include "ToolPanel.h"

#include <QBrush>
#include <QComboBox>
#include <QCompleter>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace ss::ui {

namespace {
constexpr int kColSwatch = 0;
constexpr int kColCode = 1;
constexpr int kColName = 2;
constexpr int kColSymbol = 3;
constexpr int kColCount = 4;
constexpr int kCodeRole = Qt::UserRole;
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

    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const auto selected = m_table->selectedItems();
        if (selected.isEmpty()) return;
        const int row = selected.first()->row();
        auto* swatchItem = m_table->item(row, kColSwatch);
        if (!swatchItem) return;
        // Eraser row's code is deliberately empty (that's what "paint as
        // unstitched" means to PatternModel) — the separate role is just
        // to tell it apart here from "no row selected at all".
        setActiveColor(swatchItem->data(kCodeRole).toString());
    });

    m_activeColorLabel = new QLabel(tr("Active color: none (click a palette row)"), this);
    m_activeColorLabel->setWordWrap(true);

    m_swapToCombo = new QComboBox(this);
    m_swapToCombo->setEditable(true);
    m_swapToCombo->setInsertPolicy(QComboBox::NoInsert);
    m_swapToCombo->setPlaceholderText(tr("Swap to..."));
    connect(m_swapToCombo, &QComboBox::currentIndexChanged, this, &ToolPanel::updateSwapButtonEnabled);
    connect(m_swapToCombo, &QComboBox::editTextChanged, this, &ToolPanel::updateSwapButtonEnabled);

    m_swapButton = new QPushButton(tr("Swap"), this);
    m_swapButton->setEnabled(false);
    m_swapButton->setToolTip(tr("Replace the active color with the selected color everywhere in the pattern."));
    connect(m_swapButton, &QPushButton::clicked, this, [this] {
        const int idx = m_swapToCombo->currentIndex();
        if (idx < 0) return;
        const QString toCode = m_swapToCombo->itemData(idx).toString();
        if (m_activeColor.isEmpty() || toCode.isEmpty() || toCode == m_activeColor) return;
        emit swapRequested(m_activeColor, toCode);
    });

    auto* swapRow = new QHBoxLayout;
    swapRow->addWidget(m_swapToCombo, 1);
    swapRow->addWidget(m_swapButton);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_activeColorLabel);
    layout->addLayout(swapRow);
}

void ToolPanel::setDmcTable(const core::dmc::DmcTable* table) {
    m_dmcTable = table;
    populateSwapCombo();
}

void ToolPanel::populateSwapCombo() {
    m_swapToCombo->clear();
    if (!m_dmcTable) return;

    for (const auto& c : m_dmcTable->colors()) {
        const QString code = QString::fromLatin1(c.code);
        const QString name = QString::fromLatin1(c.name);
        m_swapToCombo->addItem(QStringLiteral("%1  %2").arg(code, name), code);
    }
    m_swapToCombo->setCurrentIndex(-1);

    auto* completer = new QCompleter(m_swapToCombo->model(), m_swapToCombo);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    m_swapToCombo->setCompleter(completer);
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
        connect(m_pattern, &core::pattern::PatternModel::cellsChanged, this, &ToolPanel::refresh);
    }
    resetSelection();
    refresh();
}

void ToolPanel::resetSelection() {
    m_activeColor.clear();
    m_hasActiveSelection = false;
    m_table->blockSignals(true);
    m_table->clearSelection();
    m_table->blockSignals(false);
    updateActiveColorLabel();
    updateSwapButtonEnabled();
    emit activeColorChanged(m_activeColor);
}

void ToolPanel::setActiveColor(const QString& dmcCode) {
    if (m_hasActiveSelection && m_activeColor == dmcCode) return;
    m_activeColor = dmcCode;
    m_hasActiveSelection = true;

    // Exactly one row has an empty code (the eraser row), so this finds
    // it correctly whether dmcCode is a real DMC code or the eraser's "".
    m_table->blockSignals(true);
    m_table->clearSelection();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto* swatchItem = m_table->item(row, kColSwatch);
        if (swatchItem && swatchItem->data(kCodeRole).toString() == dmcCode) {
            m_table->selectRow(row);
            break;
        }
    }
    m_table->blockSignals(false);

    updateActiveColorLabel();
    updateSwapButtonEnabled();
    emit activeColorChanged(m_activeColor);
}

void ToolPanel::updateActiveColorLabel() {
    if (!m_hasActiveSelection) {
        m_activeColorLabel->setText(tr("Active color: none (click a palette row)"));
        return;
    }
    if (m_activeColor.isEmpty()) {
        m_activeColorLabel->setText(tr("Active: Unstitched — click a cell to erase it"));
        return;
    }
    QString name = tr("Unknown");
    if (m_dmcTable) {
        if (auto c = m_dmcTable->findByCode(m_activeColor.toStdString())) {
            name = QString::fromLatin1(c->name);
        }
    }
    m_activeColorLabel->setText(tr("Active color: DMC %1 %2 — click a cell to paint it").arg(m_activeColor, name));
}

void ToolPanel::updateSwapButtonEnabled() {
    const int idx = m_swapToCombo->currentIndex();
    const QString toCode = idx >= 0 ? m_swapToCombo->itemData(idx).toString() : QString();
    m_swapButton->setEnabled(!m_activeColor.isEmpty() && !toCode.isEmpty() && toCode != m_activeColor);
}

void ToolPanel::refresh() {
    m_table->setRowCount(0);
    if (!m_pattern || !m_dmcTable) return;

    const auto counts = m_pattern->colorCounts();
    const auto& symbols = m_pattern->symbolMap();
    const int blanks = m_pattern->blankCellCount();

    m_table->setRowCount(counts.size() + (blanks > 0 ? 1 : 0));
    int row = 0;

    if (blanks > 0) {
        // Synthetic row, not a real DMC entry: selecting it arms the
        // eraser (paint cells as unstitched via an empty dmcCode).
        auto* swatchItem = new QTableWidgetItem();
        swatchItem->setBackground(QBrush(Qt::lightGray, Qt::DiagCrossPattern));
        swatchItem->setData(kCodeRole, QString());
        m_table->setItem(row, kColSwatch, swatchItem);
        m_table->setItem(row, kColCode, new QTableWidgetItem(QStringLiteral("—")));
        m_table->setItem(row, kColName, new QTableWidgetItem(tr("Unstitched")));
        m_table->setItem(row, kColSymbol, new QTableWidgetItem());
        auto* countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, blanks);
        m_table->setItem(row, kColCount, countItem);

        if (m_hasActiveSelection && m_activeColor.isEmpty()) {
            m_table->selectRow(row);
        }
        ++row;
    }

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
        swatchItem->setData(kCodeRole, code);
        m_table->setItem(row, kColSwatch, swatchItem);
        m_table->setItem(row, kColCode, new QTableWidgetItem(code));
        m_table->setItem(row, kColName, new QTableWidgetItem(name));
        m_table->setItem(row, kColSymbol, new QTableWidgetItem(symbols.value(code)));
        auto* countItem = new QTableWidgetItem();
        countItem->setData(Qt::DisplayRole, count);
        m_table->setItem(row, kColCount, countItem);

        if (m_hasActiveSelection && code == m_activeColor) {
            m_table->selectRow(row);
        }
    }

    updateActiveColorLabel();
    updateSwapButtonEnabled();
}

} // namespace ss::ui
