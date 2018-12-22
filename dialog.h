#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QVector2D>
#include <QGraphicsScene>

namespace Ui {
class Dialog;
}

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

    bool isValid() { return valid; }

    QVector2D p[2];
private:
    bool valid = false;
};

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

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    QGraphicsScene *mScene=0;

    void voronoi();

    QVector2D intersectLines_noParallel(const Seg2f& S0, const Seg2f& S1);
    QVector2D bezier(const QVector2D &s, const QVector2D &e, const QVector2D &c, const float t);

private slots:

    void on_mGenerate_clicked();

    void on_mBrowseButton_clicked();

private:
    Ui::Dialog *ui;
    int random(int min, int max);
    QString DXF_Line(int id, float x1, float y1, float z1, float x2, float y2, float z2);
};

#endif // DIALOG_H
