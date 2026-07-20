#pragma once

#include <QDialog>

class QCheckBox;
class QRadioButton;
class QSpinBox;

namespace ss::ui {

// One of the two deliberately-kept dialogs (the other is PDF export
// options) — everything else in the app happens inline in the ToolPanel or
// directly on the grid.
class ImportOptionsDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode { Exact, QuantizeToN };

    explicit ImportOptionsDialog(QWidget* parent = nullptr);

    Mode mode() const;
    int targetColorCount() const;

    // Whether to auto-detect and drop a flat background color (sampled
    // from the image's corners) so it becomes unstitched cells instead
    // of a stitched color. Real alpha transparency in the source image
    // is always treated as unstitched regardless of this option.
    bool removeBackground() const;

private:
    QRadioButton* m_exactRadio;
    QRadioButton* m_quantizeRadio;
    QSpinBox* m_colorCountSpin;
    QCheckBox* m_removeBackgroundCheck;
};

} // namespace ss::ui
