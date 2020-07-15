#ifndef IMPORTPROCESSDIALOG_H
#define IMPORTPROCESSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ImportProcessDialog; }
QT_END_NAMESPACE


class ImportProcessDialog : public QDialog
{
    Q_OBJECT // Required for classes that use Qt elements

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    Ui::ImportProcessDialog *ui;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    explicit ImportProcessDialog(QWidget *parent = nullptr);

//-Destructor----------------------------------------------------------------------------------------------------
public:
    ~ImportProcessDialog();
};

#endif // IMPORTPROCESSDIALOG_H
