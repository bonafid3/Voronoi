#ifndef DXF_H
#define DXF_H

#include <QString>
#include <vector>
#include <QFile>
#include "polysegs.h"

enum class eSection {
    eUNKNOWN,
    eENTITIES
};

enum class eEntity {
    eUNKNOWN,
    ePOINT,
    eLINE,
    eLWPOLYLINE,
    eARC
};

class DXF
{
public:
    DXF();
    void processDXF(const QString& fname);
    bool hasNext();

    PolySegs& segments();
    std::vector<PolySegs>& polygons();

    QString next();
    void processArcEntity();
    void processLineEntity();
    void processPointEntity();
    void processLwPolyLineEntity();

    void sortSegments();
    void createPolygons();
private:
    QFile mF;
    eSection mCurrentSection;
    eEntity mCurrentEntity;

    std::vector<QString> mLines;

    PolySegs mSegments;
    std::vector<PolySegs> mPolygons;
};

#endif // DXF_H
