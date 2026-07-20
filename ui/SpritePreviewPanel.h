#pragma once

#include <QImage>
#include <QScrollArea>

namespace ss::ui {

// Read-only, native-scale (nearest-neighbor) view of the originally
// imported sprite, for reference alongside the reduced pattern.
class SpritePreviewPanel : public QScrollArea {
    Q_OBJECT

public:
    explicit SpritePreviewPanel(QWidget* parent = nullptr);

    void setImage(const QImage& image);

private:
    class ImageLabel;
    ImageLabel* m_label;
};

} // namespace ss::ui
