#include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    MainWindow w;

    if(!w.initCompleted())
        return 1;

    w.show();
    return a.exec();
}
