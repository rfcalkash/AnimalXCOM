#ifndef TERRAINGENERATOR_H
#define TERRAINGENERATOR_H

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QQueue>
#include <QRandomGenerator>
#include <QRect>
#include <QVector>

enum class TerrainType {
    Empty,
    Road,
    Building,
    River,
    Forest
};

static QVector<TerrainType> allTerrainTypes { TerrainType::Empty, TerrainType::Road, TerrainType::Building, TerrainType::River, TerrainType::Forest };
struct Block {
private:
    TerrainType elements[4] = { TerrainType::Empty, TerrainType::Empty, TerrainType::Empty, TerrainType::Empty };

public:
    TerrainType getElement(Qt::ArrowType direction) const
    {
        if (direction == Qt::NoArrow) {
            return TerrainType::Empty;
        }
        return elements[direction - 1];
    }
    void setElement(Qt::ArrowType direction, TerrainType type)
    {
        if (direction == Qt::NoArrow) {
            return;
        }
        elements[direction - 1] = type;
    }
    bool collapsed = false;
    // QVector<TerrainType> possibleStates[4] = { allTerrainTypes, allTerrainTypes, allTerrainTypes, allTerrainTypes };
    // QHash<Qt::ArrowType, TerrainType> states;
};

class TerrainGenerator : public QObject {
    Q_OBJECT

    QMutex m_mutex;

public:
    TerrainGenerator(int width, int depth, qint32 seed = QRandomGenerator::global()->generate(), QObject* parent = nullptr);

    void generate();
    Block getBlock(int x, int y) const;
    void iterateBlocks(std::function<void(int x, int y, const Block& block)> func, const QRect& region = QRect());
    int getProgress();

private:
    int m_width;
    int m_depth;
    QRandomGenerator* rng;
    QVector<QVector<Block>> m_field;

    void initializeField();

    void initializeBorders();

    void generateValidBlock(int row, int col);

    QVector<TerrainType> getCompatibleElements(int row, int col, Qt::ArrowType direction);

    void collapseWaveFunction();

    bool hasCollapsedNeighbor(int row, int col);

    void addUncollapedNeighborsToQueue(int row, int col, QQueue<QPair<int, int>>& queue);
};

#endif // TERRAINGENERATOR_H
