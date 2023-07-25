#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    if(!w.initCompleted())
    {
        QMessageBox::critical(nullptr, u"Cannot Start"_s, u"Initialization failed!"_s);
        return 1;
    }

    w.show();
    return a.exec();
}
