#include "ImportOptionsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace ss::ui {

ImportOptionsDialog::ImportOptionsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Import Options"));

    m_exactRadio = new QRadioButton(tr("Exact colors"));
    m_exactRadio->setChecked(true);
    m_exactRadio->setToolTip(tr("Match every distinct source color to its own DMC floss.\n"
                                 "Best for pixel art that's already low-color."));

    m_quantizeRadio = new QRadioButton(tr("Reduce to a target color count"));
    m_quantizeRadio->setToolTip(tr("Quantize the source image before DMC matching.\n"
                                    "Best for photos or noisy source art."));

    m_colorCountSpin = new QSpinBox;
    m_colorCountSpin->setRange(2, 256);
    m_colorCountSpin->setValue(32);
    m_colorCountSpin->setEnabled(false);

    connect(m_quantizeRadio, &QRadioButton::toggled, m_colorCountSpin, &QSpinBox::setEnabled);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* form = new QFormLayout;
    form->addRow(m_exactRadio);
    form->addRow(m_quantizeRadio);
    form->addRow(tr("Target colors:"), m_colorCountSpin);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

ImportOptionsDialog::Mode ImportOptionsDialog::mode() const {
    return m_quantizeRadio->isChecked() ? Mode::QuantizeToN : Mode::Exact;
}

int ImportOptionsDialog::targetColorCount() const {
    return m_colorCountSpin->value();
}

} // namespace ss::ui
