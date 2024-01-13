#include "dialog.h"
#include "ui_dialog.h"

#define JC_VORONOI_IMPLEMENTATION

#include "jc_voronoi.h"
#include "clipper.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QMouseEvent>
#include <cmath>

#include "utils.h"
#include "polysegs.h"

#define M 1000.f

using namespace ClipperLib;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    connect(ui->mGLWidget, &cGLWidget::glInitialized, this, &Dialog::onGlInitialized);
    connect(ui->mGLWidget, &cGLWidget::leftMouseButtonPressed, this, &Dialog::onLeftMouseButtonPressed);
    connect(ui->mGLWidget, &cGLWidget::rightMouseButtonPressed, this, &Dialog::onRightMouseButtonPressed);
    connect(ui->mGLWidget, &cGLWidget::mouseButtonClicked, this, &Dialog::onMouseButtonClicked);
    connect(ui->mGLWidget, &cGLWidget::mouseMove, this, &Dialog::onMouseMove);
    connect(ui->mGLWidget, &cGLWidget::dragStarted, this, &Dialog::onDragStarted);
    connect(ui->mGLWidget, &cGLWidget::dropped, this, &Dialog::onDropped);

    mDXF.processDXF(":/default.dxf");

    ui->mGLWidget->setFocus();

    mDragSubjectIterator = mPoints.end();
}

void Dialog::onGlInitialized()
{
    drawScene();
}

void Dialog::drawScene()
{
    ui->mGLWidget->clearVBOs();
    drawSystem();
    drawControlPoints();
    if(ui->showClipperCheckbox->isChecked()) {
        drawClipper();
    }
}

void Dialog::onDragStarted(QVector3D p)
{
    auto it = findMinDistPoint(p.toVector2D());
    if(it != mPoints.end()) {
        mDragSubjectIterator = it;
        int x, y;
        ui->mGLWidget->worldToScreen(QVector3D(*it, 0), x, y);
        QPoint cp = ui->mGLWidget->mapToGlobal(QPoint(x, y));
        QCursor::setPos(cp.x(), cp.y());
    }
}

void Dialog::onDropped(QVector3D p)
{
    if(mDragSubjectIterator != mPoints.end()) {
        mDragSubjectIterator->setX(p.x());
        mDragSubjectIterator->setY(p.y());
        mDragSubjectIterator = mPoints.end();
        drawScene();
        voronoi();
    }
}

void Dialog::onMouseMove(QVector3D p)
{
    if(mDragSubjectIterator != mPoints.end()) {
        mDragSubjectIterator->setX(p.x());
        mDragSubjectIterator->setY(p.y());
        drawScene();
        voronoi();
    }
}

void Dialog::onLeftMouseButtonPressed(QVector3D p)
{
    // does nothing atm
}

void Dialog::onRightMouseButtonPressed(QVector3D p)
{
    if(mPoints.size()) {
        mPoints.erase(findMinDistPoint(p.toVector2D()));
        mDragSubjectIterator = mPoints.end();
    }
    qd << "have points: " << mPoints.size();
    drawScene();
    voronoi();
}

void Dialog::onMouseButtonClicked(QVector3D p)
{
    mPoints.push_back( p.toVector2D() );
    mDragSubjectIterator = mPoints.end();
    drawScene();
    voronoi();
}

std::vector<QVector2D>::iterator Dialog::findMinDistPoint(QVector2D p)
{
    float minDist = std::numeric_limits<float>::max();
    std::vector<QVector2D>::iterator res = mPoints.end(); // end means nothing found!
    for(auto it = mPoints.begin(); it!=mPoints.end(); ++it) {
        float dist = (p - *it).length();
        if(dist < minDist) {
            minDist = dist;
            res = it;
        }
    }
    return res;
}

void Dialog::drawClipper()
{
    ui->mGLWidget->addToVBO(mDXF.segments().glFloatArray(), GL_LINES, QVector4D(0.1, 0.1, 1, 1));
}

void Dialog::drawSystem()
{
    PolySegs system;
    for(int i=-2000; i<=2000; i+=100)
    {
        system.push_back(Seg2f(QVector2D(i,-2000), QVector2D(i, 2000)));
        system.push_back(Seg2f(QVector2D(-2000,i), QVector2D(2000,i)));
    }

    ui->mGLWidget->addToVBO(system.glFloatArray(-2.5f), GL_LINES, QVector4D(0.2, 0.2, 0.2, 0.25));
}


int Dialog::random(int min, int max)
{
    return rand() % (max + 1 - min) + min;
}

QString Dialog::DXF_Line(int id, float x1, float y1, float z1, float x2, float y2, float z2)
{
    setlocale(LC_NUMERIC, "C");
    char b[4096]={0};
    sprintf(b,
                     "LINE\r\n"
                     "5\r\n"
                     "%02X\r\n"
                     "330\r\n"
                     "1F\r\n"
                     "100\r\n"
                     "AcDbEntity\r\n"
                     "8\r\n"
                     "0\r\n"
                     "6\r\n"
                     "Continuous\r\n"
                     "62\r\n"
                     "7\r\n"
                     "100\r\n"
                     "AcDbLine\r\n"
                     "10\r\n"
                     "%f\r\n"
                     "20\r\n"
                     "%f\r\n"
                     "30\r\n"
                     "%f\r\n"
                     "11\r\n"
                     "%f\r\n"
                     "21\r\n"
                     "%f\r\n"
                     "31\r\n"
                     "%f\r\n"
                     "0\r\n",id, x1, y1, z1, x2, y2, z2);

    QString res(b);
    setlocale(LC_ALL, "");
    return res;
}

void Dialog::voronoi(const bool saveFile)
{
    if(ui->mDXFFile->text().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please specify an output DXF file.");
        return;
    }

    if(mPoints.empty())
        return;

    ui->mGLWidget->clearVBOs();

    drawScene();

    jcv_point* points = 0;

    float curves = ui->mCurves->value()/100.f;
    int offs = ui->mOffset->value() * M / 2;

    points = (jcv_point*)malloc( sizeof(jcv_point) * mPoints.size());
    if( !points )
        return;

    srand(QDateTime::currentSecsSinceEpoch());

    int idx=0;
    for(auto& p : mPoints) {
        points[idx].x = p.x();
        points[idx].y = p.y();
        qd << points[idx].x << points[idx].y;
        idx++;
    }

    jcv_rect* rect = 0;

    jcv_diagram diagram;
    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_diagram_generate(mPoints.size(), (const jcv_point*)points, rect, nullptr, &diagram);

    if(saveFile)
        writeFile(ui->mDXFFile->text(), readFile(":/dxf1.txt"));

    int id=115;

    Path mainPoly;
    auto& s = mDXF.segments();
    for(int i=0; i<s.size(); i++)
    {
        mainPoly << IntPoint(s[i].p0().x()*M, s[i].p0().y()*M) << IntPoint(s[i].p1().x()*M, s[i].p1().y()*M);
    }

    const jcv_site* sites = jcv_diagram_get_sites( &diagram );
    for( int i = 0; i < diagram.numsites; ++i )
    {
        ClipperOffset co;
        Paths solution;
        Path poly;

        float alpha = ui->mAlpha->value();
        float tension = ui->mTension->value();

        PolySegs segs;

        const jcv_site* site = &sites[i];
        const jcv_graphedge* e = site->edges;
        while( e ) {
            poly << IntPoint(e->pos[0].x*M, e->pos[0].y*M) << IntPoint(e->pos[1].x*M, e->pos[1].y*M);
            e = e->next;
        }

        co.AddPath(poly, jtRound, etClosedPolygon);
        solution.clear();
        co.Execute(solution, -offs);

        // should be only one solution for each site
        for(int s=0; s<solution.size(); s++) {
            segs.clear();
            mPath.mPoints.clear();

            for(int i=0; i<solution[s].size(); i++) {
                int j=i+1; if(j==solution[s].size()) j=0;

                QVector2D v1(solution[s][i].X/M, solution[s][i].Y/M);
                QVector2D v2(solution[s][j].X/M, solution[s][j].Y/M);

                segs.push_back(Seg2f(v1, v2));
                mPath.mPoints.push_back(v1);
            }
        }

        std::vector<QVector2D> pts;

        if(ui->mSplineGroupBox->isChecked()) {
            float totalLen = mPath.calcTotalLen(alpha, tension);
            qd << "total len:" << totalLen;

            float samplingStep = totalLen / ui->mSamples->value();
            for(float dist=0.f; dist<=totalLen; dist+=samplingStep) {
                float t = mPath.getT(dist, alpha, tension);

                if(t<0.f) {
                    qDebug() << dist << "is over" << totalLen;
                    continue;
                }

                pts.push_back( mPath.getSplinePoint(t, alpha, tension) );
            }
        } else if(ui->mBezierGroupBox->isChecked()) {
            for(int i=0; i<segs.size(); i++) {
                int j=i+1; if(j==segs.size()) j=0;

                QVector2D v1 = segs[i].p[1] - segs[i].p[0];
                QVector2D v2 = segs[j].p[1] - segs[j].p[0];

                QVector2D nv1 = segs[i].p[0] + v1*(1-(curves/2));
                QVector2D nv2 = segs[j].p[0] + v2*   (curves/2);

                for(int t=0; t<=10; t+=1) {
                    pts.push_back(bezier(nv1, nv2, segs[i].p[1], t*0.1));
                }
            }
        } else {
            for(int i=0; i<segs.size(); i++) {
                pts.push_back(QVector2D(segs[i].p[0].x(), segs[i].p[0].y()));
            }
        }

        poly.clear();

        for(int k=0; k<pts.size(); k++) {
            int l=k+1; if(l==pts.size()) l=0;
            poly << IntPoint(pts[k].x()*M, pts[k].y()*M) << IntPoint(pts[l].x()*M, pts[l].y()*M);
        }

        Clipper clipper;
        clipper.AddPath(mainPoly, ptSubject, true);
        clipper.AddPath(poly, ptClip, true);
        clipper.Execute(ctIntersection, solution);
        if(!solution.empty()) {
            for(int s=0; s<solution.size(); s++) {
                segs.clear();
                for(int i=0; i<solution[s].size(); i++) {
                    int j=i+1; if(j==solution[s].size()) j=0;

                    QVector2D v1(solution[s][i].X/M, solution[s][i].Y/M);
                    QVector2D v2(solution[s][j].X/M, solution[s][j].Y/M);

                    segs.push_back(Seg2f(v1, v2));

                    if(saveFile) {
                        QString line = DXF_Line(id++, v1.x(), v1.y(), 0, v2.x(), v2.y(), 0);
                        appendFile(ui->mDXFFile->text(), line.toUtf8());
                    }

                }
                ui->mGLWidget->addToVBO(segs.glFloatArray(), GL_LINES, QVector4D(0.1, 0.1, 0.1, 1));
            }
        }
    }

    jcv_diagram_free( &diagram );

    if(saveFile)
        appendFile(ui->mDXFFile->text(), readFile(":/dxf2.txt"));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_mGenerate_clicked()
{
    voronoi(true);
}

void Dialog::on_mBrowseButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("DXF file"), "voronoi.dxf", tr("DXF files (*.dxf)"));

    ui->mDXFFile->setText(fileName);
}

void Dialog::drawControlPoints()
{
    const float r=1;
    for(auto v: mPoints) {
        PolySegs segs;
        for(int i=0; i<=360; i+=10) {
            float phi = d2r(i);
            float nextPhi = d2r(i+10);
            segs.push_back(Seg2f(QVector2D(sin(phi)*r, cos(phi)*r), QVector2D(sin(nextPhi)*r, cos(nextPhi)*r)));
        }
        segs.translate(v);
        ui->mGLWidget->addToVBO(segs.glFloatArray(0.5f), GL_LINES, QVector4D(0,0,0,1));
    }
}

QVector2D Dialog::intersectLines_noParallel(const Seg2f& S0, const Seg2f& S1)
{
    const QVector2D s(S1.p[0] - S0.p[0]);
    const QVector2D t(S0.p[1] - S0.p[0]);
    const QVector2D u(S1.p[0] - S1.p[1]);

    const float det = vCrossZ(t, u);
    const float detA = vCrossZ(s, u);

    const float alpha = detA / det;

    return S0.p[0] + t * alpha;
}

QVector2D Dialog::bezier(const QVector2D& s, const QVector2D& e, const QVector2D& c, const float t)
{
    QVector2D q0 =  s +   (c - s) * t;
    QVector2D q1 =  c +   (e - c) * t;
    return q0 + (q1 - q0) * t;
}

void Dialog::on_mDXFClipperPolyBrowseButton_clicked()
{
    auto fileName = QFileDialog::getOpenFileName(this, "Open DXF clipper file");
    if(fileName.isEmpty())
        return;

    ui->mDXFClipperPolyFile->setText(fileName);

    mDXF.processDXF(fileName);
    drawScene();
}

void Dialog::on_mCurves_valueChanged(int arg1)
{
    drawScene();
    voronoi();
}


void Dialog::on_mOffset_valueChanged(double arg1)
{
    drawScene();
    voronoi();
}


void Dialog::on_showClipperCheckbox_stateChanged(int arg1)
{
    drawScene();
    voronoi();
}

void Dialog::on_mAlpha_valueChanged(double arg1)
{
    drawScene();
    voronoi();
}

void Dialog::on_mTension_valueChanged(double arg1)
{
    drawScene();
    voronoi();
}

void Dialog::on_mSamples_valueChanged(int arg1)
{
    drawScene();
    voronoi();
}

void Dialog::on_mSplineGroupBox_toggled(bool arg1)
{
    if(ui->mBezierGroupBox->isChecked()) {
        ui->mBezierGroupBox->blockSignals(true);
        ui->mBezierGroupBox->setChecked(false);
        ui->mBezierGroupBox->blockSignals(false);
    }
    drawScene();
    voronoi();
}

void Dialog::on_mBezierGroupBox_toggled(bool arg1)
{
    if(ui->mSplineGroupBox->isChecked()) {
        ui->mSplineGroupBox->blockSignals(true);
        ui->mSplineGroupBox->setChecked(false);
        ui->mSplineGroupBox->blockSignals(false);
    }
    drawScene();
    voronoi();
}
