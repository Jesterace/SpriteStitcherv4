#pragma once

#include "ss/core/dmc/DmcMatcher.h"
#include "ss/core/dmc/DmcTable.h"
#include "ss/core/pattern/PatternModel.h"

#include <QMainWindow>
#include <QUndoStack>

class QAction;
class QLabel;

namespace ss::ui {

class GridView;
class SpritePreviewPanel;
class ToolPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openImage();
    void exportPdf();
    void onCellHovered(int x, int y);
    void onZoomChanged(double pixelsPerCell);
    void onCellClicked(int x, int y, Qt::MouseButton button);
    void onSwapRequested(const QString& fromCode, const QString& toCode);

private:
    void buildToolbar();
    void buildMenuBar();
    void buildDocks();
    void buildStatusBar();
    void updateStatusSummary();

    core::dmc::DmcTable m_dmcTable;
    core::dmc::DmcMatcher m_dmcMatcher;
    core::pattern::PatternModel m_pattern;
    QImage m_spriteImage;
    QUndoStack m_undoStack;

    GridView* m_gridView;
    SpritePreviewPanel* m_spritePreview;
    ToolPanel* m_toolPanel;

    QAction* m_gridToggleAction;
    QAction* m_overlayToggleAction;
    QAction* m_exportAction;

    QLabel* m_statusCoord;
    QLabel* m_statusZoom;
    QLabel* m_statusSummary;
};

} // namespace ss::ui
