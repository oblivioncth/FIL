#include "importprocessdialog.h"
#include "ui_importprocessdialog.h"

//===============================================================================================================
// IMPORT PROCESS DIALOG
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
ImportProcessDialog::ImportProcessDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ImportProcessDialog)
{
    ui->setupUi(this);
}

//-Destructor----------------------------------------------------------------------------------------------------
ImportProcessDialog::~ImportProcessDialog() { delete ui; }
