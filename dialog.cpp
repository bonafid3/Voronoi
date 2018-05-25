#include "dialog.h"
#include "ui_dialog.h"

#define JC_VORONOI_IMPLEMENTATION

#include "jc_voronoi.h"
#include "clipper.h"

#include <QFileDialog>
#include <QMessageBox>

#include "utils.h"

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

QString Dialog::DXF_Line(int id, double x1, double y1, double z1, double x2, double y2, double z2)
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

    points = (jcv_point*)malloc( sizeof(jcv_point) * horizontal * vertical);
    if( !points )
        return;

    srand(0);

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

        co.Execute(solution, -ui->mOffset->value());

        for(int s=0; s<solution.size(); s++)
        {
            for(int i=0; i<solution[s].size(); i++)
            {
                int j=i+1; if(j==solution[s].size()) j=0;
                //qd << solution[s][i].X << solution[s][i].Y;
                mScene->addLine(solution[s][i].X, solution[s][i].Y, solution[s][j].X, solution[s][j].Y);

                QString line = DXF_Line(id++, solution[s][i].X, solution[s][i].Y, 0, solution[s][j].X, solution[s][j].Y, 0);
                appendFile(ui->mDXFFile->text(), line.toUtf8());
            }
        }
    }

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
