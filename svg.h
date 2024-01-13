#ifndef SVG_H
#define SVG_H

#include <QChar>
#include <QVector>
#include <QVector2D>

#include <vector>

#include "polysegs.h"

class cSVGCmd
{
public:
    cSVGCmd(){}
    void set(QChar cmd) { this->cmd = cmd; }
    void set(QChar cmd, float op0) { set(cmd); ops.push_back(op0); }
    void set(QChar cmd, float op0, float op1) { set(cmd, op0); ops.push_back(op1); }
    void add(QString val) { ops.push_back(val.toFloat()); }

    QVector2D absStart;

    QChar cmd;
    QVector<float> ops;
};

class SVG
{
public:
    SVG();

    void processSVG(const QString& fname);

    std::vector<QVector2D>& points();
    PolySegs& segments();

    float mX=0, mY=0;
    QVector<cSVGCmd> mCommands;

    std::vector<QVector2D> mPoints;
    PolySegs mSegments;

    int processCMD(cSVGCmd cmd);
    void parseCommands();
    bool isNum(QChar c);
    QVector2D bezier(QVector2D s, QVector2D e, QVector2D c0, QVector2D c1, const float f);
    void appendPoint(float x, float y);
    void appendPoint(QVector2D p);
    void appendPoints(std::vector<QVector2D> newPoints);

    void appendSegment(Seg2f seg);
    void sortSegments();
private:
    void translate(QVector2D v);
    void flipY();
};

#endif // SVG_H
