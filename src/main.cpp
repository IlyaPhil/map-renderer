#include <QApplication>
#include <QFileDialog>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    if (argc > 1) {
        // Загружаем все файлы, переданные аргументами командной строки
        for (int i = 1; i < argc; i++)
            window.loadChart(QString::fromLocal8Bit(argv[i]));
    } else {
        QStringList filePaths = QFileDialog::getOpenFileNames(
            nullptr,
            "Открыть карты S57",
            "",
            "S57 Charts (*.000);;Все файлы (*)"
        );
        for (const QString& filePath : filePaths)
            window.loadChart(filePath);
    }

    return app.exec();
}