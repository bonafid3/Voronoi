#ifndef BOUNDS2D_H
#define BOUNDS2D_H

#include "utils.h"

#include <QVector2D>
#include <QtOpenGL>

class Bounds2D
{
public:
    Bounds2D();

    void add(QVector2D p);

    float minX;
    float maxX;
    float minY;
    float maxY;

    Bounds2D& operator|=(const Bounds2D& rhs);
    operator QString() const { return "Min/max X: " + toStr(minX) + " " + toStr(maxX) + ", Min/max Y: " + toStr(minY) + " " + toStr(maxY); }
    void init();
};

#endif // BOUNDS2D_H
