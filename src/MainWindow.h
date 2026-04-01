#pragma once
#include <QMainWindow>
#include <QMap>
#include "MapFeature.h"
#include "S57Loader.h"

class MapWidget;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void loadChart(const QString& filePath);

private:
    MapWidget* m_mapWidget = nullptr;
    S57Loader  m_loader;
    QMap<LayerType, QCheckBox*> m_checkboxes;
    QCheckBox* m_namesCheckbox = nullptr;

    void setupUI();
    void onCoordinatesChanged(double lon, double lat);
};