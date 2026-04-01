#include "MainWindow.h"
#include "MapWidget.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    resize(1200, 800);
    setWindowTitle("S57 Chart Viewer");
}

void MainWindow::setupUI() {
    m_mapWidget = new MapWidget(this);
    setCentralWidget(m_mapWidget);

    connect(m_mapWidget, &MapWidget::coordinatesChanged,
            this, &MainWindow::onCoordinatesChanged);

    // Layer panel (right dock)
    QDockWidget* dock = new QDockWidget("Слои", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setMinimumWidth(160);

    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(8, 8, 8, 8);

    const QList<QPair<LayerType, QString>> layers = {
        {LayerType::Coastline,   "Береговая линия"},
        {LayerType::LandObjects, "Береговые объекты"},
        {LayerType::Beacons,     "Створные знаки"},
        {LayerType::Buoys,       "Буи"},
        {LayerType::Depths,      "Глубины"},
        {LayerType::Fairway,     "Фарватер"},
        {LayerType::NavLines,    "Створные линии"},
    };

    for (const auto& pair : layers) {
        LayerType lt = pair.first;

        QWidget* row = new QWidget(panel);
        QHBoxLayout* hl = new QHBoxLayout(row);
        hl->setContentsMargins(0, 1, 0, 1);
        hl->setSpacing(6);

        // Color swatch
        QLabel* swatch = new QLabel(row);
        swatch->setFixedSize(14, 14);
        QColor c = MapWidget::legendColor(lt);
        swatch->setStyleSheet(QString("background:%1; border:1px solid #888;").arg(c.name()));
        hl->addWidget(swatch);

        QCheckBox* cb = new QCheckBox(pair.second, row);
        cb->setChecked(true);
        connect(cb, &QCheckBox::toggled, [this, lt](bool checked) {
            m_mapWidget->setLayerVisible(lt, checked);
        });
        m_checkboxes[lt] = cb;
        hl->addWidget(cb);
        hl->addStretch();

        layout->addWidget(row);
    }

    m_namesCheckbox = new QCheckBox("Названия объектов", panel);
    m_namesCheckbox->setChecked(true);
    connect(m_namesCheckbox, &QCheckBox::toggled,
            m_mapWidget, &MapWidget::setNamesVisible);
    layout->addWidget(m_namesCheckbox);

    layout->addStretch();

    QPushButton* fitBtn = new QPushButton("Показать всё", panel);
    connect(fitBtn, &QPushButton::clicked, m_mapWidget, &MapWidget::fitAll);
    layout->addWidget(fitBtn);

    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    statusBar()->showMessage("Откройте файл карты (.000)");
}

void MainWindow::loadChart(const QString& filePath) {
    statusBar()->showMessage("Загрузка: " + filePath + " ...");
    qApp->processEvents();

    if (!m_loader.load(filePath)) {
        QMessageBox::critical(this, "Ошибка загрузки",
                              "Не удалось открыть карту:\n" + m_loader.error());
        statusBar()->showMessage("Ошибка загрузки");
        return;
    }

    m_mapWidget->setFeatures(m_loader.features(), m_loader.bounds());
    setWindowTitle("S57 Chart Viewer — " + QFileInfo(filePath).fileName());
    statusBar()->showMessage(
        QString("Загружено объектов: %1").arg(m_loader.features().size())
    );
}

void MainWindow::onCoordinatesChanged(double lon, double lat) {
    statusBar()->showMessage(
        QString("Долгота: %1°   Широта: %2°")
            .arg(lon, 0, 'f', 6)
            .arg(lat, 0, 'f', 6)
    );
}