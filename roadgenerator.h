#ifndef ROADGENERATOR_H
#define ROADGENERATOR_H

#include <QPoint>
#include <QRandomGenerator>
#include <QRect>
#include <QSet>

class RoadGenerator {
    int m_width;
    int m_depth;
    QString m_lastError;
    QSet<QPoint> m_data;
    float _getRoadsAmount() const;

public:
    RoadGenerator(int width, int depth);
    bool generate(int maxBuildingSide, int maxRoadLanes, int roadsAmount, quint32 seed = 1);
    QString lastError() const;
    bool isRoadAtPoint(const QPoint& point);

private:
    void _generateNewBuilding(int maxBuildingSide, int maxRoadLanes, const QRect& bounds, QRandomGenerator* rg);
};

#endif // ROADGENERATOR_H
