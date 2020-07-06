#include <QSet>
#include <QFile>
#include <QtXml>
#include <assert.h>
#include <QFileInfo>
#include <QPushButton>
#include <QLineEdit>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"


//===============================================================================================================
// MAIN WINDOW
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Setup
    ui->setupUi(this);
    QApplication::setApplicationName(VER_PRODUCTNAME_STR);
    setWindowTitle(VER_PRODUCTNAME_STR);


}

//-Destructor----------------------------------------------------------------------------------------------------
MainWindow::~MainWindow() { delete ui; }

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
void MainWindow::initializeForms()
{

}

//-Slots---------------------------------------------------------------------------------------------------------
//Private:
void MainWindow::all_on_pushButton_clicked()
{
    // Get the object that called this slot
    QPushButton *senderPushButton = qobject_cast<QPushButton *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderPushButton == nullptr)
        assert("Pointer conversion to push button failed");

    // Determine sender and take corresponding action
    if(senderPushButton == ui->pushButton_launchBoxBrowse)
    {

    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {

    }
    else
        assert("Unhandled use of all_on_pushButton_clicked() slot");
}

void MainWindow::all_on_linedEdit_textEdited()
{
    // Get the object that called this slot
    QLineEdit *senderLineEdit = qobject_cast<QLineEdit *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        assert("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
    {

    }
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
    {

    }
    else
        assert("Unhandled use of all_on_linedEdit_textEdited() slot");
}


