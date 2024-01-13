#include "svg.h"
#include "utils.h"
#include <QByteArray>

SVG::SVG()
{
}

void SVG::processSVG(const QString& fname)
{
    if(!QFile::exists(fname)) return;

    mX = 0;
    mY = 0;
    mPoints.clear();
    mSegments.clear();

    QByteArray data = readFile(fname);

    data.replace("\r","");
    data.replace("\n","");
    data.replace("\t","");

    QRegExp rx("<path.* d=\"([^\">]+)\"");
    rx.setMinimal(true); // important, make regexp lazy instead of greedy!
    int offset=0;
    while((offset = rx.indexIn(data,offset)) != -1) {
        QString path = rx.cap(1);

        mCommands.clear();

        int i=0;

        while(i < path.size()) {
            cSVGCmd cmd;

            // skip space if any
            if(path[i] == ' ') {
                i++;
                continue;
            }

            cmd.set(path[i]);
            int operands = processCMD(cmd);

            i++;

            if(operands == 0) {
                qd << "z command?";
            }

            if(operands == -1) {
                qd << "unknown" << cmd.cmd;
                break;
            }

            for(int j=0; j<operands; j++) {
                QString val;

                if(path[i] == '-') {
                    val += "-";
                    i++;
                }

                if(path[i] == ' ' || path[i] == ',') i++;

                while(isNum(path[i]))
                    val += path[i++];

                if(val.size())
                    cmd.add(val);
            }

            mCommands.push_back(cmd);
        }

        // path parsed here
        parseCommands();

        offset += rx.matchedLength();
    }

    sortSegments();
}

void SVG::sortSegments()
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

std::vector<QVector2D> &SVG::points()
{
    return mPoints;
}

PolySegs &SVG::segments()
{
    return mSegments;
}

// get the number of operands for a command
int SVG::processCMD(cSVGCmd cmd)
{
    qd << "check cmd" << cmd.cmd;
    switch(cmd.cmd.unicode())
    {
    case 'm':
    case 'M': return 2;

    case 'L':
    case 'l': return 2;

    case 'c':
    case 'C': return 6;

    case 'H':
    case 'h':
    case 'V':
    case 'v': return 1;

    case 's':
    case 'S': return 4;

    case 'z': return 0;
    case ' ': return 0;

    case ',': return 0;

    default:
        return -1;
    }
}

void SVG::appendPoint(float x, float y)
{
    appendPoint(QVector2D(x,y));
}

void SVG::appendPoint(QVector2D p)
{
    mPoints.push_back(p);
}

void SVG::appendPoints(std::vector<QVector2D> newPoints)
{
    mPoints.insert(mPoints.end(), newPoints.begin(), newPoints.end());
}

void SVG::appendSegment(Seg2f seg)
{
    mSegments.push_back(seg);
}

void SVG::parseCommands()
{
    for(int i=0; i<mCommands.size(); ++i) {
        cSVGCmd& cmd = mCommands[i];

        if(cmd.cmd == "c") {
            QVector2D s(mX, mY);

            cmd.absStart = s;

            QVector2D c0(cmd.ops[0], cmd.ops[1]); c0 += s;
            QVector2D c1(cmd.ops[2], cmd.ops[3]); c1 += s;
            QVector2D e(cmd.ops[4], cmd.ops[5]); e += s;

            std::vector<QVector2D> res;
            for(float t=0.1; t<=0.9; t+=0.1)
                res.push_back( bezier(s, e, c0, c1, t) );

            appendPoint(s);
            appendPoints(res);
            appendPoint(e);

            mX = e.x();
            mY = e.y();
        } else if(cmd.cmd == "C") {
            QVector2D s(mX, mY);

            cmd.absStart = s;

            QVector2D c0(cmd.ops[0], cmd.ops[1]);
            QVector2D c1(cmd.ops[2], cmd.ops[3]);
            QVector2D e(cmd.ops[4], cmd.ops[5]);

            std::vector<QVector2D> res;
            for(float t=0.1; t<=0.9; t+=0.1)
                res.push_back( bezier(s, e, c0, c1, t) );

            appendPoint(s);
            appendPoints(res);
            appendPoint(e);

            mX = e.x();
            mY = e.y();
        } else if(cmd.cmd == 's') {
            QVector2D s(mX, mY);
            cmd.absStart = s;
            QVector2D c0; // to be calculated from previous command operands
            QVector2D c1(cmd.ops[0], cmd.ops[1]); c1 += s;
            QVector2D e(cmd.ops[2], cmd.ops[3]); e += s;
            if(i > 0) {
                cSVGCmd& prevCmd = mCommands[i-1];
                if(prevCmd.cmd == 'c') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'C') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]);
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]);
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 's') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'S') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]);
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]);
                    c0 = pe + (pe - pcp1);
                } else {
                    qd << "CMD" << cmd.cmd << "prev" << prevCmd.cmd;
                    Q_ASSERT(false);
                }

                std::vector<QVector2D> res;
                for(float t=0.1; t<=0.9; t+=0.1)
                    res.push_back( bezier(s, e, c0, c1, t) );

                appendPoint(s);
                appendPoints(res);
                appendPoint(e);

                mX = e.x();
                mY = e.y();
            }
        }
        else if(cmd.cmd == 'S') {
            QVector2D s(mX, mY);
            cmd.absStart = s;
            QVector2D c0; // to be calc
            QVector2D c1(cmd.ops[0], cmd.ops[1]);
            QVector2D e(cmd.ops[2], cmd.ops[3]);
            if(i > 0) {
                cSVGCmd& prevCmd = mCommands[i-1];
                if(prevCmd.cmd == 'c') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'C') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]);
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]);
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 's') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'S') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]);
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]);
                    c0 = pe + (pe - pcp1);
                } else {
                    qd << "CMD" << cmd.cmd << "prev" << prevCmd.cmd;
                    Q_ASSERT(false);
                }

                std::vector<QVector2D> res;
                for(float t=0.1; t<=0.9; t+=0.1)
                    res.push_back( bezier(s, e, c0, c1, t) );

                appendPoint(s);
                appendPoints(res);
                appendPoint(e);

                mX = e.x();
                mY = e.y();
            }
        } else if(cmd.cmd == 'm') {
            if(i==0) { // the first 'm' is meant to be absolute
                mX = cmd.ops[0];
                mY = cmd.ops[1];
            }else {
                mX += cmd.ops[0];
                mY += cmd.ops[1];
            }
            qd << mX;
            qd << mY;
            appendPoint(mX, mY);
            continue;
        } else if(cmd.cmd == 'M') {
            mX = cmd.ops[0];
            mY = cmd.ops[1];
            qd << mX;
            qd << mY;
            appendPoint(mX, mY);
            continue;
        }
        else if(cmd.cmd == "z"){
            qd << "close path";
            continue;
        } else if(cmd.cmd == 'l') {
            QVector2D begin(mX, mY);
            mX += cmd.ops[0];
            mY += cmd.ops[1];
            appendSegment(Seg2f(begin, {mX, mY}));
        } else if(cmd.cmd == 'L') {
            QVector2D begin(mX, mY);
            mX = cmd.ops[0];
            mY = cmd.ops[1];
            appendSegment(Seg2f(begin, {mX, mY}));
        } else if(cmd.cmd == 'H') {
            mX = cmd.ops[0];
        } else if(cmd.cmd == 'V') {
            mY = cmd.ops[0];
        } else if(cmd.cmd == 'h') {
            QVector2D begin(mX, mY);
            mX += cmd.ops[0];
            appendSegment(Seg2f(begin, {mX, mY}));
        } else if(cmd.cmd == 'v') {
            QVector2D begin(mX, mY);
            mY += cmd.ops[0];
            appendSegment(Seg2f(begin, {mX, mY}));
        }

        // insert the calculated point
        qd << mX;
        qd << mY;

        appendPoint(mX, mY);
    }

    flipY();
    //scale(3.0f);
}

void SVG::flipY()
{
    for(auto& p : mPoints) {
        p.setY(-p.y());
    }
}

void SVG::translate(QVector2D v)
{
    for(auto& p : mPoints) {
        p += v;
    }
}

bool SVG::isNum(QChar c)
{
    if((c >= '0' && c<='9') || c=='.') return true;
    return false;
}

QVector2D SVG::bezier(QVector2D s, QVector2D e, QVector2D c0, QVector2D c1, const float f)
{
    QVector2D q0 =  s +  (c0 - s) * f;
    QVector2D q1 = c0 + (c1 - c0) * f;
    QVector2D q2 = c1 +  (e - c1) * f;
    QVector2D r0 = q0 + (q1 - q0) * f;
    QVector2D r1 = q1 + (q2 - q1) * f;
    return r0 + (r1 - r0) * f;
}
