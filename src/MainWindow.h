#pragma once
#include <QMainWindow>
#include <QMap>
#include "MapFeature.h"

class MapWidget;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void loadChart(const QString& filePath);

private:
    MapWidget* m_mapWidget = nullptr;
    QMap<LayerType, QCheckBox*> m_checkboxes;
    QCheckBox* m_namesCheckbox = nullptr;

    void setupUI();
    void onCoordinatesChanged(double lon, double lat);
};