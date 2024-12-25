#include "roadgenerator.h"

QString RoadGenerator::lastError() const
{
    return m_lastError;
}

bool RoadGenerator::isRoadAtPoint(const QPoint& point)
{
    return m_data.contains(point);
}

void RoadGenerator::_generateNewBuilding(int maxBuildingSide, int maxRoadLanes, const QRect& bounds, QRandomGenerator* rg)
{
    int topLanes = rg->bounded(1, maxRoadLanes + 1);
    int bottomLanes = rg->bounded(1, maxRoadLanes + 1);
    int leftLanes = rg->bounded(1, maxRoadLanes + 1);
    int rightLanes = rg->bounded(1, maxRoadLanes + 1);
    int width = rg->bounded(1, maxBuildingSide + 1);
    int height = rg->bounded(1, maxBuildingSide + 1);
    int x = rg->bounded(-width, bounds.width());
    int y = rg->bounded(-height, bounds.height());
    const QRect innerRect(QPoint(x, y), QSize(width, height));
    const QRect outerRect(QPoint(x - leftLanes, y - topLanes), QSize(width + leftLanes + rightLanes, height + topLanes + bottomLanes));
    for (int dx = outerRect.x(); dx < outerRect.x() + outerRect.width(); ++dx) {
        for (int dy = outerRect.y(); dy < outerRect.y() + outerRect.height(); ++dy) {
            const QPoint p(dx, dy);
            if (!innerRect.contains(p) && bounds.contains(p)) {
                m_data.insert(p);
            } else {
                m_data.remove(p);
            }
        }
    }
}

float RoadGenerator::_getRoadsAmount() const
{
    return float(m_data.size()) / (m_width * m_depth);
}

RoadGenerator::RoadGenerator(int width, int depth)
    : m_width(width)
    , m_depth(depth)
{
}

bool RoadGenerator::generate(int maxBuildingSide, int maxRoadLanes, int roadsAmount, quint32 seed)
{
    if (maxBuildingSide < 1) {
        m_lastError = QStringLiteral("Building side cannot be less than 1");
        return false;
    }
    if (maxRoadLanes < 1) {
        m_lastError = QStringLiteral("Road lanes cannot be less than 1");
        return false;
    }
    m_data.clear();
    const QRect bounds(QPoint { 0, 0 }, QSize(m_width, m_depth));
    QRandomGenerator rg(seed);
    for (int i = 0; i < roadsAmount; ++i) {
        _generateNewBuilding(maxBuildingSide, maxRoadLanes, bounds, &rg);
    }
    return true;
}
