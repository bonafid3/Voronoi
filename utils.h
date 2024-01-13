#ifndef UTILS_H
#define UTILS_H

#include <QFile>
#include <QDebug>
#include <QByteArray>
#include <QVector2D>

#define qd qDebug()
#define ftoStr(x) QString::number(x, 'f', 2)
#define toStr(x) QString::number(x)

inline float d2r(float deg)
{
    return deg * 0.0174533f;
}

inline float r2d(const float rad)
{
    return rad * 57.2958f;
}

inline QVector2D vRot90(const QVector2D v) {
    QVector2D res;
    res.setX(v.y());
    res.setY(-v.x());
    return res;
}

inline float vDot(const QVector2D& a, const QVector2D& b)
{
    return a.x() * b.x() + a.y() * b.y();
}

inline float vCrossZ(const QVector2D & a, const QVector2D & b)
{
    return a.x() * b.y() - a.y() * b.x();
}

inline QByteArray readFile(const QString fname)
{
    QFile f(fname);
    f.open(QFile::ReadOnly);
    QByteArray data = f.readAll();
    f.close();
    return data;
}

inline void writeFile(const QString fname, QByteArray data)
{
    QFile f(fname);
    f.open(QFile::WriteOnly);
    f.write(data);
    f.close();
}

inline void appendFile(const QString fname, QByteArray data)
{
    QFile f(fname);
    f.open(QFile::Append);
    f.write(data);
    f.close();
}

#endif // UTILS_H
