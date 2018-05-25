#ifndef UTILS_H
#define UTILS_H

#include <QFile>
#include <QDebug>
#include <QByteArray>

#define qd qDebug()

QByteArray readFile(const QString fname)
{
    QFile f(fname);
    f.open(QFile::ReadOnly);
    QByteArray data = f.readAll();
    f.close();
    return data;
}

void writeFile(const QString fname, QByteArray data)
{
    QFile f(fname);
    f.open(QFile::WriteOnly);
    f.write(data);
    f.close();
}

void appendFile(const QString fname, QByteArray data)
{
    QFile f(fname);
    f.open(QFile::Append);
    f.write(data);
    f.close();
}

#endif // UTILS_H
