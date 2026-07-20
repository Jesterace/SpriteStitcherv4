#include "ss/core/pattern/PatternModel.h"

#include "ss/core/pattern/SymbolAssigner.h"

#include <QSet>

namespace ss::core::pattern {

PatternModel::PatternModel(QObject* parent) : QObject(parent) {}

void PatternModel::reset(int width, int height, const QString& name) {
    m_width = width;
    m_height = height;
    m_name = name;
    m_cells.assign(static_cast<size_t>(width) * height, PatternCell{});
    m_symbolMap.clear();
    emit patternReset();
}

void PatternModel::buildFromReduction(const quantize::ReductionResult& reduction,
                                       const dmc::DmcMatcher& matcher,
                                       const QString& name) {
    m_width = reduction.width;
    m_height = reduction.height;
    m_name = name;
    m_cells.assign(static_cast<size_t>(m_width) * m_height, PatternCell{});

    std::vector<QString> paletteToCode(reduction.palette.size());
    for (size_t i = 0; i < reduction.palette.size(); ++i) {
        const QRgb c = reduction.palette[i];
        const auto dmc = matcher.match(qRed(c), qGreen(c), qBlue(c));
        paletteToCode[i] = QString::fromLatin1(dmc.code);
    }

    for (size_t i = 0; i < reduction.paletteIndex.size(); ++i) {
        m_cells[i].dmcCode = paletteToCode[static_cast<size_t>(reduction.paletteIndex[i])];
    }

    rebuildSymbolMapFromGrid();
    emit patternReset();
}

void PatternModel::setName(const QString& name) {
    m_name = name;
}

bool PatternModel::isValidCoord(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

const PatternCell& PatternModel::cellAt(int x, int y) const {
    return m_cells[indexOf(x, y)];
}

void PatternModel::setCellColor(int x, int y, const QString& dmcCode) {
    if (!isValidCoord(x, y)) return;

    if (!m_symbolMap.contains(dmcCode)) {
        QVector<QString> codes = m_symbolMap.keys().toVector();
        codes.push_back(dmcCode);
        m_symbolMap = SymbolAssigner::assign(codes, m_symbolMap);
    }

    auto& cell = m_cells[indexOf(x, y)];
    cell.dmcCode = dmcCode;
    cell.symbol = m_symbolMap.value(dmcCode);

    emit cellChanged(x, y);
}

void PatternModel::setCellsColor(const std::vector<QPoint>& cells, const QString& dmcCode) {
    if (cells.empty()) return;

    if (!m_symbolMap.contains(dmcCode)) {
        QVector<QString> codes = m_symbolMap.keys().toVector();
        codes.push_back(dmcCode);
        m_symbolMap = SymbolAssigner::assign(codes, m_symbolMap);
    }
    const QString symbol = m_symbolMap.value(dmcCode);

    for (const QPoint& p : cells) {
        if (!isValidCoord(p.x(), p.y())) continue;
        auto& cell = m_cells[indexOf(p.x(), p.y())];
        cell.dmcCode = dmcCode;
        cell.symbol = symbol;
    }

    rebuildSymbolMapFromGrid();
    emit cellsChanged();
}

void PatternModel::swapColor(const QString& fromCode, const QString& toCode) {
    if (fromCode == toCode) return;

    if (!m_symbolMap.contains(toCode)) {
        QVector<QString> codes = m_symbolMap.keys().toVector();
        codes.push_back(toCode);
        m_symbolMap = SymbolAssigner::assign(codes, m_symbolMap);
    }
    const QString newSymbol = m_symbolMap.value(toCode);

    for (auto& cell : m_cells) {
        if (cell.dmcCode == fromCode) {
            cell.dmcCode = toCode;
            cell.symbol = newSymbol;
        }
    }

    rebuildSymbolMapFromGrid();
    emit colorSwapped(fromCode, toCode);
}

QMap<QString, int> PatternModel::colorCounts() const {
    QMap<QString, int> counts;
    for (const auto& cell : m_cells) {
        if (cell.dmcCode.isEmpty()) continue;
        counts[cell.dmcCode]++;
    }
    return counts;
}

void PatternModel::rebuildSymbolMapFromGrid() {
    QSet<QString> distinctCodes;
    for (const auto& cell : m_cells) {
        if (!cell.dmcCode.isEmpty()) distinctCodes.insert(cell.dmcCode);
    }
    const QVector<QString> codes = distinctCodes.values().toVector();
    m_symbolMap = SymbolAssigner::assign(codes, m_symbolMap);

    for (auto& cell : m_cells) {
        if (!cell.dmcCode.isEmpty()) {
            cell.symbol = m_symbolMap.value(cell.dmcCode);
        }
    }
}

} // namespace ss::core::pattern
