#include "roadsdrawer.h"

void RoadsDrawer::_generate()
{
    updateImplicits();
    m_lastDrawnProgress = 0;
    busy(true);
    m_generator.reset(new TerrainGenerator(fieldWidth(), fieldHeight(), seed()));
    QtConcurrent::run([this]() {
        m_generator->generate();
    }).then([this]() { busy(false); });
}

void RoadsDrawer::updateImplicits()
{
    setImplicitSize(fieldWidth() * zoom(), fieldHeight() * zoom());
}

void RoadsDrawer::_drawBlock(const Block& block, const QRectF& rect, QPainter* p)
{
    static QMap<TerrainType, QColor> colorMap {
        { TerrainType::Empty, "#90B77D" },
        { TerrainType::Road, "#9B9B9B" },
        { TerrainType::Building, "#D49B54" },
        { TerrainType::River, "#5B8FB9" },
        { TerrainType::Forest, "#2D5A27" }
    };

    if (!block.collapsed) {
        p->fillRect(rect, fillColor());
        return;
    }
    const auto center = rect.center();
    p->fillRect(rect, colorMap.value(TerrainType::Empty));
    QVector<Qt::ArrowType> roads;
    for (auto direction : { Qt::UpArrow, Qt::DownArrow, Qt::LeftArrow, Qt::RightArrow }) {
        const auto type = block.getElement(direction);
        if (type == TerrainType::Road) {
            roads.append(direction);
            continue;
        }
        QPainterPath path;
        path.moveTo(center);
        const QPointF p1(direction != Qt::RightArrow ? rect.left() : rect.right(), direction != Qt::DownArrow ? rect.top() : rect.bottom());
        const QPointF p2(_isVerticalEdge(direction) ? p1.x() : rect.right(), _isVerticalEdge(direction) ? rect.bottom() : p1.y());
        path.lineTo(p1);
        path.lineTo(p2);
        path.lineTo(center);
        p->fillPath(path, colorMap.value(type));
    }
    if (roads.size() > 0) {
        foreach (auto direction, roads) {
            const QPointF tl(direction != Qt::RightArrow ? rect.left() : rect.right() - rect.width() / 4, direction != Qt::DownArrow ? rect.top() : rect.bottom() - rect.height() / 4);
            const QPointF br(tl.x() + (_isVerticalEdge(direction) ? rect.width() / 4 : rect.width() / 2), tl.y() + (_isVerticalEdge(direction) ? rect.height() / 2 : rect.height() / 4));
            p->fillRect(QRectF(tl, br), colorMap.value(TerrainType::Road));
        }
        if (roads.size() > 1) {
            p->fillRect(QRectF(rect.left() + rect.width() / 4, rect.top() + rect.height() / 4, rect.width() / 2, rect.height() / 2), colorMap.value(TerrainType::Road));
        }
    }
}

bool RoadsDrawer::_isVerticalEdge(Qt::ArrowType type)
{
    return type == Qt::LeftArrow || Qt::RightArrow;
}

RoadsDrawer::RoadsDrawer(QQuickItem* parent)
    : QQuickPaintedItem { parent }
{
    reAskTimer.setInterval(1000);
    reAskTimer.setSingleShot(false);
    connect(&reAskTimer, &QTimer::timeout, this, [this]() {
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
}

void RoadsDrawer::redraw()
{
    seed(QRandomGenerator::system()->generate());
    _generate();
}

void RoadsDrawer::zoom(int newValue)
{
    int adjustedValue = qMax(1, qMin(32, newValue));
    if (m_zoom == adjustedValue) {
        return;
    }
    m_zoom = adjustedValue;
    emit zoomChanged();
    updateImplicits();
}

void RoadsDrawer::paint(QPainter* painter)
{
    m_generator->iterateBlocks([painter, this](int x, int y, const Block& block) {
        QRectF paintRect = QRectF(x * zoom(), y * zoom(), zoom(), zoom()).translated(-topLeft());
        // painter->setBrush(block.collapsed ? "green" : fillColor());
        // painter->setPen(block.collapsed ? "green" : fillColor());
        _drawBlock(block, paintRect, painter);
    },
        QRect(topLeft().x() / zoom(), topLeft().y() / zoom(), (width() / zoom()) + 1, (height() / zoom()) + 1));
}

void RoadsDrawer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_moving) {
        topLeft(topLeft() + m_lastMovingPosition - event->position());
        m_lastMovingPosition = event->position();
    }
}

void RoadsDrawer::wheelEvent(QWheelEvent* event)
{
}

void RoadsDrawer::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_moving = true;
        m_lastMovingPosition = event->position();
    }
}

void RoadsDrawer::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_moving = false;
    }
}
