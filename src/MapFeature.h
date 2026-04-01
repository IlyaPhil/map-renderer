#pragma once
#include <QVector>
#include <QPointF>
#include <QString>

enum class LayerType {
    Coastline,
    Beacons,
    Buoys,
    Depths,
    Fairway,
    NavLines,
    LandObjects,
    Unknown
};

inline QString layerDisplayName(LayerType t) {
    switch (t) {
        case LayerType::Coastline:   return "Береговая линия";
        case LayerType::Beacons:     return "Створные знаки";
        case LayerType::Buoys:       return "Буи";
        case LayerType::Depths:      return "Глубины";
        case LayerType::Fairway:     return "Фарватер";
        case LayerType::NavLines:    return "Створные линии";
        case LayerType::LandObjects: return "Береговые объекты";
        default:                     return "Прочее";
    }
}

struct MapFeature {
    enum GeomType { Point, Line, Area };
    GeomType geomType = Point;
    QVector<QPointF> points;  // (lon, lat)
    QString name;             // OBJNAM attribute
    double depth = 0.0;       // sounding depth or DRVAL1 for DEPARE
    LayerType layer = LayerType::Unknown;
};