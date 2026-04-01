#pragma once
#include <QVector>
#include <QString>
#include <QRectF>
#include "MapFeature.h"

struct OGRGeometry;

class S57Loader {
public:
    bool load(const QString& filePath);

    const QVector<MapFeature>& features() const { return m_features; }
    QRectF bounds() const { return m_bounds; }
    QString error() const { return m_error; }

private:
    QVector<MapFeature> m_features;
    QRectF m_bounds;
    QString m_error;

    static LayerType classifyLayer(const QString& className);
    void processGeometry(OGRGeometry* geom, const QString& name,
                         double depth, LayerType layer);
};