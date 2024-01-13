#pragma once

#include <QVector2D>

class Point : public QVector2D
{
public:
    Point(QVector2D p) : QVector2D{ p }
    {}

    float segLen = 0.f;
};
