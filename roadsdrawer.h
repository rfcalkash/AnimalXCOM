#ifndef ROADSDRAWER_H
#define ROADSDRAWER_H

#include "macros.h"
#include "terraingenerator.h"
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QQmlEngine>
#include <QQuickPaintedItem>
#include <QSvgRenderer>
#include <QtConcurrent>

class RoadsDrawer : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    AUTO_PROPERTY_ADDITIONALWRITER(int, fieldWidth, _generate, 500)
    AUTO_PROPERTY_ADDITIONALWRITER(int, fieldHeight, _generate, 500)
    AUTO_PROPERTY_ADDITIONALWRITER(qint32, seed, _generate, 1)
    AUTO_PROPERTY(bool, busy, false)
    AUTO_PROPERTY_WRITER(int, zoom, zoom, 100)
    AUTO_PROPERTY(int, progress, 0)

    QScopedPointer<TerrainGenerator> m_generator;
    void _generate();
    QTimer reAskTimer;
    void updateImplicits();
    void _drawBlock(const QSharedPointer<Block>& block, const QRectF& rect, QPainter* p);
    static bool _isVerticalEdge(Qt::Edge type);
    int m_lastDrawnProgress = 0;
    bool m_moving = false;
    QPointF m_lastMovePosition = QPointF(0, 0);
    QPointF m_topLeftPosition = QPointF(0, 0);
    static QImage _loadImageFromSvg(const QString& path, const QSize& scaleSize);
    QImage parking;
    QImage turn;
    QImage straight;
    QImage cross;
    QImage crossT;
    QImage crossWalk;
    QImage trafficLight;
    void _drawCrossWalk(const QRectF& rect, QPainter* p, Qt::Edge edge);

public:
    explicit RoadsDrawer(QQuickItem* parent = nullptr);
    Q_INVOKABLE void redraw();
    void zoom(int newValue);

    // QQuickPaintedItem interface
public:
    void paint(QPainter* painter) override;

    // QQuickItem interface
protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // QQuickItem interface
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};

#endif // ROADSDRAWER_H
