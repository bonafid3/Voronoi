#pragma once

#include <QLine>
#include "point.h"

#include <qmath.h>

class Spline
{
public:

    Spline() = default;

    float getT(float distance, float alpha, float tension) {
        for(size_t i=0;i<mPoints.size(); ++i) {
            if(distance - mPoints[i].segLen <= 0) {
                float t = i;
                QVector2D prev;
                for(; t<mPoints.size(); t+=0.0005f) {
                    QVector2D p = getSplinePoint(t, alpha, tension);
                    if(t>i) {
                        QLineF line(prev.x(), prev.y(), p.x(), p.y());
                        distance -= line.length();
                        if(distance <= 0.f) return t;
                    }
                    prev = p;
                }
            } else {
                distance -= mPoints[i].segLen;
            }
        }
        return -1.0;
    }

    float distance(QVector2D p0, QVector2D p1) {
        return (p0 - p1).length();
    }

    float calcTotalLen(float alpha, float tension) {
        for(size_t i=0; i<mPoints.size(); i++) mPoints[i].segLen = 0.f;

        float total = 0.f;
        QVector2D prev = getSplinePoint(0.f, alpha, tension);
        for(float t=0.0005f; t<mPoints.size(); t+=0.0005f) {
            QVector2D p = getSplinePoint(t, alpha, tension);
            QLineF line(prev.x(), prev.y(), p.x(), p.y());
            mPoints[static_cast<size_t>(t)].segLen += line.length();
            total += line.length();
            prev = p;
        }
        return total;
    }

    QVector2D doTheMath(float t, double alpha, double tension, bool gradient)
    {
        int p0i, p1i, p2i, p3i;

        p1i = (int)t;
        p2i = (p1i + 1) % mPoints.size();
        p3i = (p2i + 1) % mPoints.size();
        p0i = p1i >= 1 ? p1i - 1 : mPoints.size() - 1;

        QVector2D p0 = mPoints[p0i];
        QVector2D p1 = mPoints[p1i];
        QVector2D p2 = mPoints[p2i];
        QVector2D p3 = mPoints[p3i];

        t = t - (int)t;

        float t0 = 0.f;
        float t1 = t0 + qPow(distance(p0, p1), alpha);
        float t2 = t1 + qPow(distance(p1, p2), alpha);
        float t3 = t2 + qPow(distance(p2, p3), alpha);

        QVector2D m1 = (1.0f - tension) * (t2 - t1) *
            ((p1 - p0) / (t1 - t0) - (p2 - p0) / (t2 - t0) + (p2 - p1) / (t2 - t1));
        QVector2D m2 = (1.0f - tension) * (t2 - t1) *
            ((p2 - p1) / (t2 - t1) - (p3 - p1) / (t3 - t1) + (p3 - p2) / (t3 - t2));

        QVector2D a = 2.0f * (p1 - p2) + m1 + m2;
        QVector2D b = -3.0f * (p1 - p2) - m1 - m1 - m2;
        QVector2D c = m1;
        QVector2D d = p1;

        return  gradient ?
                    a * 3 * t * t +
                    b * 2 * t +
                    c
                :
                    a * t * t * t +
                    b * t * t +
                    c * t +
                    d;
    }

    QVector2D getSplinePoint(float t, float alpha, float tension)
    {
        return doTheMath(t, alpha, tension, false);
    }

    QVector2D getSplineGradient(float t, float alpha, float tension)
    {
        return doTheMath(t, alpha, tension, true);
    }

    std::vector<Point> mPoints;

};
