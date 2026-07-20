#include "SpritePreviewPanel.h"

#include <QLabel>
#include <QPainter>

#include <algorithm>

namespace ss::ui {

// Renders at an integer nearest-neighbor scale so sprite pixels stay crisp
// rather than getting blurred or aliased by Qt's default smooth scaling.
class SpritePreviewPanel::ImageLabel : public QWidget {
public:
    void setImage(const QImage& image) {
        m_image = image;
        updateGeometry();
        update();
    }

    QSize sizeHint() const override {
        if (m_image.isNull()) return QSize(0, 0);
        const int scale = std::max(1, displayScale());
        return m_image.size() * scale;
    }

protected:
    void paintEvent(QPaintEvent*) override {
        if (m_image.isNull()) return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(rect(), m_image);
    }

private:
    int displayScale() const {
        if (m_image.isNull() || m_image.width() == 0) return 1;
        // Aim for a reasonably large reference preview without needing to
        // scroll for typical small sprites.
        constexpr int kTargetWidth = 512;
        return std::max(1, kTargetWidth / m_image.width());
    }

    QImage m_image;
};

SpritePreviewPanel::SpritePreviewPanel(QWidget* parent)
    : QScrollArea(parent), m_label(new ImageLabel) {
    setWidget(m_label);
    setWidgetResizable(false);
    setAlignment(Qt::AlignCenter);
    setBackgroundRole(QPalette::Dark);
}

void SpritePreviewPanel::setImage(const QImage& image) {
    m_label->setImage(image);
}

} // namespace ss::ui
