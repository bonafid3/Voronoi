#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QVector2D>
#include <QGraphicsScene>
#include "dxf.h"
#include "seg2f.h"
#include <QVector3D>
#include "spline.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

    void voronoi(const bool saveFile = false);

    QVector2D intersectLines_noParallel(const Seg2f& S0, const Seg2f& S1);
    QVector2D bezier(const QVector2D &s, const QVector2D &e, const QVector2D &c, const float t);

    void drawSystem();

    void drawClipper();

    std::vector<QVector2D>::iterator findMinDistPoint(QVector2D p);
    void drawControlPoints();
    void drawScene();

protected:
    //bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onGlInitialized();
    void onLeftMouseButtonPressed(QVector3D p);
    void onRightMouseButtonPressed(QVector3D p);
    void onMouseButtonClicked(QVector3D);
    void onMouseMove(QVector3D);
    void onDragStarted(QVector3D);
    void onDropped(QVector3D);


    void on_mGenerate_clicked();
    void on_mBrowseButton_clicked();

    void on_mDXFClipperPolyBrowseButton_clicked();

    void on_mCurves_valueChanged(int arg1);

    void on_mOffset_valueChanged(double arg1);

    void on_showClipperCheckbox_stateChanged(int arg1);

    void on_mAlpha_valueChanged(double arg1);

    void on_mTension_valueChanged(double arg1);

    void on_mSamples_valueChanged(int arg1);

    void on_mSplineGroupBox_toggled(bool arg1);

    void on_mBezierGroupBox_toggled(bool arg1);

    void on_showSitePointsCheckbox_stateChanged(int arg1);

private:

    DXF mDXF;

    Spline mPath;

    std::vector<QVector2D>::iterator mDragSubjectIterator;
    std::vector<QVector2D> mPoints;

    Ui::Dialog *ui;
    int random(int min, int max);
    QString DXF_Line(int id, float x1, float y1, float z1, float x2, float y2, float z2);
};

#endif // DIALOG_H
