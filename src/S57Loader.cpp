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

void S57Loader::clear() {
    m_features.clear();
    m_bounds = QRectF();
    m_error.clear();
}

// Пересчёт общего географического охвата по всем накопленным объектам
void S57Loader::recomputeBounds() {
    double minLon =  std::numeric_limits<double>::max();
    double minLat =  std::numeric_limits<double>::max();
    double maxLon = -std::numeric_limits<double>::max();
    double maxLat = -std::numeric_limits<double>::max();

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
}

// Общий код обхода слоёв GDAL-датасета и извлечения объектов в m_features.
// Вызывается как из load(), так и из loadFromBase64().
void S57Loader::processDataset(GDALDataset* ds) {
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

            // Передаём className в processGeometry для сохранения в MapFeature::s57class
            processGeometry(geom, name, depth, layerType, className);
            OGRFeature::DestroyFeature(feat);
        }
    }
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

    processDataset(ds);
    GDALClose(ds);
    recomputeBounds();

    qDebug() << "Загружено из файла" << m_features.size() << "объектов, bounds:" << m_bounds;
    return !m_features.isEmpty();
}

bool S57Loader::loadFromBase64(const QString& base64) {
    CPLSetConfigOption("GDAL_DATA", "C:/msys64/mingw64/share/gdal");
    GDALAllRegister();

    // Декодируем строку base64 в бинарные данные S57-файла
    QByteArray data = QByteArray::fromBase64(base64.toLatin1());
    if (data.isEmpty()) {
        m_error = "Пустые данные после декодирования base64";
        return false;
    }

    // Путь во временной виртуальной файловой системе GDAL (только в памяти)
    const char* vsiPath = "/vsimem/s57_import.000";

    // Регистрируем буфер как виртуальный файл.
    // FALSE означает: владелец памяти — мы (QByteArray), GDAL только читает.
    VSILFILE* vsiFile = VSIFileFromMemBuffer(
        vsiPath,
        reinterpret_cast<GByte*>(data.data()),
        static_cast<vsi_l_offset>(data.size()),
        FALSE
    );
    if (!vsiFile) {
        m_error = "Не удалось создать виртуальный файл GDAL из base64-данных";
        return false;
    }
    VSIFCloseL(vsiFile);  // закрываем дескриптор — GDAL открывает файл сам через GDALOpenEx

    GDALDataset* ds = static_cast<GDALDataset*>(
        GDALOpenEx(vsiPath, GDAL_OF_VECTOR, nullptr, nullptr, nullptr)
    );

    if (!ds) {
        VSIUnlink(vsiPath);  // убираем виртуальный файл даже при ошибке
        m_error = "Не удалось разобрать S57 из base64-данных";
        return false;
    }

    processDataset(ds);
    GDALClose(ds);
    VSIUnlink(vsiPath);  // освобождаем виртуальный файл из памяти GDAL
    recomputeBounds();

    qDebug() << "Загружено из base64, всего объектов:" << m_features.size() << ", bounds:" << m_bounds;
    return true;
}

void S57Loader::processGeometry(OGRGeometry* geom, const QString& name,
                                 double depth, LayerType layer,
                                 const QString& s57class) {
    if (!geom) return;

    OGRwkbGeometryType gtype = wkbFlatten(geom->getGeometryType());

    switch (gtype) {
        case wkbPoint: {
            OGRPoint* pt = static_cast<OGRPoint*>(geom);
            MapFeature f;
            f.geomType  = MapFeature::Point;
            f.points.append(QPointF(pt->getX(), pt->getY()));
            f.name      = name;
            f.layer     = layer;
            f.s57class  = s57class;
            f.depth     = (geom->getGeometryType() == wkbPoint25D)
                          ? pt->getZ() : depth;
            m_features.append(f);
            break;
        }
        case wkbMultiPoint: {
            OGRMultiPoint* mp = static_cast<OGRMultiPoint*>(geom);
            for (int j = 0; j < mp->getNumGeometries(); j++)
                processGeometry(mp->getGeometryRef(j), name, depth, layer, s57class);
            break;
        }
        case wkbLineString: {
            OGRLineString* ls = static_cast<OGRLineString*>(geom);
            MapFeature f;
            f.geomType = MapFeature::Line;
            for (int j = 0; j < ls->getNumPoints(); j++)
                f.points.append(QPointF(ls->getX(j), ls->getY(j)));
            f.name     = name;
            f.layer    = layer;
            f.s57class = s57class;
            f.depth    = depth;
            if (!f.points.isEmpty())
                m_features.append(f);
            break;
        }
        case wkbMultiLineString: {
            OGRMultiLineString* mls = static_cast<OGRMultiLineString*>(geom);
            for (int j = 0; j < mls->getNumGeometries(); j++)
                processGeometry(mls->getGeometryRef(j), name, depth, layer, s57class);
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
            f.name     = name;
            f.layer    = layer;
            f.s57class = s57class;
            f.depth    = depth;
            if (!f.points.isEmpty())
                m_features.append(f);
            break;
        }
        case wkbMultiPolygon: {
            OGRMultiPolygon* mpoly = static_cast<OGRMultiPolygon*>(geom);
            for (int j = 0; j < mpoly->getNumGeometries(); j++)
                processGeometry(mpoly->getGeometryRef(j), name, depth, layer, s57class);
            break;
        }
        default:
            break;
    }
}