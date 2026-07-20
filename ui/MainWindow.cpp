#include "MainWindow.h"

#include "GridView.h"
#include "SpritePreviewPanel.h"
#include "ToolPanel.h"
#include "dialogs/ImportOptionsDialog.h"

#include "ss/core/image/ImageLoader.h"
#include "ss/core/pdf/PdfExporter.h"
#include "ss/core/project/ProjectSerializer.h"
#include "ss/core/quantize/ColorReducer.h"
#include "ss/core/undo/PatternCommands.h"

#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
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
    buildMenuBar();
    buildStatusBar();

    connect(m_gridView, &GridView::cellHovered, this, &MainWindow::onCellHovered);
    connect(m_gridView, &GridView::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(m_gridView, &GridView::cellClicked, this, &MainWindow::onCellClicked);
    connect(m_toolPanel, &ToolPanel::swapRequested, this, &MainWindow::onSwapRequested);
    connect(&m_pattern, &core::pattern::PatternModel::colorSwapped, this, &MainWindow::updateStatusSummary);
    connect(&m_pattern, &core::pattern::PatternModel::cellsChanged, this, &MainWindow::updateStatusSummary);
    connect(&m_pattern, &core::pattern::PatternModel::cellChanged, this, &MainWindow::updateStatusSummary);
    connect(&m_undoStack, &QUndoStack::cleanChanged, this, &MainWindow::updateWindowTitle);

    onZoomChanged(m_gridView->pixelsPerCell());
    updateWindowTitle();
}

void MainWindow::buildToolbar() {
    auto* toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar->addAction(tr("Open Image..."), this, &MainWindow::openImage);
    m_saveProjectAction = toolbar->addAction(tr("Save Project"), this, &MainWindow::saveProject);
    m_saveProjectAction->setEnabled(false);
    m_exportAction = toolbar->addAction(tr("Export PDF..."), this, &MainWindow::exportPdf);
    m_exportAction->setEnabled(false);
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

void MainWindow::buildMenuBar() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* openProjectAction = fileMenu->addAction(tr("Open Project..."), this, &MainWindow::openProject);
    openProjectAction->setShortcut(QKeySequence::Open);

    QAction* saveAction = fileMenu->addAction(tr("Save Project"), this, &MainWindow::saveProject);
    saveAction->setShortcut(QKeySequence::Save);

    QAction* saveAsAction = fileMenu->addAction(tr("Save Project As..."), this, &MainWindow::saveProjectAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Import Sprite Image..."), this, &MainWindow::openImage);
    fileMenu->addAction(tr("Export PDF..."), this, &MainWindow::exportPdf);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit"), this, &QWidget::close)->setShortcut(QKeySequence::Quit);

    auto* editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction* undoAction = m_undoStack.createUndoAction(this, tr("Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);

    QAction* redoAction = m_undoStack.createRedoAction(this, tr("Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAction);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_gridToggleAction);
    viewMenu->addAction(m_overlayToggleAction);
    viewMenu->addAction(m_spriteDock->toggleViewAction());
}

void MainWindow::buildDocks() {
    m_spritePreview = new SpritePreviewPanel(this);
    m_spriteDock = new QDockWidget(tr("Sprite Reference"), this);
    m_spriteDock->setWidget(m_spritePreview);
    m_spriteDock->setObjectName("spriteDock");
    addDockWidget(Qt::LeftDockWidgetArea, m_spriteDock);
    m_spriteDock->hide(); // available on demand; the overlay toggle covers the quick case

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
    if (!confirmDiscardUnsavedChanges()) return;

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
    m_currentProjectPath.clear(); // a fresh import isn't tied to any saved project file yet
    m_undoStack.clear();          // importing is a new document, not an edit to undo back through

    loadPatternIntoUi();
}

void MainWindow::openProject() {
    if (!confirmDiscardUnsavedChanges()) return;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Project"), QString(), tr("SpriteStitcher Projects (*.sstitch)"));
    if (path.isEmpty()) return;

    QString error;
    auto loaded = core::project::ProjectSerializer::load(path, &error);
    if (!loaded) {
        QMessageBox::warning(this, tr("Open Failed"), error);
        return;
    }

    m_spriteImage = loaded->spriteImage;
    m_pattern.loadFromSaved(loaded->width, loaded->height, loaded->name, loaded->cellCodes, loaded->symbolMap);
    m_currentProjectPath = path;
    m_undoStack.clear();
    m_undoStack.setClean();

    loadPatternIntoUi();
}

bool MainWindow::saveProject() {
    if (m_currentProjectPath.isEmpty()) return saveProjectAs();

    QString error;
    if (!core::project::ProjectSerializer::save(m_pattern, m_spriteImage, m_currentProjectPath, &error)) {
        QMessageBox::warning(this, tr("Save Failed"), error);
        return false;
    }
    m_undoStack.setClean();
    return true;
}

bool MainWindow::saveProjectAs() {
    const QString suggested = (m_pattern.name().isEmpty() ? tr("Untitled") : m_pattern.name()) + ".sstitch";
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Project As"), suggested,
                                                        tr("SpriteStitcher Projects (*.sstitch)"));
    if (path.isEmpty()) return false;

    QString error;
    if (!core::project::ProjectSerializer::save(m_pattern, m_spriteImage, path, &error)) {
        QMessageBox::warning(this, tr("Save Failed"), error);
        return false;
    }
    m_currentProjectPath = path;
    m_undoStack.setClean();
    updateWindowTitle();
    return true;
}

bool MainWindow::confirmDiscardUnsavedChanges() {
    if (m_pattern.width() == 0 || m_undoStack.isClean()) return true;

    const auto choice = QMessageBox::question(
        this, tr("Unsaved Changes"), tr("This pattern has unsaved changes. Save before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

    if (choice == QMessageBox::Cancel) return false;
    if (choice == QMessageBox::Save) return saveProject();
    return true; // Discard
}

void MainWindow::loadPatternIntoUi() {
    m_gridView->setSpriteImage(m_spriteImage);
    m_spritePreview->setImage(m_spriteImage);
    m_gridView->zoomToFit();

    m_exportAction->setEnabled(true);
    m_saveProjectAction->setEnabled(true);
    updateStatusSummary();
    updateWindowTitle();
}

void MainWindow::updateWindowTitle() {
    if (m_pattern.width() == 0) {
        setWindowTitle(tr("SpriteStitcher"));
        return;
    }
    const QString name = m_pattern.name().isEmpty() ? tr("Untitled") : m_pattern.name();
    const QString dirty = m_undoStack.isClean() ? QString() : tr("*");
    setWindowTitle(tr("%1%2 — SpriteStitcher").arg(name, dirty));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscardUnsavedChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::exportPdf() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export PDF Chart"), m_pattern.name() + ".pdf", tr("PDF Files (*.pdf)"));
    if (path.isEmpty()) return;

    core::pdf::ExportOptions options;
    options.patternName = m_pattern.name();

    QString error;
    if (!core::pdf::PdfExporter::exportToPdf(m_pattern, m_dmcTable, path, options, &error)) {
        QMessageBox::warning(this, tr("Export Failed"), error);
    }
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

void MainWindow::onCellClicked(int x, int y, Qt::MouseButton button) {
    if (!m_pattern.isValidCoord(x, y)) return;

    if (button == Qt::LeftButton) {
        // Paint with the active color, if one is selected in the palette.
        const QString activeColor = m_toolPanel->activeColor();
        if (activeColor.isEmpty() || activeColor == m_pattern.cellAt(x, y).dmcCode) return;
        m_undoStack.push(new core::undo::RecolorCellCommand(&m_pattern, x, y, activeColor));
    } else if (button == Qt::RightButton) {
        // Eyedropper: pick up this cell's color as the new active color.
        const QString code = m_pattern.cellAt(x, y).dmcCode;
        if (!code.isEmpty()) m_toolPanel->setActiveColor(code);
    }
}

void MainWindow::onSwapRequested(const QString& fromCode, const QString& toCode) {
    m_undoStack.push(new core::undo::SwapColorCommand(&m_pattern, fromCode, toCode));
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
