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

    QString next();
    void processArcEntity();
    void processLineEntity();
    void processPointEntity();
    void processLwPolyLineEntity();

    void sortSegments();
private:
    QFile mF;
    eSection mCurrentSection;
    eEntity mCurrentEntity;

    std::vector<QString> mLines;

    PolySegs mSegments;
};

#endif // DXF_H
