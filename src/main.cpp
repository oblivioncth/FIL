#include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    MainWindow w;

    if(!w.initCompleted())
    {
        QMessageBox::critical(nullptr, "Cannot Start", "Initialization failed!");
        return 1;
    }

    w.show();
    return a.exec();
}
