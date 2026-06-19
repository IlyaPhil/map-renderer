#include "MapWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QCoreApplication>

MapWidget::MapWidget(const QList<QString>& charts, QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    m_layerVisible[LayerType::Coastline]   = true;
    m_layerVisible[LayerType::Beacons]     = true;
    m_layerVisible[LayerType::Buoys]       = true;
    m_layerVisible[LayerType::Depths]      = true;
    m_layerVisible[LayerType::Fairway]     = true;
    m_layerVisible[LayerType::NavLines]    = true;
    m_layerVisible[LayerType::LandObjects] = true;
    m_layerVisible[LayerType::Unknown]     = false;

    // Загружаем все карты из переданного списка base64-строк
    for (const QString& b64 : charts)
        addChart(b64);
}

void MapWidget::addChart(const QString& base64data) {
    // Запоминаем, была ли карта пустой до загрузки
    bool wasEmpty = m_features.isEmpty();

    if (!m_loader.loadFromBase64(base64data)) {
        qDebug() << "addChart: ошибка загрузки —" << m_loader.error();
        return;
    }

    // Синхронизируем кэш фич и bbox с загрузчиком (он накапливает всё внутри)
    m_features = m_loader.features();
    m_geoBounds = m_loader.bounds();

    if (wasEmpty)
        fitAll();  // первая карта — вписать в экран
    else
        update();  // последующие — не сбивать текущий вид
}

void MapWidget::clearCharts() {
    m_loader.clear();
    m_features.clear();
    m_geoBounds = QRectF();
    update();
}

void MapWidget::setLayerVisible(LayerType layer, bool visible) {
    m_layerVisible[layer] = visible;
    update();
}

void MapWidget::setNamesVisible(bool visible) {
    m_showNames = visible;
    update();
}

void MapWidget::setMarkerMode(MarkerMode mode) {
    m_markerMode = mode;
    update();
}

void MapWidget::setMarkerSize(int pixels) {
    m_markerSize = pixels;
    update();
}

// Возвращает иконку для заданного S57-класса.
// Сначала ищет PNG-файл в папке icons/ рядом с exe.
// Если файл не найден — генерирует цветную заглушку с аббревиатурой класса.
QPixmap MapWidget::iconFor(const QString& s57class) {
    if (s57class.isEmpty()) return QPixmap();

    // Возвращаем из кэша, если уже загружали
    if (m_iconCache.contains(s57class))
        return m_iconCache[s57class];

    // Пытаемся загрузить PNG из папки icons/ рядом с exe
    QString path = QCoreApplication::applicationDirPath()
                   + "/icons/" + s57class + ".png";
    QPixmap px(path);

    if (px.isNull()) {
        // Генерируем заглушку: цветной круг с аббревиатурой класса
        const int sz = 32;
        px = QPixmap(sz, sz);
        px.fill(Qt::transparent);

        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);

        // Буи — красные, навигационные знаки — оранжевые
        QColor fill = s57class.startsWith("BOY")
                      ? QColor(220, 30, 30)
                      : QColor(180, 80, 0);
        p.setBrush(fill);
        p.setPen(QPen(Qt::white, 1.5));
        p.drawEllipse(1, 1, sz - 2, sz - 2);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRect(0, 0, sz, sz), Qt::AlignCenter, s57class.left(3));
    }

    m_iconCache[s57class] = px;
    return px;
}

void MapWidget::fitAll() {
    if (m_geoBounds.isEmpty() || width() <= 0 || height() <= 0) return;

    double scaleX = width()  / m_geoBounds.width();
    double scaleY = height() / m_geoBounds.height();
    m_scale = qMin(scaleX, scaleY) * 0.9;

    double centerLon = m_geoBounds.left() + m_geoBounds.width()  / 2.0;
    double centerLat = m_geoBounds.top()  + m_geoBounds.height() / 2.0;

    m_pan.setX(width()  / 2.0 - centerLon * m_scale);
    m_pan.setY(height() / 2.0 + centerLat * m_scale);

    update();
}

QPointF MapWidget::geoToScreen(const QPointF& geo) const {
    return QPointF(
         geo.x() * m_scale + m_pan.x(),
        -geo.y() * m_scale + m_pan.y()
    );
}

QPointF MapWidget::screenToGeo(const QPointF& screen) const {
    return QPointF(
         (screen.x() - m_pan.x()) / m_scale,
        -(screen.y() - m_pan.y()) / m_scale
    );
}

void MapWidget::zoomAt(const QPointF& screenPos, double factor) {
    QPointF geo = screenToGeo(screenPos);
    m_scale *= factor;
    m_pan.setX(screenPos.x() - geo.x() * m_scale);
    m_pan.setY(screenPos.y() + geo.y() * m_scale);
    update();
}

MapWidget::LayerStyle MapWidget::styleFor(LayerType layer) {
    switch (layer) {
        case LayerType::Coastline:   return {QColor(50,  100, 180), QColor(210, 200, 160), 1};
        case LayerType::Buoys:       return {QColor(220, 30,  30),  QColor(220, 30,  30),  1};
        case LayerType::Beacons:     return {QColor(180, 80,  0),   QColor(180, 80,  0),   1};
        case LayerType::Depths:      return {QColor(0,   80,  180), QColor(160, 210, 235), 1};
        case LayerType::Fairway:     return {QColor(0,   140, 90),  QColor(190, 235, 210), 1};
        case LayerType::NavLines:    return {QColor(130, 0,   150), Qt::transparent,        1};
        case LayerType::LandObjects: return {QColor(90,  60,  30),  QColor(180, 155, 110), 1};
        default:                     return {QColor(120, 120, 120), Qt::transparent,        1};
    }
}

QColor MapWidget::legendColor(LayerType layer) {
    LayerStyle s = styleFor(layer);
    return s.fillColor.isValid() && s.fillColor != Qt::transparent
           ? s.fillColor : s.color;
}

// Цвет воды в зависимости от минимальной глубины (DRVAL1)
static QColor depthColor(double drval1) {
    if (drval1 <= 0)  return QColor(175, 220, 235);
    if (drval1 < 2)   return QColor(155, 208, 228);
    if (drval1 < 5)   return QColor(130, 192, 220);
    if (drval1 < 10)  return QColor(100, 170, 210);
    if (drval1 < 20)  return QColor(72,  145, 195);
    if (drval1 < 50)  return QColor(50,  118, 178);
    return                   QColor(35,  90,  160);
}

void MapWidget::drawFeature(QPainter& painter, const MapFeature& f) {
    if (f.points.isEmpty()) return;

    LayerStyle style = styleFor(f.layer);

    QVector<QPointF> screen;
    screen.reserve(f.points.size());
    for (const auto& geo : f.points)
        screen.append(geoToScreen(geo));

    switch (f.geomType) {
        case MapFeature::Area: {
            QColor fill = (f.layer == LayerType::Depths)
                          ? depthColor(f.depth) : style.fillColor;
            painter.setPen(QPen(style.color, style.lineWidth));
            painter.setBrush(QBrush(fill));
            painter.drawPolygon(QPolygonF(screen));
            break;
        }
        case MapFeature::Line: {
            QPen pen(style.color, style.lineWidth);
            if (f.layer == LayerType::NavLines)
                pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(screen.constData(), screen.size());
            break;
        }
        case MapFeature::Point: {
            QPointF pt = screen[0];

            // Режим иконок: буи и навигационные знаки рисуются как PNG
            if (m_markerMode == MarkerMode::Icons &&
                (f.layer == LayerType::Beacons || f.layer == LayerType::Buoys)) {
                QPixmap icon = iconFor(f.s57class);
                if (!icon.isNull()) {
                    int half = m_markerSize / 2;
                    painter.drawPixmap(
                        QRect(pt.x() - half, pt.y() - half,
                              m_markerSize, m_markerSize),
                        icon
                    );
                    break;
                }
            }

            // Режим фигур (по умолчанию)
            painter.setPen(QPen(style.color, 1));

            if (f.layer == LayerType::Depths) {
                painter.setBrush(QColor(0, 80, 180));
                painter.drawEllipse(pt, 2, 2);
                if (f.depth != 0.0 && m_scale > 2000) {
                    painter.setPen(QColor(0, 50, 140));
                    painter.setFont(QFont("Arial", 8));
                    painter.drawText(pt + QPointF(4, -3),
                                     QString::number(f.depth, 'f', 1));
                }
            } else if (f.layer == LayerType::Buoys) {
                painter.setBrush(style.color);
                painter.drawEllipse(pt, 5, 5);
            } else if (f.layer == LayerType::Beacons) {
                QPolygonF tri;
                tri << pt + QPointF(0, -8)
                    << pt + QPointF(6,  4)
                    << pt + QPointF(-6, 4);
                painter.setBrush(style.color);
                painter.drawPolygon(tri);
            } else if (f.layer == LayerType::LandObjects) {
                painter.setBrush(QColor(150, 110, 60));
                painter.drawRect(QRectF(pt - QPointF(4, 4), QSizeF(8, 8)));
            } else {
                painter.setBrush(style.color);
                painter.drawEllipse(pt, 3, 3);
            }
            break;
        }
    }

    // Подпись названия объекта для точечных объектов
    if (m_showNames && !f.name.isEmpty() && f.geomType == MapFeature::Point) {
        QPointF pt = screen[0];
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 8));
        painter.drawText(pt + QPointF(9, 4), f.name);
    }
}

void MapWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), QColor(195, 220, 240));

    // Полигоны: явный z-порядок, чтобы суша перекрывала зоны глубин
    static const QList<LayerType> areaOrder = {
        LayerType::Depths,
        LayerType::Coastline,
        LayerType::Fairway,
        LayerType::LandObjects,
        LayerType::Unknown,
    };
    for (LayerType lt : areaOrder) {
        if (!m_layerVisible.value(lt, false)) continue;
        for (const auto& f : m_features)
            if (f.layer == lt && f.geomType == MapFeature::Area)
                drawFeature(painter, f);
    }

    for (const auto& f : m_features) {
        if (!m_layerVisible.value(f.layer, false)) continue;
        if (f.geomType == MapFeature::Line) drawFeature(painter, f);
    }
    for (const auto& f : m_features) {
        if (!m_layerVisible.value(f.layer, false)) continue;
        if (f.geomType == MapFeature::Point) drawFeature(painter, f);
    }
}

void MapWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_panning = true;
        m_lastMousePos = e->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* e) {
    QPointF geo = screenToGeo(e->pos());
    emit coordinatesChanged(geo.x(), geo.y());

    if (m_panning) {
        QPoint delta = e->pos() - m_lastMousePos;
        m_pan += delta;
        m_lastMousePos = e->pos();
        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton)
        zoomAt(e->pos(), 2.0);
}

void MapWidget::wheelEvent(QWheelEvent* e) {
    double factor = e->angleDelta().y() > 0 ? 1.25 : 0.8;
    zoomAt(e->position(), factor);
}

void MapWidget::resizeEvent(QResizeEvent*) {
    if (!m_geoBounds.isEmpty())
        fitAll();
}