#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    QGraphicsScene *mScene=0;

    void voronoi();

private slots:

    void on_mGenerate_clicked();

    void on_mBrowseButton_clicked();

private:
    Ui::Dialog *ui;
    int random(int min, int max);
    QString DXF_Line(int id, double x1, double y1, double z1, double x2, double y2, double z2);
};

#endif // DIALOG_H
