#include "terraingenerator.h"

TerrainGenerator::TerrainGenerator(int width, int depth, qint32 seed, QObject* parent)
    : QObject(parent)
    , m_width(width)
    , m_depth(depth)
    , rng(new QRandomGenerator(seed))
{
}

void TerrainGenerator::generate()
{
    initializeField();
    initializeBorders();
    collapseWaveFunction();
}

Block TerrainGenerator::getBlock(int x, int y) const
{
    return m_field[x][y];
}

void TerrainGenerator::iterateBlocks(std::function<void(int, int, const Block&)> func, const QRect& region)
{
    QMutexLocker locker(&m_mutex);
    const auto bounds = region.isNull() ? QRect(0, 0, m_width, m_depth) : region;
    for (int x = qMax(0, bounds.left()); x <= qMin(m_width - 1, bounds.right()); ++x) {
        for (int y = qMax(0, bounds.top()); y <= qMin(m_depth - 1, bounds.bottom()); ++y) {
            func(x, y, m_field[x][y]);
        }
    }
}

int TerrainGenerator::getProgress()
{
    int processed = 0;
    QMutexLocker locker(&m_mutex);
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_depth; ++y) {
            if (m_field[x][y].collapsed) {
                processed++;
            }
        }
    }
    return processed;
}

void TerrainGenerator::initializeField()
{
    QMutexLocker locker(&m_mutex);
    m_field.resize(m_depth);
    for (int i = 0; i < m_depth; ++i) {
        m_field[i].resize(m_width);
        for (int j = 0; j < m_width; ++j) {
            m_field[i][j] = Block();
        }
    }
}

void TerrainGenerator::initializeBorders()
{
    QMutexLocker locker(&m_mutex);
    // Fill borders with random valid states
    for (int i = 0; i < m_depth; ++i) {
        generateValidBlock(i, 0);
        generateValidBlock(i, m_width - 1);
    }
    for (int j = 1; j < m_width - 1; ++j) {
        generateValidBlock(0, j);
        generateValidBlock(m_depth - 1, j);
    }
}

void TerrainGenerator::generateValidBlock(int row, int col)
{
    Block& block = m_field[row][col];
    QHash<Qt::ArrowType, QVector<TerrainType>> possibleStates {
        { Qt::UpArrow, allTerrainTypes },
        { Qt::DownArrow, allTerrainTypes },
        { Qt::LeftArrow, allTerrainTypes },
        { Qt::RightArrow, allTerrainTypes }
    };
    int roads = 0;
    QVector<Qt::ArrowType> freeExits;
    for (auto direction : { Qt::UpArrow, Qt::DownArrow, Qt::LeftArrow, Qt::RightArrow }) {
        const auto comaptibleElements = getCompatibleElements(row, col, direction);
        possibleStates.insert(direction, comaptibleElements);
        if (comaptibleElements == QVector<TerrainType> { TerrainType::Road }) {
            roads++;
        } else if (comaptibleElements.size() > 1 && comaptibleElements.contains(TerrainType::Road)) {
            freeExits.append(direction);
        }
    }
    if (roads == 1 && freeExits.size() > 0 && rng->bounded(100) > 10) {
        possibleStates.insert(freeExits.value(rng->bounded(freeExits.size())), { TerrainType::Road });
        roads++;
    }
    for (auto iter = possibleStates.constBegin(); iter != possibleStates.constEnd(); ++iter) {
        block.setElement(iter.key(), iter.value().at(rng->bounded(iter.value().size())));
    }
    block.collapsed = true;
}

QVector<TerrainType> TerrainGenerator::getCompatibleElements(int row, int col, Qt::ArrowType direction)
{
    switch (direction) {
    case Qt::NoArrow:
        return allTerrainTypes;
    case Qt::UpArrow:
        if (col == 0 || !m_field[row][col - 1].collapsed) {
            return allTerrainTypes;
        } else {
            return { m_field[row][col - 1].getElement(Qt::DownArrow) };
        }
    case Qt::DownArrow:
        if (col == m_depth - 1 || !m_field[row][col + 1].collapsed) {
            return allTerrainTypes;
        } else {
            return { m_field[row][col + 1].getElement(Qt::UpArrow) };
        }
    case Qt::LeftArrow:
        if (row == 0 || !m_field[row - 1][col].collapsed) {
            return allTerrainTypes;
        } else {
            return { m_field[row - 1][col].getElement(Qt::RightArrow) };
        }
    case Qt::RightArrow:
        if (row == m_width - 1 || !m_field[row + 1][col].collapsed) {
            return allTerrainTypes;
        } else {
            return { m_field[row + 1][col].getElement(Qt::LeftArrow) };
        }
    }
}

void TerrainGenerator::collapseWaveFunction()
{
    QQueue<QPair<int, int>> queue;

    // Start with cells adjacent to borders
    {
        QMutexLocker locker(&m_mutex);
        for (int i = 1; i < m_depth - 1; ++i) {
            for (int j = 1; j < m_width - 1; ++j) {

                if (hasCollapsedNeighbor(i, j)) {
                    queue.enqueue({ i, j });
                }
            }
        }
    }
    while (!queue.isEmpty()) {
        QMutexLocker locker(&m_mutex);
        auto [row, col] = queue.dequeue();
        if (m_field[row][col].collapsed)
            continue;

        generateValidBlock(row, col);
        addUncollapedNeighborsToQueue(row, col, queue);
    }
}

bool TerrainGenerator::hasCollapsedNeighbor(int row, int col)
{
    return (row > 0 && m_field[row - 1][col].collapsed) || (row < m_depth - 1 && m_field[row + 1][col].collapsed) || (col > 0 && m_field[row][col - 1].collapsed) || (col < m_width - 1 && m_field[row][col + 1].collapsed);
}

void TerrainGenerator::addUncollapedNeighborsToQueue(int row, int col, QQueue<QPair<int, int>>& queue)
{
    if (row > 0 && !m_field[row - 1][col].collapsed)
        queue.enqueue({ row - 1, col });
    if (row < m_depth - 1 && !m_field[row + 1][col].collapsed)
        queue.enqueue({ row + 1, col });
    if (col > 0 && !m_field[row][col - 1].collapsed)
        queue.enqueue({ row, col - 1 });
    if (col < m_width - 1 && !m_field[row][col + 1].collapsed)
        queue.enqueue({ row, col + 1 });
}
