#ifndef TERRAINGENERATOR_H
#define TERRAINGENERATOR_H

#include <QDebug>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QQueue>
#include <QRandomGenerator>
#include <QRect>
#include <QSharedPointer>
#include <QVector>

enum class TerrainType {
    Empty,
    Road,
    Building,
    River,
    Forest
};

struct Block {
private:
    TerrainType centerElement = TerrainType::Empty;
    QHash<Qt::Edge, TerrainType> sideElements { { Qt::TopEdge, TerrainType::Empty }, { Qt::TopEdge, TerrainType::Empty }, { Qt::TopEdge, TerrainType::Empty }, { Qt::TopEdge, TerrainType::Empty } };
    QHash<Qt::Corner, bool> filledCorners { { Qt::TopLeftCorner, false }, { Qt::TopRightCorner, false }, { Qt::BottomLeftCorner, false }, { Qt::BottomRightCorner, false } };
    bool _isValid = true;

public:
    void setFilledCorner(Qt::Corner corner, bool filled = true)
    {
        filledCorners[corner] = filled;
    }

    bool getFilledCorner(Qt::Corner corner) const
    {
        return filledCorners.value(corner);
    }

    TerrainType getSideElement(Qt::Edge direction) const
    {
        return sideElements.value(direction);
    }
    void setSideElement(Qt::Edge direction, TerrainType type)
    {
        sideElements[direction] = type;
    }
    bool collapsed = false;
    int variator;
    void collapse(QRandomGenerator* r)
    {
        collapsed = true;
        variator = r->bounded(4);
    }
    static Block* invalidBlock()
    {
        const auto ret = new Block();
        ret->_isValid = false;
        return ret;
    }
    // QVector<TerrainType> possibleStates[4] = { allTerrainTypes, allTerrainTypes, allTerrainTypes, allTerrainTypes };
    // QHash<Qt::ArrowType, TerrainType> states;
    TerrainType getCenterElement() const
    {
        return centerElement;
    }
    void setCenterElement(TerrainType newCenterElement)
    {
        centerElement = newCenterElement;
    }
    bool isValid() const
    {
        return _isValid;
    }
};

class Field {
    QHash<int, QSharedPointer<Block>> m_data;
    int m_width;
    int m_depth;
    int m_size;

public:
    Field(int width, int depth)
        : m_width(width)
        , m_depth(depth)
        , m_size(width * depth)
    {
        m_data.reserve(m_size);
        for (int i = 0; i < m_size; ++i) {
            m_data[i] = QSharedPointer<Block>(new Block());
        }
    }
    QSharedPointer<Block> operator()(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= m_width || y >= m_depth) {
            return QSharedPointer<Block>(Block::invalidBlock());
        }
        const auto index = y * m_width + x % m_width;
        if (index >= m_size) {
            return QSharedPointer<Block>(Block::invalidBlock());
        }
        return m_data.value(index);
    }

    QSharedPointer<Block> operator()(const QPoint& p) const
    {
        return (*this)(p.x(), p.y());
    }

    QSharedPointer<Block> nextBlock(int x, int y, Qt::Edge direction) const
    {
        return (*this)(nextCoords(x, y, direction));
    }

    static QPoint nextCoords(int x, int y, Qt::Edge direction)
    {
        auto newX = x;
        auto newY = y;
        switch (direction) {
        case Qt::TopEdge:
            newY--;
            break;
        case Qt::LeftEdge:
            newX--;
            break;
        case Qt::RightEdge:
            newX++;
            break;
        case Qt::BottomEdge:
            newY++;
            break;
        }
        return { newX, newY };
    }

    static QPoint nextCoords(const QPoint& p, Qt::Edge direction)
    {
        return nextCoords(p.x(), p.y(), direction);
    }

    QSharedPointer<Block> nextBlock(const QPoint& p, Qt::Edge direction) const
    {
        return nextBlock(p.x(), p.y(), direction);
    }

    int width() const
    {
        return m_width;
    }
    int depth() const
    {
        return m_depth;
    }
    int size() const
    {
        return m_size;
    }
};

class TerrainGenerator : public QObject {
    Q_OBJECT
public:
    inline static QVector<TerrainType> allTerrainTypes { TerrainType::Empty, TerrainType::Building, TerrainType::River, TerrainType::Forest };
    inline static QVector<Qt::Edge> allEdges { Qt::TopEdge, Qt::LeftEdge, Qt::RightEdge, Qt::BottomEdge };
    inline static QVector<Qt::Corner> allCorners { Qt::TopLeftCorner, Qt::TopRightCorner, Qt::BottomLeftCorner, Qt::BottomRightCorner };

private:
    QMutex m_mutex;

    QHash<TerrainType, int> m_typeWeights;

    TerrainType _getRandomType(const QVector<TerrainType>& types);
    float _getTypeWeight(TerrainType type, const QVector<TerrainType>& types = allTerrainTypes + QVector<TerrainType> { TerrainType::Road });
    static Qt::Edge _nextEdge(Qt::Edge edge);
    static Qt::Edge _oppositeEdge(Qt::Edge edge);
    static QPair<Qt::Edge, Qt::Edge> _cornerEdges(Qt::Corner corner);
    static Qt::Edge _turnEdge(Qt::Edge edge, bool toRight);
    Qt::Edge _fillRoad(const QSharedPointer<Block>& block, Qt::Edge currentDirection, int& turnPosiibility, Qt::LayoutDirection& turnsData);
    int _buildRoad(const QPoint& startPoint, Qt::Edge direction);

public:
    static QPoint cornerShift(Qt::Corner corner);

    TerrainGenerator(int width, int depth, qint32 seed = QRandomGenerator::global()->generate(),
        const QHash<TerrainType, int>& typeWeights = {
            { TerrainType::Empty, 100 },
            { TerrainType::Road, 50 },
            { TerrainType::Building, 20 },
            { TerrainType::River, 50 },
            { TerrainType::Forest, 100 } },
        QObject* parent = nullptr);

    void generate();
    QSharedPointer<Block> getBlock(int x, int y) const;
    void iterateBlocks(std::function<void(int, int, const QSharedPointer<Block>&)> func, const QRect& region = QRect());
    int getProgress();

private:
    int m_width;
    int m_depth;
    QRandomGenerator* rng;
    Field m_field;

    void polishData();

    void initializeField();

    void initializeRoads();

    void generateValidBlock(int x, int y);

    void collapseWaveFunction();

    bool hasCollapsedNeighbor(int x, int y);

    void addUncollapsedNeighborsToQueue(const QPoint& p, QQueue<QPoint>& queue);
};

#endif // TERRAINGENERATOR_H
