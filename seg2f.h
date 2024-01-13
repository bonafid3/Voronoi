#pragma once

#include <QVector2D>

class Seg2f {

public:

    Seg2f() {}
    Seg2f(const QVector2D &a, const QVector2D &b)
    {
        p[0] = a;
        p[1] = b;
        valid = true;
    }

    void set(const QVector2D &a,const QVector2D &b)
    {
        *this = Seg2f(a,b);
    }

    bool isValid()
    {
        return valid;
    }

    QVector2D& p0()
    {
        return p[0];
    }

    QVector2D& p1()
    {
        return p[1];
    }

    void swap()
    {
        auto temp = p[0];
        p[0] = p[1];
        p[1] = temp;
    }

    Seg2f swapped()
    {
        return Seg2f(p[1], p[0]);
    }

    QVector2D p[2];

private:
    bool valid = false;

};
