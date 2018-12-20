#include "dialog.h"
#include "ui_dialog.h"

#define JC_VORONOI_IMPLEMENTATION

#include "jc_voronoi.h"
#include "clipper.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <cmath>

#include "utils.h"

enum { STRAIGHT=0, CURVED };

using namespace ClipperLib;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    mScene = new QGraphicsScene(this);

    ui->graphicsView->setScene(mScene);
}

int Dialog::random(int min, int max)
{
    return rand() % (max + 1 - min) + min;
}

QString Dialog::DXF_Line(int id, float x1, float y1, float z1, float x2, float y2, float z2)
{
    char b[4096]={0};
    int length;
    length = sprintf(b,
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
    return res;
}

void Dialog::voronoi()
{
    if(ui->mDXFFile->text().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please specify an output DXF file.");
        return;
    }

    jcv_point* points = 0;

    int horizontal = ui->mHorizontal->value();
    int vertical = ui->mVertical->value();
    int spacing = ui->mSpacing->value();
    int disturbance = ui->mDisturbance->value();
    int vtype = ui->mType->currentIndex();
    int offs = ui->mOffset->value();

    points = (jcv_point*)malloc( sizeof(jcv_point) * horizontal * vertical);
    if( !points )
        return;

    srand(QDateTime::currentSecsSinceEpoch());

    int pos=0;
    for(int j=0; j < vertical; ++j)
        for(int i = 0; i < horizontal; ++i)
        {
            points[pos].x = (float)(i*spacing) + random(-disturbance, disturbance);
            points[pos].y = (float)(j*spacing) + random(-disturbance, disturbance);

            qd << points[pos].x << points[pos].y;
            pos++;
        }

    jcv_rect* rect = 0;

    jcv_diagram diagram;
    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_diagram_generate(horizontal*vertical, (const jcv_point*)points, rect, &diagram);

    writeFile(ui->mDXFFile->text(), readFile(":/dxf1.txt"));

    int id=115;

    float minX=FLT_MAX, minY=FLT_MAX;
    float maxX=FLT_MIN, maxY=FLT_MIN;

    const jcv_site* sites = jcv_diagram_get_sites( &diagram );
    for( int i = 0; i < diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];

        QPen pen = QPen(QColor(random(0,255),random(0,255),random(0,255)));
        pen.setWidth(2);

        ClipperOffset co;
        Path poly;

        const jcv_graphedge* e = site->edges;
        while( e )
        {
            //mScene->addLine(e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y, pen);
            poly << IntPoint(e->pos[0].x, e->pos[0].y) << IntPoint(e->pos[1].x, e->pos[1].y);
            e = e->next;
        }

        co.AddPath(poly, jtRound, etClosedPolygon);

        Paths solution;

        Seg2f S1, S2;
        std::vector<Seg2f> segs;

        co.Execute(solution, -offs);

        for(int s=0; s<solution.size(); s++)
        {
            segs.clear();// for now

            for(int i=0; i<solution[s].size(); i++)
            {
                int j=i+1; if(j==solution[s].size()) j=0;
                //qd << solution[s][i].X << solution[s][i].Y;

                QVector2D v1(solution[s][i].X, solution[s][i].Y);
                QVector2D v2(solution[s][j].X, solution[s][j].Y);

                if(v1.x() < minX) minX = v1.x();
                else if(v1.x() > maxX) maxX = v1.x();
                if(v2.x() < minX) minX = v2.x();
                else if(v2.x() > maxX) maxX = v2.x();

                if(v1.y() < minY) minY = v1.y();
                else if(v1.y() > maxY) maxY = v1.y();
                if(v2.y() < minY) minY = v2.y();
                else if(v2.y() > maxY) maxY = v2.y();

                if(vtype == STRAIGHT) {
                    mScene->addLine(v1.x(), v1.y(), v2.x(), v2.y());
                    QString line = DXF_Line(id++, v1.x(), v1.y(), 0, v2.x(), v2.y(), 0);
                    appendFile(ui->mDXFFile->text(), line.toUtf8());
                } else {
                    segs.push_back(Seg2f(v1, v2));
                }
            }
        }

        std::vector<QVector2D> pts;
        for(int i=0; i<segs.size(); i++) {

            int j=i+1; if(j==segs.size()) j=0;

            QVector2D v1 = segs[i].p[1] - segs[i].p[0];
            QVector2D v2 = segs[j].p[1] - segs[j].p[0];

            QVector2D nv1 = segs[i].p[0] + v1/2;
            QVector2D nv2 = segs[j].p[0] + v2/2;

            //mScene->addLine(nv1.x(), nv1.y(), nv2.x(), nv2.y());

            for(int t=0; t<=10; t++) {
                pts.push_back(nv1 + bezier(v1/2, v2/2, t*0.1));
            }
        }

        for(int k=0; k<pts.size(); k++) {
            int l=k+1; if(l==pts.size()) l=0;

            mScene->addLine(pts[k].x(), pts[k].y(), pts[l].x(), pts[l].y());

            QString line = DXF_Line(id++, pts[k].x(), pts[k].y(), 0, pts[l].x(), pts[l].y(), 0);
            appendFile(ui->mDXFFile->text(), line.toUtf8());
        }

        /* //simple polygon corner rounding
        for(int i=0; i<segs.size(); i++) {
            int j=i+1; if(j==segs.size()) j=0;
            QVector2D isect = intersectLines_noParallel(segs[i], segs[j]);
            QVector2D oldV1 = segs[i].p[1] - segs[i].p[0];
            float dot = vDot(oldV1, isect-segs[i].p[0]) / oldV1.length();
            QVector2D newV1(oldV1.normalized() * dot);
            newV1 += segs[i].p[0];
            mScene->addLine(isect.x(), isect.y(), newV1.x(), newV1.y());
            mScene->addEllipse(isect.x()-cr, isect.y()-cr, cr*2, cr*2);
        }*/

    }

    minX -= 2*offs;
    minY -= 2*offs;
    maxX += 2*offs;
    maxY += 2*offs;

    mScene->addLine(minX, minY, maxX, minY);
    mScene->addLine(maxX, minY, maxX, maxY);
    mScene->addLine(maxX, maxY, minX, maxY);
    mScene->addLine(minX, maxY, minX, minY);

    QString line = DXF_Line(id++, minX, minY, 0, maxX, minY, 0);
    appendFile(ui->mDXFFile->text(), line.toUtf8());

     line = DXF_Line(id++, maxX, minY, 0, maxX, maxY, 0);
    appendFile(ui->mDXFFile->text(), line.toUtf8());

     line = DXF_Line(id++, maxX, maxY, 0, minX, maxY, 0);
    appendFile(ui->mDXFFile->text(), line.toUtf8());

     line = DXF_Line(id++, minX, maxY, 0, minX, minY, 0);
    appendFile(ui->mDXFFile->text(), line.toUtf8());

    jcv_diagram_free( &diagram );

    appendFile(ui->mDXFFile->text(), readFile(":/dxf2.txt"));

    return;
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_mGenerate_clicked()
{
    mScene->clear();
    voronoi();
}

void Dialog::on_mBrowseButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("DXF file"), "voronoi.dxf", tr("DXF files (*.dxf)"));

    ui->mDXFFile->setText(fileName);
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


// must be between 0 and 1
QVector2D Dialog::bezier(const QVector2D &p1, const QVector2D &p2, const float t)
{
    QVector2D v1 = p1 * t;
    QVector2D v2 = p1 + p2 * t;
    return v1 + (v2 - v1) * t;
}
