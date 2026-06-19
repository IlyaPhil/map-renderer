#pragma once
#include <QWidget>
#include <QMap>
#include <QList>
#include <QPixmap>
#include "MapFeature.h"
#include "S57Loader.h"

class MapWidget : public QWidget {
    Q_OBJECT
public:
    // charts — список карт в формате base64 (пустой список — пустой виджет)
    explicit MapWidget(const QList<QString>& charts = {}, QWidget* parent = nullptr);

    // Добавить одну карту (строка base64, как от сервера)
    void addChart(const QString& base64data);

    // Очистить все загруженные карты и начать с нуля
    void clearCharts();

    // Количество загруженных объектов (для статусной строки)
    int featureCount() const { return m_features.size(); }

    void setLayerVisible(LayerType layer, bool visible);
    void setNamesVisible(bool visible);

    // Режим отображения навигационных знаков (буи и створные знаки)
    enum class MarkerMode { Shapes, Icons };

    // Переключить режим отображения знаков с перерисовкой
    void setMarkerMode(MarkerMode mode);

    // Установить размер иконки в пикселях с перерисовкой
    void setMarkerSize(int pixels);

    static QColor legendColor(LayerType layer);

public slots:
    void fitAll();

signals:
    void coordinatesChanged(double lon, double lat);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    S57Loader m_loader;              // накапливает объекты всех загруженных карт
    QVector<MapFeature> m_features;  // кэш объектов для отрисовки
    QRectF m_geoBounds;              // общий географический охват всех карт

    double m_scale = 1.0;   // пикселей на градус
    QPointF m_pan;          // смещение в пикселях

    bool m_panning = false;
    QPoint m_lastMousePos;

    QMap<LayerType, bool> m_layerVisible;
    bool m_showNames = true;

    MarkerMode m_markerMode = MarkerMode::Shapes;  // текущий режим знаков
    int m_markerSize = 24;                          // размер иконки в пикселях
    QMap<QString, QPixmap> m_iconCache;             // кэш загруженных/сгенерированных иконок

    // Вернуть иконку для S57-класса: сначала ищет файл icons/<класс>.png,
    // при отсутствии генерирует цветную заглушку
    QPixmap iconFor(const QString& s57class);

    QPointF geoToScreen(const QPointF& geo) const;
    QPointF screenToGeo(const QPointF& screen) const;
    void zoomAt(const QPointF& screenPos, double factor);
    void drawFeature(QPainter& painter, const MapFeature& f);

    struct LayerStyle {
        QColor color;
        QColor fillColor;
        int lineWidth;
    };
    static LayerStyle styleFor(LayerType layer);
};