#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SpriteStitcher");
    QApplication::setOrganizationName("SpriteStitcher");
    QApplication::setWindowIcon(QIcon(":/icons/spritestitcher.png"));

    ss::ui::MainWindow window;
    window.show();

    return QApplication::exec();
}
