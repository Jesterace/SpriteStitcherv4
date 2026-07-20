#include "GridView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace ss::ui {

using core::pattern::PatternModel;

GridView::GridView(QWidget* parent) : QAbstractScrollArea(parent) {
    setMouseTracking(true);
    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void GridView::setPattern(PatternModel* pattern) {
    if (m_pattern) {
        disconnect(m_pattern, nullptr, this, nullptr);
    }
    m_pattern = pattern;
    if (m_pattern) {
        connect(m_pattern, &PatternModel::patternReset, this, [this] {
            updateScrollRange();
            viewport()->update();
        });
        connect(m_pattern, &PatternModel::cellChanged, this, [this](int, int) {
            viewport()->update();
        });
        connect(m_pattern, &PatternModel::colorSwapped, this, [this](const QString&, const QString&) {
            viewport()->update();
        });
        connect(m_pattern, &PatternModel::cellsChanged, this, [this] {
            viewport()->update();
        });
    }
    updateScrollRange();
    viewport()->update();
}

void GridView::setShowGrid(bool show) {
    if (m_showGrid == show) return;
    m_showGrid = show;
    viewport()->update();
}

void GridView::setShowSpriteOverlay(bool show) {
    if (m_showSpriteOverlay == show) return;
    m_showSpriteOverlay = show;
    viewport()->update();
}

QRectF GridView::gridArea() const {
    return QRectF(kRulerMargin, kRulerMargin,
                  std::max(0, viewport()->width() - kRulerMargin),
                  std::max(0, viewport()->height() - kRulerMargin));
}

void GridView::updateScrollRange() {
    if (!m_pattern) {
        horizontalScrollBar()->setRange(0, 0);
        verticalScrollBar()->setRange(0, 0);
        return;
    }
    const QRectF area = gridArea();
    const int contentW = static_cast<int>(std::ceil(m_pattern->width() * m_pixelsPerCell));
    const int contentH = static_cast<int>(std::ceil(m_pattern->height() * m_pixelsPerCell));

    horizontalScrollBar()->setRange(0, std::max(0, contentW - static_cast<int>(area.width())));
    horizontalScrollBar()->setPageStep(static_cast<int>(area.width()));
    verticalScrollBar()->setRange(0, std::max(0, contentH - static_cast<int>(area.height())));
    verticalScrollBar()->setPageStep(static_cast<int>(area.height()));
}

void GridView::resizeEvent(QResizeEvent* event) {
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
}

QRgb GridView::colorForCode(const QString& dmcCode) const {
    if (m_dmcTable && !dmcCode.isEmpty()) {
        if (auto c = m_dmcTable->findByCode(dmcCode.toStdString())) {
            return qRgb(c->r, c->g, c->b);
        }
    }
    return qRgb(210, 210, 210);
}

void GridView::paintEvent(QPaintEvent*) {
    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), palette().color(QPalette::Mid));

    if (!m_pattern || m_pattern->width() == 0 || m_pattern->height() == 0) {
        return;
    }

    const QRectF area = gridArea();
    const double offsetX = horizontalScrollBar()->value();
    const double offsetY = verticalScrollBar()->value();
    const double cellSize = m_pixelsPerCell;

    painter.fillRect(area, Qt::white);

    int firstCol = static_cast<int>(std::floor(offsetX / cellSize));
    int lastCol = static_cast<int>(std::ceil((offsetX + area.width()) / cellSize));
    int firstRow = static_cast<int>(std::floor(offsetY / cellSize));
    int lastRow = static_cast<int>(std::ceil((offsetY + area.height()) / cellSize));
    firstCol = std::clamp(firstCol, 0, m_pattern->width() - 1);
    lastCol = std::clamp(lastCol, 0, m_pattern->width() - 1);
    firstRow = std::clamp(firstRow, 0, m_pattern->height() - 1);
    lastRow = std::clamp(lastRow, 0, m_pattern->height() - 1);

    painter.save();
    painter.setClipRect(area);

    if (m_showSpriteOverlay && !m_spriteImage.isNull()) {
        const QRectF dest(area.left() - offsetX, area.top() - offsetY,
                           m_pattern->width() * cellSize, m_pattern->height() * cellSize);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dest, m_spriteImage);
    } else {
        QFont symbolFont = painter.font();
        const bool drawSymbols = cellSize >= 14.0;
        if (drawSymbols) {
            symbolFont.setPixelSize(static_cast<int>(cellSize * 0.6));
            painter.setFont(symbolFont);
        }

        for (int y = firstRow; y <= lastRow; ++y) {
            for (int x = firstCol; x <= lastCol; ++x) {
                const auto& cell = m_pattern->cellAt(x, y);
                const QRectF cellRect(area.left() + x * cellSize - offsetX,
                                       area.top() + y * cellSize - offsetY,
                                       cellSize, cellSize);
                painter.fillRect(cellRect, QColor(colorForCode(cell.dmcCode)));

                if (drawSymbols && !cell.symbol.isEmpty()) {
                    const QColor bg(colorForCode(cell.dmcCode));
                    painter.setPen(bg.lightnessF() > 0.55 ? Qt::black : Qt::white);
                    painter.drawText(cellRect, Qt::AlignCenter, cell.symbol);
                }
            }
        }
    }

    if (m_showGrid && cellSize >= 4.0) {
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        for (int x = firstCol; x <= lastCol + 1; ++x) {
            const double lineX = area.left() + x * cellSize - offsetX;
            painter.drawLine(QPointF(lineX, area.top()), QPointF(lineX, area.bottom()));
        }
        for (int y = firstRow; y <= lastRow + 1; ++y) {
            const double lineY = area.top() + y * cellSize - offsetY;
            painter.drawLine(QPointF(area.left(), lineY), QPointF(area.right(), lineY));
        }
    }

    // Red center lines, both axes — matches the PDF chart style.
    painter.setPen(QPen(QColor(200, 30, 30), 2));
    const double centerX = area.left() + (m_pattern->width() / 2.0) * cellSize - offsetX;
    const double centerY = area.top() + (m_pattern->height() / 2.0) * cellSize - offsetY;
    if (centerX >= area.left() && centerX <= area.right()) {
        painter.drawLine(QPointF(centerX, area.top()), QPointF(centerX, area.bottom()));
    }
    if (centerY >= area.top() && centerY <= area.bottom()) {
        painter.drawLine(QPointF(area.left(), centerY), QPointF(area.right(), centerY));
    }

    painter.restore();

    // Ruler margins (unclipped): background + numbers every 10 stitches.
    painter.fillRect(QRectF(0, 0, viewport()->width(), kRulerMargin), palette().color(QPalette::Window));
    painter.fillRect(QRectF(0, 0, kRulerMargin, viewport()->height()), palette().color(QPalette::Window));
    painter.setPen(palette().color(QPalette::WindowText));

    QFont rulerFont = painter.font();
    rulerFont.setPixelSize(10);
    painter.setFont(rulerFont);

    for (int x = firstCol; x <= lastCol; ++x) {
        if (x % 10 != 0) continue;
        const double lineX = area.left() + x * cellSize - offsetX;
        if (lineX < area.left() || lineX > area.right()) continue;
        painter.drawText(QRectF(lineX - 15, 2, 30, kRulerMargin - 4), Qt::AlignCenter, QString::number(x));
    }
    for (int y = firstRow; y <= lastRow; ++y) {
        if (y % 10 != 0) continue;
        const double lineY = area.top() + y * cellSize - offsetY;
        if (lineY < area.top() || lineY > area.bottom()) continue;
        painter.drawText(QRectF(2, lineY - 8, kRulerMargin - 4, 16), Qt::AlignCenter, QString::number(y));
    }
}

QPoint GridView::cellAt(QPoint viewportPos) const {
    if (!m_pattern) return QPoint(-1, -1);
    const QRectF area = gridArea();
    if (!area.contains(viewportPos)) return QPoint(-1, -1);

    const double contentX = horizontalScrollBar()->value() + (viewportPos.x() - area.left());
    const double contentY = verticalScrollBar()->value() + (viewportPos.y() - area.top());
    const int col = static_cast<int>(std::floor(contentX / m_pixelsPerCell));
    const int row = static_cast<int>(std::floor(contentY / m_pixelsPerCell));

    if (!m_pattern->isValidCoord(col, row)) return QPoint(-1, -1);
    return QPoint(col, row);
}

void GridView::mouseMoveEvent(QMouseEvent* event) {
    const QPoint c = cellAt(event->pos());
    emit cellHovered(c.x(), c.y());
}

void GridView::mousePressEvent(QMouseEvent* event) {
    const QPoint c = cellAt(event->pos());
    if (c.x() >= 0) {
        emit cellClicked(c.x(), c.y(), event->button());
    }
}

void GridView::leaveEvent(QEvent*) {
    emit cellHovered(-1, -1);
}

void GridView::setZoomAnchored(double newPixelsPerCell, QPoint viewportAnchor) {
    const double clamped = std::clamp(newPixelsPerCell, kMinPixelsPerCell, kMaxPixelsPerCell);
    if (qFuzzyCompare(clamped, m_pixelsPerCell)) return;

    const QRectF area = gridArea();
    const double anchorContentX = horizontalScrollBar()->value() + (viewportAnchor.x() - area.left());
    const double anchorContentY = verticalScrollBar()->value() + (viewportAnchor.y() - area.top());
    const double cellX = anchorContentX / m_pixelsPerCell;
    const double cellY = anchorContentY / m_pixelsPerCell;

    m_pixelsPerCell = clamped;
    updateScrollRange();

    const double newContentX = cellX * m_pixelsPerCell;
    const double newContentY = cellY * m_pixelsPerCell;
    horizontalScrollBar()->setValue(static_cast<int>(newContentX - (viewportAnchor.x() - area.left())));
    verticalScrollBar()->setValue(static_cast<int>(newContentY - (viewportAnchor.y() - area.top())));

    viewport()->update();
    emit zoomChanged(m_pixelsPerCell);
}

void GridView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const double factor = event->angleDelta().y() > 0 ? 1.25 : 0.8;
        setZoomAnchored(m_pixelsPerCell * factor, event->position().toPoint());
        event->accept();
        return;
    }
    QAbstractScrollArea::wheelEvent(event);
}

void GridView::zoomIn() {
    setZoomAnchored(m_pixelsPerCell * 1.25, viewport()->rect().center());
}

void GridView::zoomOut() {
    setZoomAnchored(m_pixelsPerCell * 0.8, viewport()->rect().center());
}

void GridView::zoomToFit() {
    if (!m_pattern || m_pattern->width() == 0 || m_pattern->height() == 0) return;
    const QRectF area = gridArea();
    const double fitX = area.width() / m_pattern->width();
    const double fitY = area.height() / m_pattern->height();
    m_pixelsPerCell = std::clamp(std::min(fitX, fitY), kMinPixelsPerCell, kMaxPixelsPerCell);
    updateScrollRange();
    horizontalScrollBar()->setValue(0);
    verticalScrollBar()->setValue(0);
    viewport()->update();
    emit zoomChanged(m_pixelsPerCell);
}

void GridView::resetZoom() {
    setZoomAnchored(20.0, viewport()->rect().center());
}

} // namespace ss::ui
