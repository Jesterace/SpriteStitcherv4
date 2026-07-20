#include "MainWindow.h"

#include "GridView.h"
#include "SpritePreviewPanel.h"
#include "ToolPanel.h"
#include "dialogs/ImportOptionsDialog.h"

#include "ss/core/image/ImageLoader.h"
#include "ss/core/quantize/ColorReducer.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

namespace ss::ui {

using core::image::ImageLoader;
using core::quantize::ColorReducer;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_dmcMatcher(m_dmcTable) {
    setWindowTitle(tr("SpriteStitcher"));
    resize(1280, 840);

    m_gridView = new GridView(this);
    m_gridView->setDmcTable(&m_dmcTable);
    m_gridView->setPattern(&m_pattern);
    setCentralWidget(m_gridView);

    buildToolbar();
    buildDocks();
    buildStatusBar();

    connect(m_gridView, &GridView::cellHovered, this, &MainWindow::onCellHovered);
    connect(m_gridView, &GridView::zoomChanged, this, &MainWindow::onZoomChanged);

    onZoomChanged(m_gridView->pixelsPerCell());
}

void MainWindow::buildToolbar() {
    auto* toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar->addAction(tr("Open Image..."), this, &MainWindow::openImage);
    toolbar->addSeparator();

    toolbar->addAction(tr("Zoom Out"), m_gridView, &GridView::zoomOut);
    toolbar->addAction(tr("Fit"), m_gridView, &GridView::zoomToFit);
    toolbar->addAction(tr("Zoom In"), m_gridView, &GridView::zoomIn);
    toolbar->addSeparator();

    m_gridToggleAction = toolbar->addAction(tr("Grid"));
    m_gridToggleAction->setCheckable(true);
    m_gridToggleAction->setChecked(true);
    connect(m_gridToggleAction, &QAction::toggled, m_gridView, &GridView::setShowGrid);

    m_overlayToggleAction = toolbar->addAction(tr("Sprite Overlay"));
    m_overlayToggleAction->setCheckable(true);
    connect(m_overlayToggleAction, &QAction::toggled, m_gridView, &GridView::setShowSpriteOverlay);
}

void MainWindow::buildDocks() {
    m_spritePreview = new SpritePreviewPanel(this);
    auto* spriteDock = new QDockWidget(tr("Sprite Reference"), this);
    spriteDock->setWidget(m_spritePreview);
    spriteDock->setObjectName("spriteDock");
    addDockWidget(Qt::LeftDockWidgetArea, spriteDock);
    spriteDock->hide(); // available on demand; the overlay toggle covers the quick case

    m_toolPanel = new ToolPanel(this);
    m_toolPanel->setDmcTable(&m_dmcTable);
    m_toolPanel->setPattern(&m_pattern);
    auto* toolDock = new QDockWidget(tr("Palette"), this);
    toolDock->setWidget(m_toolPanel);
    toolDock->setObjectName("toolDock");
    addDockWidget(Qt::RightDockWidgetArea, toolDock);
}

void MainWindow::buildStatusBar() {
    m_statusCoord = new QLabel(tr("—"), this);
    m_statusZoom = new QLabel(this);
    m_statusSummary = new QLabel(tr("No pattern loaded"), this);

    statusBar()->addWidget(m_statusCoord);
    statusBar()->addPermanentWidget(m_statusSummary);
    statusBar()->addPermanentWidget(m_statusZoom);
}

void MainWindow::openImage() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Sprite Image"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp)"));
    if (path.isEmpty()) return;

    QString error;
    const auto loaded = ImageLoader::load(path, &error);
    if (!loaded) {
        QMessageBox::warning(this, tr("Import Failed"), error);
        return;
    }

    ImportOptionsDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    core::quantize::ReductionResult reduction;
    if (dialog.mode() == ImportOptionsDialog::Mode::Exact) {
        reduction = ColorReducer::reduceExact(loaded->image);
    } else {
        reduction = ColorReducer::reduceMedianCut(loaded->image, dialog.targetColorCount());
    }

    m_spriteImage = loaded->image;
    m_pattern.buildFromReduction(reduction, m_dmcMatcher, QFileInfo(path).completeBaseName());

    m_gridView->setSpriteImage(m_spriteImage);
    m_spritePreview->setImage(m_spriteImage);
    m_gridView->zoomToFit();

    updateStatusSummary();
}

void MainWindow::onCellHovered(int x, int y) {
    if (x < 0 || y < 0) {
        m_statusCoord->setText(tr("—"));
        return;
    }
    const auto& cell = m_pattern.cellAt(x, y);
    if (cell.dmcCode.isEmpty()) {
        m_statusCoord->setText(tr("(%1, %2)").arg(x).arg(y));
        return;
    }
    QString name;
    if (auto c = m_dmcTable.findByCode(cell.dmcCode.toStdString())) {
        name = QString::fromLatin1(c->name);
    }
    m_statusCoord->setText(tr("(%1, %2)  DMC %3 %4  '%5'")
                                .arg(x).arg(y).arg(cell.dmcCode, name, cell.symbol));
}

void MainWindow::onZoomChanged(double pixelsPerCell) {
    m_statusZoom->setText(tr("Zoom: %1%").arg(qRound(pixelsPerCell / 20.0 * 100.0)));
}

void MainWindow::updateStatusSummary() {
    const int w = m_pattern.width();
    const int h = m_pattern.height();
    const int stitches = w * h;
    const int colors = m_pattern.colorCounts().size();
    m_statusSummary->setText(
        tr("%1 × %2 stitches — %3 total — %4 colors").arg(w).arg(h).arg(stitches).arg(colors));
}

} // namespace ss::ui
