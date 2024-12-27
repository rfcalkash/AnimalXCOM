#include "terraingenerator.h"

TerrainType TerrainGenerator::_getRandomType(const QVector<TerrainType>& types)
{
    if (types.size() == 0) {
        return TerrainType::Empty;
    }
    int weightsSum = 0;
    foreach (auto type, types) {
        weightsSum += m_typeWeights.value(type);
    }
    const auto winner = rng->bounded(weightsSum);
    int currentWeight = 0;
    foreach (auto type, types) {
        currentWeight += m_typeWeights.value(type);
        if (currentWeight > winner) {
            return type;
        }
    }
    return types.last();
}

float TerrainGenerator::_getTypeWeight(TerrainType type, const QVector<TerrainType>& types)
{
    if (types.size() == 0) {
        return 0;
    }
    int weightsSum = 0;
    foreach (auto type, types) {
        weightsSum += m_typeWeights.value(type);
    }
    return float(m_typeWeights.value(type)) / weightsSum;
}

Qt::Edge TerrainGenerator::_nextEdge(Qt::Edge edge)
{
    if (edge == Qt::BottomEdge) {
        return Qt::TopEdge;
    }
    return Qt::Edge(edge * 2);
}

Qt::Edge TerrainGenerator::_oppositeEdge(Qt::Edge edge)
{
    switch (edge) {
    case Qt::TopEdge:
        return Qt::BottomEdge;
    case Qt::LeftEdge:
        return Qt::RightEdge;
    case Qt::RightEdge:
        return Qt::LeftEdge;
    case Qt::BottomEdge:
        return Qt::TopEdge;
    }
}

QPair<Qt::Edge, Qt::Edge> TerrainGenerator::_cornerEdges(Qt::Corner corner)
{
    switch (corner) {
    case Qt::TopLeftCorner:
        return { Qt::LeftEdge, Qt::TopEdge };
    case Qt::TopRightCorner:
        return { Qt::RightEdge, Qt::TopEdge };
    case Qt::BottomLeftCorner:
        return { Qt::LeftEdge, Qt::BottomEdge };
    case Qt::BottomRightCorner:
        return { Qt::RightEdge, Qt::BottomEdge };
    }
}

Qt::Edge TerrainGenerator::_turnEdge(Qt::Edge edge, bool toRight)
{
    Qt::Edge ret;
    switch (edge) {
    case Qt::TopEdge:
        ret = Qt::RightEdge;
        break;
    case Qt::LeftEdge:
        ret = Qt::TopEdge;
        break;
    case Qt::RightEdge:
        ret = Qt::BottomEdge;
        break;
    case Qt::BottomEdge:
        ret = Qt::LeftEdge;
        break;
    }
    return toRight ? _oppositeEdge(ret) : ret;
}

Qt::Edge TerrainGenerator::_fillRoad(const QSharedPointer<Block>& block, Qt::Edge currentDirection, int& turnPosiibility, Qt::LayoutDirection& turnsData)
{
    auto ret = currentDirection;
    block->setCenterElement(TerrainType::Road);
    if (rng->bounded(20) < turnPosiibility) {
        turnPosiibility = 1;
        switch (turnsData) {
        case Qt::LeftToRight:
        case Qt::RightToLeft:
            ret = _turnEdge(currentDirection, turnsData == Qt::LeftToRight);
            turnsData = Qt::LayoutDirectionAuto;
            break;
        case Qt::LayoutDirectionAuto:
            turnsData = Qt::LayoutDirection(rng->bounded(2));
            ret = _turnEdge(currentDirection, turnsData != Qt::LeftToRight);
            break;
        }
    } else {
        turnPosiibility++;
    }
    for (auto side : allEdges) {
        block->setSideElement(side, side == ret || side == _oppositeEdge(currentDirection) ? TerrainType::Road : block->collapsed ? block->getSideElement(side)
                                                                                                                                  : TerrainType::Empty);
    }
    block->collapse(rng);
    return ret;
}

int TerrainGenerator::_buildRoad(const QPoint& startPoint, Qt::Edge direction)
{
    auto currentCell = startPoint;
    int turnPossibility = 1;
    auto turnData = Qt::LayoutDirectionAuto;
    auto block = m_field(currentCell);
    auto currentDirection = direction;
    int ret = 0;
    while (block->isValid()) {
        if (!(block->collapsed && block->getCenterElement() == TerrainType::Road)) {
            ret++;
        }
        currentDirection = _fillRoad(block, currentDirection, turnPossibility, turnData);
        currentCell = m_field.nextCoords(currentCell, currentDirection);
        block = m_field(currentCell);
    }
    return ret;
}

QPoint TerrainGenerator::cornerShift(Qt::Corner corner)
{
    switch (corner) {
    case Qt::TopLeftCorner:
        return QPoint(-1, -1);
    case Qt::TopRightCorner:
        return QPoint(1, -1);
    case Qt::BottomLeftCorner:
        return QPoint(-1, 1);
    case Qt::BottomRightCorner:
        return QPoint(1, 1);
    }
}

TerrainGenerator::TerrainGenerator(int width, int depth, qint32 seed, const QHash<TerrainType, int>& typeWeights, QObject* parent)
    : QObject(parent)
    , m_width(width)
    , m_depth(depth)
    , m_typeWeights(typeWeights)
    , rng(new QRandomGenerator(seed))
    , m_field(width, depth)
{
}

void TerrainGenerator::generate()
{
    // initializeField();
    initializeRoads();
    // generateValidBlock(0, 0);
    collapseWaveFunction();
    polishData();
}

QSharedPointer<Block> TerrainGenerator::getBlock(int x, int y) const
{
    return m_field(x, y);
}

void TerrainGenerator::iterateBlocks(std::function<void(int, int, const QSharedPointer<Block>&)> func, const QRect& region)
{
    QMutexLocker locker(&m_mutex);
    const auto bounds = region.isNull() ? QRect(0, 0, m_width, m_depth) : region;
    for (int x = qMax(0, bounds.left()); x <= qMin(m_width - 1, bounds.right()); ++x) {
        for (int y = qMax(0, bounds.top()); y <= qMin(m_depth - 1, bounds.bottom()); ++y) {
            func(x, y, m_field(x, y));
        }
    }
}

int TerrainGenerator::getProgress()
{
    int processed = 0;
    QMutexLocker locker(&m_mutex);
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_depth; ++y) {
            if (m_field(x, y)->collapsed) {
                processed++;
            }
        }
    }
    return processed;
}

void TerrainGenerator::polishData()
{
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_depth; ++y) {
            const auto B = m_field(x, y);
            if (!B->isValid() || B->getCenterElement() == TerrainType::Road) {
                continue;
            }
            for (auto corner : allCorners) {
                const auto edges = _cornerEdges(corner);
                bool allMatch = true;
                for (auto edge : { edges.first, edges.second }) {
                    const auto b = m_field.nextBlock(x, y, edge);
                    if (!b->isValid() || b->getCenterElement() == B->getCenterElement()) {
                        B->setSideElement(edge, B->getCenterElement());
                    } else {
                        allMatch = false;
                        B->setSideElement(edge, TerrainType::Empty);
                    }
                }
                if (allMatch) {
                    const auto b = m_field(QPoint(x, y) + cornerShift(corner));
                    if (!b->isValid() || b->getCenterElement() == B->getCenterElement()) {
                        B->setFilledCorner(corner);
                    }
                }
            }
        }
    }
}

void TerrainGenerator::initializeField()
{
    QMutexLocker locker(&m_mutex);
    m_field = Field(m_width, m_depth);
}

void TerrainGenerator::initializeRoads()
{
    int roadsCount = 0;
    int enoughtCount = m_field.size() * _getTypeWeight(TerrainType::Road);
    auto startSide = Qt::TopEdge;
    while (roadsCount < enoughtCount) {
        QMutexLocker locker(&m_mutex);
        QPoint currentCell;
        switch (startSide) {
        case Qt::TopEdge:
            currentCell = QPoint(rng->bounded(m_width), 0);
            break;
        case Qt::BottomEdge:
            currentCell = QPoint(rng->bounded(m_width), m_depth - 1);
            break;
        case Qt::LeftEdge:
            currentCell = QPoint(0, rng->bounded(m_depth));
            break;
        case Qt::RightEdge:
            currentCell = QPoint(m_width - 1, rng->bounded(m_depth));
            break;
        }
        roadsCount += _buildRoad(currentCell, _oppositeEdge(startSide));
        startSide = _nextEdge(startSide);
    }
}

void TerrainGenerator::generateValidBlock(int x, int y)
{
    const auto block = m_field(x, y);
    if (!block->isValid()) {
        return;
    }
    QVector<TerrainType> allSidesStates;
    for (auto direction : allEdges) {
        const auto neighbor = m_field.nextBlock(x, y, direction);
        allSidesStates.append(neighbor->isValid() && neighbor->collapsed ? QVector<TerrainType> { neighbor->getCenterElement() } : allTerrainTypes);
    }
    if (!allSidesStates.contains(TerrainType::Empty)) {
        allSidesStates.append(TerrainType::Empty);
    }
    block->setCenterElement(_getRandomType(allSidesStates));
    block->collapse(rng);
}

void TerrainGenerator::collapseWaveFunction()
{
    QQueue<QPoint> queue;

    {
        QMutexLocker locker(&m_mutex);
        for (int x = 0; x < m_width; ++x) {
            for (int y = 0; y < m_depth; ++y) {

                if (hasCollapsedNeighbor(x, y)) {
                    queue.enqueue({ x, y });
                }
            }
        }
        if (queue.isEmpty()) {
            generateValidBlock(0, 0);
            queue.enqueue({ 0, 0 });
        }
    }
    while (!queue.isEmpty()) {
        QMutexLocker locker(&m_mutex);
        auto p = queue.dequeue();
        if (m_field(p)->collapsed)
            continue;

        generateValidBlock(p.x(), p.y());
        addUncollapsedNeighborsToQueue(p, queue);
    }
}

bool TerrainGenerator::hasCollapsedNeighbor(int x, int y)
{
    bool ret = false;
    for (auto direction : allEdges) {
        const auto block = m_field.nextBlock(x, y, direction);
        ret |= block->isValid() && block->collapsed;
    }
    return ret;
}

void TerrainGenerator::addUncollapsedNeighborsToQueue(const QPoint& p, QQueue<QPoint>& queue)
{
    for (auto direction : allEdges) {
        const auto newCoords = Field::nextCoords(p, direction);
        const auto block = m_field(newCoords);
        if (block->isValid() && !block->collapsed) {
            queue.enqueue(newCoords);
        }
    }
}
