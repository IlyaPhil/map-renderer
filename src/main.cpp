#include <QApplication>
#include <QFileDialog>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QString filePath;
    if (argc > 1) {
        filePath = QString::fromLocal8Bit(argv[1]);
    } else {
        filePath = QFileDialog::getOpenFileName(
            nullptr,
            "Открыть карту S57",
            "",
            "S57 Charts (*.000);;Все файлы (*)"
        );
        if (filePath.isEmpty()) return 0;
    }

    MainWindow window;
    window.show();
    window.loadChart(filePath);

    return app.exec();
}