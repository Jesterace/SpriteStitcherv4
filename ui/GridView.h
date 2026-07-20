#pragma once

#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"

#include <QAbstractScrollArea>
#include <QImage>

namespace ss::ui {

// The centerpiece view: a custom-painted, viewport-culled stitch grid with
// smooth cursor-anchored zoom and native scrollbar panning. Deliberately not
// QGraphicsView/QGraphicsScene — at 300x300 cells that path means 90k
// QGraphicsItems, which is the kind of thing that stops being smooth. Direct
// QPainter + viewport culling keeps repaint cost proportional to what's
// actually on screen.
class GridView : public QAbstractScrollArea {
    Q_OBJECT

public:
    explicit GridView(QWidget* parent = nullptr);

    void setPattern(core::pattern::PatternModel* pattern);
    void setDmcTable(const core::dmc::DmcTable* table) { m_dmcTable = table; }

    void setShowGrid(bool show);
    bool showGrid() const { return m_showGrid; }

    void setShowSpriteOverlay(bool show);
    bool showSpriteOverlay() const { return m_showSpriteOverlay; }
    void setSpriteImage(const QImage& image) { m_spriteImage = image; }

    double pixelsPerCell() const { return m_pixelsPerCell; }

public slots:
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void resetZoom();

signals:
    void zoomChanged(double pixelsPerCell);
    void cellHovered(int x, int y); // (-1, -1) when the cursor leaves the grid
    void cellClicked(int x, int y, Qt::MouseButton button);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    static constexpr double kMinPixelsPerCell = 1.0;
    static constexpr double kMaxPixelsPerCell = 96.0;
    static constexpr int kRulerMargin = 22;

    core::pattern::PatternModel* m_pattern = nullptr;
    const core::dmc::DmcTable* m_dmcTable = nullptr;
    QImage m_spriteImage;

    bool m_showGrid = true;
    bool m_showSpriteOverlay = false;
    double m_pixelsPerCell = 20.0;

    void updateScrollRange();
    void setZoomAnchored(double newPixelsPerCell, QPoint viewportAnchor);
    QRectF gridArea() const; // viewport-relative area available for the grid, after the ruler margin
    QPoint cellAt(QPoint viewportPos) const; // (-1,-1) if outside the pattern
    QRgb colorForCode(const QString& dmcCode) const;
};

} // namespace ss::ui
