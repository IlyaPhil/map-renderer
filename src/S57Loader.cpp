#include "S57Loader.h"
#include <ogrsf_frmts.h>
#include <QMap>
#include <QDebug>
#include <limits>

LayerType S57Loader::classifyLayer(const QString& name) {
    static const QMap<QString, LayerType> mapping = {
        {"COALNE", LayerType::Coastline},
        {"LNDARE", LayerType::Coastline},
        {"SLCONS", LayerType::Coastline},
        {"BOYLAT", LayerType::Buoys},
        {"BOYCAR", LayerType::Buoys},
        {"BOYISD", LayerType::Buoys},
        {"BOYSAW", LayerType::Buoys},
        {"BOYSPP", LayerType::Buoys},
        {"BOYINB", LayerType::Buoys},
        {"BCNLAT", LayerType::Beacons},
        {"BCNCAR", LayerType::Beacons},
        {"BCNISD", LayerType::Beacons},
        {"BCNSAW", LayerType::Beacons},
        {"BCNSPP", LayerType::Beacons},
        {"LIGHTS", LayerType::Beacons},
        {"SOUNDG", LayerType::Depths},
        {"DEPARE", LayerType::Depths},
        {"DEPCNT", LayerType::Depths},
        {"FAIRWY", LayerType::Fairway},
        {"RECTRC", LayerType::Fairway},
        {"TSSLPT", LayerType::Fairway},
        {"TSSRON", LayerType::Fairway},
        {"NAVLNE", LayerType::NavLines},
        {"TRSLNE", LayerType::NavLines},
        {"LNDMRK", LayerType::LandObjects},
        {"BUAARE", LayerType::LandObjects},
        {"BRIDGE", LayerType::LandObjects},
        {"MORFAC", LayerType::LandObjects},
        {"HULKES", LayerType::LandObjects},
        {"LOKBSN", LayerType::LandObjects},
        {"HRBARE", LayerType::LandObjects},
        {"DOCARE", LayerType::LandObjects},
        {"PRTARE", LayerType::LandObjects},
    };
    return mapping.value(name, LayerType::Unknown);
}

bool S57Loader::load(const QString& filePath) {
    CPLSetConfigOption("GDAL_DATA", "C:/msys64/mingw64/share/gdal");
    GDALAllRegister();

    GDALDataset* ds = static_cast<GDALDataset*>(
        GDALOpenEx(filePath.toUtf8().constData(), GDAL_OF_VECTOR,
                   nullptr, nullptr, nullptr)
    );

    if (!ds) {
        m_error = "Не удалось открыть файл: " + filePath;
        return false;
    }

    double minLon =  std::numeric_limits<double>::max();
    double minLat =  std::numeric_limits<double>::max();
    double maxLon = -std::numeric_limits<double>::max();
    double maxLat = -std::numeric_limits<double>::max();

    for (int i = 0; i < ds->GetLayerCount(); i++) {
        OGRLayer* ogrLayer = ds->GetLayer(i);
        if (!ogrLayer) continue;

        QString className = QString::fromUtf8(ogrLayer->GetName());
        LayerType layerType = classifyLayer(className);

        ogrLayer->ResetReading();
        OGRFeature* feat;

        while ((feat = ogrLayer->GetNextFeature()) != nullptr) {
            OGRGeometry* geom = feat->GetGeometryRef();
            if (!geom) {
                OGRFeature::DestroyFeature(feat);
                continue;
            }

            QString name;
            int nameIdx = feat->GetFieldIndex("OBJNAM");
            if (nameIdx >= 0 && feat->IsFieldSet(nameIdx))
                name = QString::fromUtf8(feat->GetFieldAsString(nameIdx));

            double depth = 0.0;

            if (className == "SOUNDG") {
                int idx = feat->GetFieldIndex("VALSOU");
                if (idx >= 0 && feat->IsFieldSet(idx))
                    depth = feat->GetFieldAsDouble(idx);
            } else if (className == "DEPARE") {
                int idx = feat->GetFieldIndex("DRVAL1");
                if (idx >= 0 && feat->IsFieldSet(idx))
                    depth = feat->GetFieldAsDouble(idx);
            }

            processGeometry(geom, name, depth, layerType);
            OGRFeature::DestroyFeature(feat);
        }
    }

    GDALClose(ds);

    // Compute bounding box
    for (const auto& f : m_features) {
        for (const auto& pt : f.points) {
            minLon = qMin(minLon, pt.x());
            maxLon = qMax(maxLon, pt.x());
            minLat = qMin(minLat, pt.y());
            maxLat = qMax(maxLat, pt.y());
        }
    }

    if (!m_features.isEmpty())
        m_bounds = QRectF(QPointF(minLon, minLat), QPointF(maxLon, maxLat));

    qDebug() << "Loaded" << m_features.size() << "features, bounds:" << m_bounds;
    return !m_features.isEmpty();
}

void S57Loader::processGeometry(OGRGeometry* geom, const QString& name,
                                 double depth, LayerType layer) {
    if (!geom) return;

    OGRwkbGeometryType gtype = wkbFlatten(geom->getGeometryType());

    switch (gtype) {
        case wkbPoint: {
            OGRPoint* pt = static_cast<OGRPoint*>(geom);
            MapFeature f;
            f.geomType = MapFeature::Point;
            f.points.append(QPointF(pt->getX(), pt->getY()));
            f.name = name;
            f.layer = layer;
            f.depth = (geom->getGeometryType() == wkbPoint25D)
                      ? pt->getZ() : depth;
            m_features.append(f);
            break;
        }
        case wkbMultiPoint: {
            OGRMultiPoint* mp = static_cast<OGRMultiPoint*>(geom);
            for (int j = 0; j < mp->getNumGeometries(); j++)
                processGeometry(mp->getGeometryRef(j), name, depth, layer);
            break;
        }
        case wkbLineString: {
            OGRLineString* ls = static_cast<OGRLineString*>(geom);
            MapFeature f;
            f.geomType = MapFeature::Line;
            for (int j = 0; j < ls->getNumPoints(); j++)
                f.points.append(QPointF(ls->getX(j), ls->getY(j)));
            f.name = name;
            f.layer = layer;
            f.depth = depth;
            if (!f.points.isEmpty())
                m_features.append(f);
            break;
        }
        case wkbMultiLineString: {
            OGRMultiLineString* mls = static_cast<OGRMultiLineString*>(geom);
            for (int j = 0; j < mls->getNumGeometries(); j++)
                processGeometry(mls->getGeometryRef(j), name, depth, layer);
            break;
        }
        case wkbPolygon: {
            OGRPolygon* poly = static_cast<OGRPolygon*>(geom);
            OGRLinearRing* ring = poly->getExteriorRing();
            if (!ring) break;
            MapFeature f;
            f.geomType = MapFeature::Area;
            for (int j = 0; j < ring->getNumPoints(); j++)
                f.points.append(QPointF(ring->getX(j), ring->getY(j)));
            f.name = name;
            f.layer = layer;
            f.depth = depth;
            if (!f.points.isEmpty())
                m_features.append(f);
            break;
        }
        case wkbMultiPolygon: {
            OGRMultiPolygon* mpoly = static_cast<OGRMultiPolygon*>(geom);
            for (int j = 0; j < mpoly->getNumGeometries(); j++)
                processGeometry(mpoly->getGeometryRef(j), name, depth, layer);
            break;
        }
        default:
            break;
    }
}