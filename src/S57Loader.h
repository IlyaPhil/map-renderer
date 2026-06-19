#pragma once
#include <QVector>
#include <QString>
#include <QRectF>
#include "MapFeature.h"

struct OGRGeometry;
class GDALDataset;

class S57Loader {
public:
    // Загрузка карты из файла на диске
    bool load(const QString& filePath);

    // Загрузка карты из строки base64 (данные, полученные от сервера)
    // Каждый вызов добавляет объекты к уже загруженным картам
    bool loadFromBase64(const QString& base64);

    // Очистить все загруженные данные
    void clear();

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

    // Обход всех слоёв открытого датасета GDAL и извлечение объектов
    void processDataset(GDALDataset* ds);

    // Пересчёт общего bbox по всем накопленным объектам
    void recomputeBounds();
};