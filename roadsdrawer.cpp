#include "roadsdrawer.h"

void RoadsDrawer::_generate()
{
    updateImplicits();
    m_lastDrawnProgress = 0;
    busy(true);
    QtConcurrent::run([this]() {
        m_generator.reset(new TerrainGenerator(fieldWidth(), fieldHeight(), seed()));
        m_generator->generate();
    }).then([this]() { busy(false); });
}

void RoadsDrawer::updateImplicits()
{
    const QSize imageSize { zoom(), zoom() };
    parking = _loadImageFromSvg("://images/road-parking.svg", imageSize);
    turn = _loadImageFromSvg("://images/road-turn.svg", imageSize);
    straight = _loadImageFromSvg("://images/road-straight.svg", imageSize);
    cross = _loadImageFromSvg("://images/road-crossroads.svg", imageSize);
    crossT = _loadImageFromSvg("://images/road-t-junction.svg", imageSize);
    setImplicitSize(fieldWidth() * zoom(), fieldHeight() * zoom());
}

void RoadsDrawer::_drawBlock(const QSharedPointer<Block>& block, const QRectF& rect, QPainter* p)
{
    static QMap<TerrainType, QColor> colorMap {
        { TerrainType::Empty, "#90B77D" },
        { TerrainType::Road, "#9B9B9B" },
        { TerrainType::Building, "#D49B54" },
        { TerrainType::River, "#5B8FB9" },
        { TerrainType::Forest, "#2D5A27" }
    };

    if (!block->collapsed || rect.width() < 3) {
        p->fillRect(rect, !block->collapsed ? fillColor() : colorMap.value(block->getCenterElement()));
        return;
    }
    p->fillRect(rect, colorMap.value(TerrainType::Empty));
    const auto center = rect.center();
    if (rect.width() < 9 || block->getCenterElement() != TerrainType::Road) {
        p->fillRect(rect.adjusted(rect.width() / 4, rect.height() / 4, -rect.width() / 4, -rect.height() / 4), colorMap.value(block->getCenterElement()));
        for (auto edge : TerrainGenerator::allEdges) {
            if (block->getSideElement(edge) != TerrainType::Empty) {
                QPointF tl(rect.x() + rect.width() / 4, rect.y() + rect.height() / 4);
                QPointF br;
                switch (edge) {
                case Qt::TopEdge:
                    br = QPointF { rect.x() + rect.width() * 3 / 4, rect.top() };
                    break;
                case Qt::LeftEdge:
                    br = QPointF { rect.left(), rect.y() + rect.height() * 3 / 4 };
                    break;
                case Qt::RightEdge:
                    tl.rx() += rect.width() / 2;
                    br = QPointF { rect.x() + rect.width(), rect.y() + rect.height() * 3 / 4 };
                    break;
                case Qt::BottomEdge:
                    tl.ry() += rect.height() / 2;
                    br = QPointF { rect.x() + rect.width() * 3 / 4, rect.y() + rect.height() };
                    break;
                }
                p->fillRect(QRectF(tl, br), colorMap.value(block->getSideElement(edge)));
            }
        }
        for (auto corner : TerrainGenerator::allCorners) {
            if (block->getFilledCorner(corner)) {
                auto xShift = TerrainGenerator::cornerShift(corner).x();
                auto yShift = TerrainGenerator::cornerShift(corner).y();
                QPointF tl = center + QPointF(xShift * rect.width() * 3 / 8, yShift * rect.height() * 3 / 8) - QPointF(rect.width() / 8, rect.height() / 8);
                p->fillRect(QRectF(tl, tl + QPointF(rect.width() / 4, rect.height() / 4)), colorMap.value(block->getCenterElement()));
            }
        }
    } else {
        QVector<Qt::Edge> roads;
        for (auto direction : { Qt::TopEdge, Qt::BottomEdge, Qt::LeftEdge, Qt::RightEdge }) {
            const auto type = block->getSideElement(direction);
            if (type == TerrainType::Road) {
                roads.append(direction);
                continue;
            }
            QPainterPath path;
            path.moveTo(center);
            const QPointF p1(direction != Qt::RightEdge ? rect.left() : rect.right(), direction != Qt::BottomEdge ? rect.top() : rect.bottom());
            const QPointF p2(_isVerticalEdge(direction) ? p1.x() : rect.right(), _isVerticalEdge(direction) ? rect.bottom() : p1.y());
            path.lineTo(p1);
            path.lineTo(p2);
            path.lineTo(center);
            p->fillPath(path, colorMap.value(type));
        }
        if (roads.size() == 1) {
            switch (roads.at(0)) {
            case Qt::TopEdge:
                p->drawImage(rect, parking.transformed(QTransform().rotate(90)));
                break;
            case Qt::BottomEdge:
                p->drawImage(rect, parking.transformed(QTransform().rotate(-90)));
                break;
            case Qt::LeftEdge:
                p->drawImage(rect, parking);
                break;
            case Qt::RightEdge:
                p->drawImage(rect, parking.transformed(QTransform().rotate(180)));
                break;
            }
        } else if (roads.size() == 4) {
            p->drawImage(rect, block->variator > 1 ? cross.transformed(QTransform().rotate(90)) : cross);
        } else if (roads.size() == 2) {
            if (roads.contains(Qt::LeftEdge) && roads.contains(Qt::RightEdge)) {
                p->drawImage(rect, straight);
            } else if (roads.contains(Qt::TopEdge) && roads.contains(Qt::BottomEdge)) {
                p->drawImage(rect, straight.transformed(QTransform().rotate(90)));
            } else if (roads.contains(Qt::TopEdge) && roads.contains(Qt::LeftEdge)) {
                p->drawImage(rect, turn);
            } else if (roads.contains(Qt::TopEdge) && roads.contains(Qt::RightEdge)) {
                p->drawImage(rect, turn.transformed(QTransform().rotate(90)));
            } else if (roads.contains(Qt::RightEdge) && roads.contains(Qt::BottomEdge)) {
                p->drawImage(rect, turn.transformed(QTransform().rotate(180)));
            } else {
                p->drawImage(rect, turn.transformed(QTransform().rotate(-90)));
            }
        } else if (roads.size() == 3) {
            if (roads.contains(Qt::LeftEdge) && roads.contains(Qt::RightEdge)) {
                p->drawImage(rect, roads.contains(Qt::TopEdge) ? crossT : crossT.transformed(QTransform().rotate(180)));
            } else {
                p->drawImage(rect, roads.contains(Qt::LeftEdge) ? crossT.transformed(QTransform().rotate(-90)) : crossT.transformed(QTransform().rotate(90)));
            }
        }
    }
    // #ifdef QT_DEBUG
    //     p->setBrush(QBrush("transparent"));
    //     p->setPen(QPen(QBrush("black"), 1));
    //     p->drawRect(rect);
    // #endif
}

bool RoadsDrawer::_isVerticalEdge(Qt::Edge type)
{
    return type == Qt::TopEdge || Qt::BottomEdge;
}

QImage RoadsDrawer::_loadImageFromSvg(const QString& path, const QSize& scaleSize)
{
    QSvgRenderer renderer(path);

    QImage image(scaleSize, QImage::Format_ARGB32);
    image.fill("transparent");
    QPainter painter(&image);
    renderer.render(&painter);
    return image;
}

RoadsDrawer::RoadsDrawer(QQuickItem* parent)
    : QQuickPaintedItem { parent }
{
    reAskTimer.setInterval(1000);
    reAskTimer.setSingleShot(false);
    connect(&reAskTimer, &QTimer::timeout, this, [this]() {
        if (!m_generator) {
            progress(0);
            return;
        }
        progress(m_generator->getProgress());
        if (progress() - m_lastDrawnProgress > fieldWidth() * fieldHeight() / 10) {
            m_lastDrawnProgress = progress();
            update();
        }
    });
    connect(
        this, &RoadsDrawer::busyChanged, this, [this]() {
            if (busy()) {
                reAskTimer.start();
            } else {
                reAskTimer.stop();
                update();
            }
        },
        Qt::QueuedConnection);
    setAcceptedMouseButtons(Qt::LeftButton);
}

void RoadsDrawer::redraw()
{
    // seed(QRandomGenerator::system()->generate());
    _generate();
}

void RoadsDrawer::zoom(int newValue)
{
    int adjustedValue = qMax(1, qMin(512, newValue));
    if (m_zoom == adjustedValue) {
        return;
    }
    m_zoom = adjustedValue;
    emit zoomChanged();
    updateImplicits();
}

void RoadsDrawer::paint(QPainter* painter)
{
    if (!m_generator) {
        return;
    }
    m_generator->iterateBlocks([painter, this](int x, int y, const QSharedPointer<Block>& block) {
        QRectF paintRect = QRectF(x * zoom(), y * zoom(), zoom(), zoom()).translated(-m_topLeftPosition);
        // painter->setBrush(block.collapsed ? "green" : fillColor());
        // painter->setPen(block.collapsed ? "green" : fillColor());
        _drawBlock(block, paintRect, painter);
    },
        QRect(m_topLeftPosition.x() / zoom(), m_topLeftPosition.y() / zoom(), (width() / zoom()) + 2, (height() / zoom()) + 2));
}

void RoadsDrawer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_moving) {
        m_topLeftPosition += m_lastMovePosition - event->position();
        m_lastMovePosition = event->position();
        update();
    }
}

void RoadsDrawer::wheelEvent(QWheelEvent* event)
{
    auto oldZoom = zoom();
    zoom(event->angleDelta().y() > 0 ? qMax(qCeil(zoom() * 1.1), 1) : qMax(qFloor(zoom() * 0.9), 1));
    if (zoom() != oldZoom) {
        auto coeff = float(zoom()) / oldZoom;
        m_topLeftPosition = m_topLeftPosition * coeff - event->position() * (1 - coeff);
    }
    update();
}

void RoadsDrawer::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_moving = true;
        m_lastMovePosition = event->position();
    }
}

void RoadsDrawer::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_moving = false;
    }
}
