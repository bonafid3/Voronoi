#pragma once

#include <vector>
#include "seg2f.h"
#include "bounds2d.h"

struct PolySegs : public std::vector<Seg2f>
{
    void calcBounds() {
        mBounds.init();
        for(auto& p : *this) {
            mBounds.add(p.p[0]);
            mBounds.add(p.p[1]);
        }
    }

    std::vector<GLfloat> glFloatArray(float z = 0.0f)
    {
        std::vector<GLfloat> res;
        for(size_t i=0; i<size(); i++) {
            res.push_back(at(i).p[0].x());
            res.push_back(at(i).p[0].y());
            res.push_back(z);
            res.push_back(at(i).p[1].x());
            res.push_back(at(i).p[1].y());
            res.push_back(z);
        }
        return res;
    }

    void translate(QVector2D tv)
    {
        for(auto& v : *this) {
            v.p[0] += tv;
            v.p[1] += tv;
        }
    }

    PolySegs translated(QVector2D tv)
    {
        PolySegs res(*this);
        res.translate(tv);
        return res;
    }

    void rotate(float rad)
    {
        for(auto& v : *this) {
            float si = sin(rad);
            float co = cos(rad);
            float tx = v.p[0].x();
            float ty = v.p[0].y();
            v.p[0].setX((co * tx) - (si * ty));
            v.p[0].setY((si * tx) + (co * ty));
            tx = v.p[1].x();
            ty = v.p[1].y();
            v.p[1].setX((co * tx) - (si * ty));
            v.p[1].setY((si * tx) + (co * ty));
        }
    }

    PolySegs rotated(float rad)
    {
        PolySegs res(*this);
        res.rotate(rad);
        return res;
    }

    Bounds2D mBounds;
};
