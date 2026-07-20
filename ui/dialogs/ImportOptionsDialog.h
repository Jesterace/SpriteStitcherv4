#pragma once

#include <QDialog>

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

private:
    QRadioButton* m_exactRadio;
    QRadioButton* m_quantizeRadio;
    QSpinBox* m_colorCountSpin;
};

} // namespace ss::ui
