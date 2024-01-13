#include "dialog.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QSurfaceFormat format;

    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4, 5);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    Dialog w;
    w.setWindowFlags(Qt::Window);
    w.show();

    return a.exec();
}
