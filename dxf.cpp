#include "dxf.h"
#include "utils.h"
#include <QByteArray>
#include <QString>

DXF::DXF() :
    mCurrentSection(eSection::eUNKNOWN),
    mCurrentEntity(eEntity::eUNKNOWN)
{
}

bool DXF::hasNext() {
    return !mF.atEnd();
}

QString DXF::next() {
    if(!hasNext())
        throw "End of file!!!";
    return QString::fromUtf8(mF.readLine()).trimmed();
}

PolySegs& DXF::segments()
{
    return mSegments;
}

void DXF::processDXF(const QString& fname)
{
    if(!QFile::exists(fname))
        return;

    mLines.clear();
    mSegments.clear();

    mF.setFileName(fname);
    if(mF.open(QFile::ReadOnly)) {

        try {
            while(hasNext()) {
                QString line = next();

                if(mCurrentSection != eSection::eENTITIES) {
                    if(line == "ENTITIES") {
                        mCurrentSection = eSection::eENTITIES;
                        continue;
                    }
                } else if(mCurrentSection == eSection::eENTITIES) {
                    if(line == "POINT") {
                        mCurrentEntity = eEntity::ePOINT;
                        processPointEntity();
                    } else if(line == "ARC") {
                        mCurrentEntity = eEntity::eARC;
                        processArcEntity(); // discovered the arc entity, now process it
                    } else if(line == "LINE") {
                        mCurrentEntity = eEntity::eLINE;
                        processLineEntity(); // discovered the line entity, now process it
                    } else if(line == "LWPOLYLINE") {
                        mCurrentEntity = eEntity::eLWPOLYLINE;
                        processLwPolyLineEntity(); // discovered the lw polyline entity, now process it
                    } else if(line == "ENDSEC") {
                        mCurrentSection = eSection::eUNKNOWN;
                    }
                }
            }
        } catch(const char* e) {
            qd << e;
        }

        mF.close();
    }
    sortSegments();
}

void DXF::processLwPolyLineEntity()
{
    int code = -1;
    float x1, y1;
    int numPoints = -1;
    std::vector<QVector2D> points;
    while(code != 0) {
        if(numPoints == 0) break;
        switch(code) {
        case 5: qd << next(); break;
            case 90: numPoints = next().toInt(); break;
            case 70: next(); break; // polyline flag, 0=default, 1=closed, 2=plinegen
            case 43: next(); break; // constant width
            case 10: x1 = next().toFloat(); break;
            case 20: y1 = next().toFloat(); points.push_back({x1, y1}); numPoints--; break;
            case 0: qd << "end of lwpolyline entity"; break;
            default: qd << "dxf:" << code << next(); break;
        }
    }

    for(int i=0; i<points.size()-1; i++) {
        mSegments.push_back({points[i],points[i+1]});
    }
}

void DXF::processPointEntity()
{
    int code = -1;
    float x, y, z;
    while(code != 0) {
        code = next().toInt();
        switch(code) {
            case 10: x = next().toFloat(); break;
            case 20: y = next().toFloat(); break;
            case 30: z = next().toFloat(); break;
            case  0: qd << "end of point entity"; break;
            default: qd << "dxf:" << code << next(); break;
        }
    }
}

void DXF::processArcEntity() {
    int code = -1;
    float x, y, z, radius, startAngle, endAngle;
    while(code != 0) {
        code = next().toInt();
        switch(code) {
            case 10: x = next().toFloat(); break;
            case 20: y = next().toFloat(); break;
            case 30: z = next().toFloat(); break;
            case 40: radius = next().toFloat(); break;
            case 50: startAngle = next().toFloat(); break;
            case 51: endAngle = next().toFloat(); break;
            case  0: qd << "end of arc entity"; break;
            default: qd << "dxf:" << code << next(); break;
        }
    }

    const float numSegments = 10;
    float angleIncrement = (endAngle - startAngle) / numSegments;

    // Check for clockwise or counterclockwise rotation
    if (angleIncrement < 0.0f) {
        // Adjust for CCW rotation
        angleIncrement = (endAngle + 360.0f - startAngle) / numSegments;
    }
    for (int i = 0; i < numSegments; ++i) {
        float angle1 = startAngle + i * angleIncrement;
        float angle2 = startAngle + (i + 1) * angleIncrement;

        QVector2D p1, p2;
        p1.setX(x + radius * cos(angle1 * M_PI / 180.0f));
        p1.setY(y + radius * sin(angle1 * M_PI / 180.0f));

        p2.setX(x + radius * cos(angle2 * M_PI / 180.0f));
        p2.setY(y + radius * sin(angle2 * M_PI / 180.0f));

        mSegments.push_back({ p1, p2 });
    }
}

void DXF::processLineEntity() {
    int code = -1;
    float x1, y1, z1, x2, y2, z2;
    while(code != 0) {
        code = next().toInt();
        switch(code) {
        case 10: x1 = next().toFloat(); break;
        case 11: x2 = next().toFloat(); break;
        case 20: y1 = next().toFloat(); break;
        case 21: y2 = next().toFloat(); break;
        case 30: z1 = next().toFloat(); break;
        case 31: z2 = next().toFloat(); break;
        case 0: qd << "end of line entity"; break;
        default: qd << "dxf:" << code << next(); break;
        }
    }

    mSegments.push_back({ {x1, y1}, {x2, y2} });
}

void DXF::sortSegments()
{
    if(mSegments.empty())
        return;

    PolySegs sorted;
    sorted.push_back(mSegments[0]);
    mSegments.erase(mSegments.begin());

    while(mSegments.size()) {

        QVector2D lastPoint = sorted.back().p1();
        float minDist = std::numeric_limits<float>::max();
        auto closestSegment = mSegments.begin();
        bool flipSegment = false;

        for(auto it = mSegments.begin(); it!= mSegments.end(); ++it) {
            float distToStart = (lastPoint - it->p[0]).length();
            float distToEnd = (lastPoint - it->p[1]).length();

            if (distToStart < minDist || distToEnd < minDist) {
                closestSegment = it;
                minDist = std::min(distToStart, distToEnd);
                flipSegment = distToEnd < distToStart; // Flip segment if the end is closer
            }
        }

        if(flipSegment) {
            closestSegment->swap();
        }
        sorted.push_back(*closestSegment);
        mSegments.erase(closestSegment);

    }

    mSegments = sorted;
}
