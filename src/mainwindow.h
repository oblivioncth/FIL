#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT // Required for classes that use Qt elements

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    Ui::MainWindow *ui;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    MainWindow(QWidget *parent = nullptr);

//-Destructor----------------------------------------------------------------------------------------------------
public:
    ~MainWindow();

//-Instance Functions--------------------------------------------------------------------------------------------
private:
    void initializeForms();

//-Slots---------------------------------------------------------------------------------------------------------
private slots: // Start with "all" to avoid Qt calling "connectSlotsByName" on these slots (slots that start with "on_")
    void all_on_linedEdit_textEdited();
    void all_on_pushButton_clicked();
};
#endif // MAINWINDOW_H
