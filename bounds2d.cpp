#include "bounds2d.h"

Bounds2D::Bounds2D()
{
    init();
}

void Bounds2D::init()
{
    minX = minY = std::numeric_limits<float>::max();
    maxX = maxY = std::numeric_limits<float>::lowest();
}

void Bounds2D::add(QVector2D p)
{
    if(p.x() < minX) minX = p.x();
    else if(p.x() > maxX) maxX = p.x();

    if(p.y() < minY) minY = p.y();
    else if(p.y() > maxY) maxY = p.y();
}

Bounds2D& Bounds2D::operator|=(const Bounds2D& rhs)
{
    minX = qMin(minX, rhs.minX);
    maxX = qMax(maxX, rhs.maxX);

    minY = qMin(minY, rhs.minY);
    maxY = qMax(maxY, rhs.maxY);

    return *this;
}
