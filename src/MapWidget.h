#pragma once
#include <QWidget>
#include <QMap>
#include "MapFeature.h"

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

    void setFeatures(const QVector<MapFeature>& features, const QRectF& bounds);
    void setLayerVisible(LayerType layer, bool visible);
    void setNamesVisible(bool visible);

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
    QVector<MapFeature> m_features;
    QRectF m_geoBounds;

    double m_scale = 1.0;   // pixels per degree
    QPointF m_pan;          // translation in pixels

    bool m_panning = false;
    QPoint m_lastMousePos;

    QMap<LayerType, bool> m_layerVisible;
    bool m_showNames = true;

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