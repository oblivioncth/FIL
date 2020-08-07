#include <QSet>
#include <QFile>
#include <QFileDialog>
#include <QtXml>
#include <assert.h>
#include <QFileInfo>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressDialog>
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
    initializeForms();
}

//-Destructor----------------------------------------------------------------------------------------------------
MainWindow::~MainWindow() { delete ui; }

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
void MainWindow::initializeForms()
{
    mExistingItemColor = ui->label_existingItemColor->palette().color(QPalette::Window);
    setInputStage(PATHS);

    // TODO: THIS IS FOR DEBUG PURPOSES. REMOVE
    checkLaunchBoxInput("D:/LaunchBox");
    checkFlashpointInput("C:/Users/Player/Desktop/FP");
}

void MainWindow::setInputStage(InputStage stage)
{
    switch(stage)
    {
        case PATHS:
            ui->groupBox_importSelection->setEnabled(false);
            mAlteringListWidget = true;
            ui->listWidget_platformChoices->clear();
            ui->listWidget_playlistChoices->clear();
            mAlteringListWidget = false;
            ui->groupBox_updateMode->setEnabled(false);
            ui->radioButton_onlyAdd->setChecked(true);
            ui->radioButton_updateExisting->setChecked(false);
            ui->pushButton_startImport->setEnabled(false);
        break;

        case IMPORTS:
            ui->groupBox_importSelection->setEnabled(true);
        break;
    }
}

void MainWindow::checkLaunchBoxInput(QString installPath)
{
    if(LB::Install::pathIsValidInstall(installPath))
    {
        mLaunchBoxInstall = std::make_unique<LB::Install>(installPath);
        ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Valid_Install.png"));
        if(mFlashpointInstall)
            gatherInstallInfo();
    }
    else
    {
        ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
        mLaunchBoxInstall.reset();
        setInputStage(PATHS);
        QMessageBox::critical(this, QApplication::applicationName(), MSG_LB_INSTALL_INVALID);
    }
}

void MainWindow::checkFlashpointInput(QString installPath)
{
    if(FP::Install::pathIsValidtInstall(installPath))
    {
        mFlashpointInstall = std::make_unique<FP::Install>(installPath);

        if(mFlashpointInstall->matchesTargetVersion())
            ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Valid_Install.png"));
        else
        {
            ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Mismatch_Install.png"));
            QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);
        }


        if(mLaunchBoxInstall)
            gatherInstallInfo();
    }
    else
    {
        ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
        mFlashpointInstall.reset();
        setInputStage(PATHS);
        QMessageBox::critical(this, QApplication::applicationName(), MSG_FP_INSTALL_INVALID);
    }
}

void MainWindow::gatherInstallInfo()
{
    // Get data in order but only continue if each step is successful
    if(parseFlashpointData())
    {
        if(parseLaunchBoxData())
        {
            // Populate import selection boxes
            mAlteringListWidget = true;
            ui->listWidget_platformChoices->addItems(mFlashpointInstall->getPlatformList());
            ui->listWidget_playlistChoices->addItems(mFlashpointInstall->getPlaylistList());

            // Set item attributes
            QListWidgetItem* currentItem;

            for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
            {
                currentItem = ui->listWidget_platformChoices->item(i);
                currentItem->setCheckState(Qt::Unchecked);

                if(mLaunchBoxInstall->getExistingPlatforms().contains(currentItem->text()))
                    currentItem->setBackground(QBrush(mExistingItemColor));
            }

            for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
            {
                currentItem = ui->listWidget_playlistChoices->item(i);
                currentItem->setFlags(currentItem->flags() | Qt::ItemIsUserCheckable);
                currentItem->setCheckState(Qt::Unchecked);

                if(mLaunchBoxInstall->getExistingPlaylists().contains(currentItem->text()))
                    currentItem->setBackground(QBrush(mExistingItemColor));
            }

            mAlteringListWidget = false;

            // Advance to next input stage
            setInputStage(InputStage::IMPORTS);

        }
        else
            ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
    }
    else
        ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
}

bool MainWindow::parseLaunchBoxData()
{
    // IO Error check instance
    Qx::IOOpReport existingCheck;

    // Get list of existing platforms and playlists
    existingCheck = mLaunchBoxInstall->populateExistingItems();

    // IO Error Check
    if(!existingCheck.wasSuccessful())
        postIOError(existingCheck);

    // Return true on success
    return existingCheck.wasSuccessful();
}

bool MainWindow::parseFlashpointData()
{
    // Get and open connection to Flashpoint SQLite database, check that it is valid and allow for retries
    QSqlDatabase fpDB;

    // Check that the connection is valid to allow for retries
    while(!(fpDB = mFlashpointInstall->openDatabaseConnection()).isValid() &&
          QMessageBox::critical(this, QApplication::applicationName(), MSG_FP_DB_CANT_CONNECT, QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry) == QMessageBox::Retry);

    // Check if the connection finally became valid
    if(!fpDB.isValid())
        return false;


    QSqlError errorCheck;

    // Ensure the database contains the required tables
    QSet<QString> missingTables;
    errorCheck = mFlashpointInstall->checkDatabaseForRequiredTables(missingTables);

    // SQL Error Check
    if(errorCheck.isValid())
    {
        postSqlError(errorCheck);
        return false;
    }

    // Check if tables are missing
    if(!missingTables.isEmpty())
    {
        postListError(MSG_FP_DB_MISSING_TABLE, QStringList(missingTables.begin(), missingTables.end()));
        return false;
    }

    // Ensure the database contains the required columns
    QSet<QString> missingColumns;
    errorCheck = mFlashpointInstall->checkDatabaseForRequiredColumns(missingColumns);

    // SQL Error Check
    if(errorCheck.isValid())
    {
        postSqlError(errorCheck);
        return false;
    }

    // Check if columns are missing
    if(!missingColumns.isEmpty())
    {
        postListError(MSG_FP_DB_TABLE_MISSING_COLUMN, QStringList(missingColumns.begin(), missingColumns.end()));
        return false;
    }

    // Get list of available platforms and playlists
    errorCheck = mFlashpointInstall->populateAvailableItems();

    // SQL Error Check
    if(errorCheck.isValid())
        postSqlError(errorCheck);

    // Return true on success
    return !errorCheck.isValid();

}

void MainWindow::postSqlError(QSqlError sqlError)
{
    QMessageBox sqlErrorMsg;
    sqlErrorMsg.setIcon(QMessageBox::Critical);
    sqlErrorMsg.setText(MSG_FP_DB_UNEXPECTED_ERROR);
    sqlErrorMsg.setInformativeText(sqlError.text());
    sqlErrorMsg.setStandardButtons(QMessageBox::Ok);

    sqlErrorMsg.exec();
}

void MainWindow::postListError(QString mainText, QStringList detailedItems)
{
    QMessageBox listError;
    listError.setIcon(QMessageBox::Critical);
    listError.setText(mainText);
    listError.setDetailedText(detailedItems.join("\n"));
    listError.setStandardButtons(QMessageBox::Ok);

    listError.exec();
}

void MainWindow::postIOError(Qx::IOOpReport report)
{
    QMessageBox ioErrorMsg;
    ioErrorMsg.setIcon(QMessageBox::Critical);
    ioErrorMsg.setText(MSG_LB_XML_UNEXPECTED_ERROR);
    ioErrorMsg.setInformativeText(report.getOutcome());
    ioErrorMsg.setStandardButtons(QMessageBox::Ok);

    ioErrorMsg.exec();
}

void MainWindow::importSelectionReaction(QListWidgetItem* item, QWidget* parent)
{
    if(item->checkState() == Qt::Checked)
    {
        ui->pushButton_startImport->setEnabled(true);
        ui->groupBox_updateMode->setEnabled((parent == ui->listWidget_platformChoices && mLaunchBoxInstall->getExistingPlatforms().contains(item->text())) ||
                                            (parent == ui->listWidget_playlistChoices && mLaunchBoxInstall->getExistingPlaylists().contains(item->text())));
    }
    else
    {
        bool keepUpdateGroupEnabled = false;
        bool keepStartButtonEnabled = false;

        // Check platform choices
        for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
        {
            if(ui->listWidget_platformChoices->item(i)->checkState() == Qt::Checked)
            {
                keepStartButtonEnabled = true;

                if(mLaunchBoxInstall->getExistingPlatforms().contains(ui->listWidget_platformChoices->item(i)->text()))
                    keepUpdateGroupEnabled = true;
            }
        }

        // Check playlist choices if needed
        if(!keepUpdateGroupEnabled || !keepStartButtonEnabled)
        {
            for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
            {
                if(ui->listWidget_playlistChoices->item(i)->checkState() == Qt::Checked)
                {
                    keepStartButtonEnabled = true;

                    if(mLaunchBoxInstall->getExistingPlaylists().contains(ui->listWidget_playlistChoices->item(i)->text()))
                        keepUpdateGroupEnabled = true;
                }
            }
        }

        // Apply state changes
        ui->groupBox_updateMode->setEnabled(keepUpdateGroupEnabled);
        ui->pushButton_startImport->setEnabled(keepStartButtonEnabled);
    }

}

QStringList MainWindow::getSelectedPlatforms() const
{
    QStringList selectedPlatforms;

    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
        if(ui->listWidget_platformChoices->item(i)->checkState() == Qt::Checked)
            selectedPlatforms.append(ui->listWidget_platformChoices->item(i)->text());

    return selectedPlatforms;
}

QStringList MainWindow::getSelectedPlaylists() const
{
    QStringList selectedPlaylists;

    for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
        if(ui->listWidget_playlistChoices->item(i)->checkState() == Qt::Checked)
            selectedPlaylists.append(ui->listWidget_playlistChoices->item(i)->text());

    return selectedPlaylists;
}

LB::Install::UpdateOptions MainWindow::getSelectedUpdateOptions() const
{
    return {ui->radioButton_onlyAdd->isChecked() ? LB::Install::OnlyNew : LB::Install::NewAndExisting, ui->checkBox_removeObsolete->isChecked() };
}

void MainWindow::importProcess()
{
    // Grab options
    QStringList platformsToImport = getSelectedPlatforms();
    QStringList playlistsToImport = getSelectedPlaylists();
    LB::Install::UpdateOptions updateOptions = getSelectedUpdateOptions();

    // Process tools
    QSqlError queryError;
    int totalSteps;

    // Lookup caches
    QSet<QString> targetGameIDs;

    // Initial query buffers
    QList<FP::Install::DBQueryBuffer> gameQueries;
    FP::Install::DBQueryBuffer addAppQuery;
    FP::Install::DBQueryBuffer playlistQueries;
    QList<FP::Install::DBQueryBuffer> playlistGameQueries;

    // Create progress dialog, set initial busy state and show
    QProgressDialog importProgressDialog(PD_LABEL_FP_DB_INITIAL_QUERY, PD_BUTTON_CANCEL, 0, 0);
    importProgressDialog.setWindowModality(Qt::WindowModal);
    importProgressDialog.show();

    // Make initial game query
    queryError = mFlashpointInstall->initialGameQuery(gameQueries, platformsToImport);
    if(queryError.isValid())
    {
        postSqlError(queryError);
        return;
    }

    // Make initial add apps query
    queryError = mFlashpointInstall->initialAddAppQuery(addAppQuery);
    if(queryError.isValid())
    {
        postSqlError(queryError);
        return;
    }

    // Make initial playlists query
    queryError = mFlashpointInstall->initialPlaylistQuery(playlistQueries, playlistsToImport);
    if(queryError.isValid())
    {
        postSqlError(queryError);
        return;
    }

    // Build ID list for playlist game query
    QList<FP::Install::DBPlaylist> targetKnownPlaylists;
    for(int i = 0; i < playlistQueries.size; i++)
    {
        playlistQueries.result.next(); // Advance to next record
        targetKnownPlaylists.append({playlistQueries.result.value(FP::Install::DBTable_Playlist::COL_TITLE).toString(),
                                     QUuid(playlistQueries.result.value(FP::Install::DBTable_Playlist::COL_ID).toString())});
    }

    // Make initial playlist games query
    queryError = mFlashpointInstall->initialPlaylistGameQuery(playlistGameQueries, targetKnownPlaylists);
    if(queryError.isValid())
    {
        postSqlError(queryError);
        return;
    }

    // Determine workload
    //totalSteps = gameQueries.size() +


}

//-Slots---------------------------------------------------------------------------------------------------------
//Private:
void MainWindow::all_on_pushButton_clicked()
{
    // Get the object that called this slot
    QPushButton* senderPushButton = qobject_cast<QPushButton *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderPushButton == nullptr)
        assert("Pointer conversion to push button failed");

    // Determine sender and take corresponding action
    if(senderPushButton == ui->pushButton_launchBoxBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_LAUNCHBOX_BROWSE, (QFileInfo::exists(ui->lineEdit_launchBoxPath->text()) ? ui->lineEdit_launchBoxPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_launchBoxPath->setText(QDir::toNativeSeparators(selectedDir));
            checkLaunchBoxInput(selectedDir);
        }
    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE, (QFileInfo::exists(ui->label_flashPointPath->text()) ? ui->label_flashPointPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_flashpointPath->setText(QDir::toNativeSeparators(selectedDir));
            checkFlashpointInput(selectedDir);
        }
    }
    else if(senderPushButton == ui->pushButton_selectAll_platforms)
    {
        for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
            ui->listWidget_platformChoices->item(i)->setCheckState(Qt::Checked);
    }
    else if(senderPushButton == ui->pushButton_selectNone_platforms)
    {
        for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
            ui->listWidget_platformChoices->item(i)->setCheckState(Qt::Unchecked);
    }
    else if(senderPushButton == ui->pushButton_selectAll_playlists)
    {
        for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
            ui->listWidget_playlistChoices->item(i)->setCheckState(Qt::Checked);
    }
    else if(senderPushButton == ui->pushButton_selectNone_playlists)
    {
        for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
            ui->listWidget_playlistChoices->item(i)->setCheckState(Qt::Unchecked);
    }
    else if(senderPushButton == ui->pushButton_updateModeHelp)
        QMessageBox::information(this, CAPTION_UPDATE_MODE_HELP, MSG_UPDATE_MODE_HELP);
    else if(senderPushButton == ui->pushButton_startImport)
        importProcess();
    else if(senderPushButton == ui->pushButton_exit)
        QApplication::quit();
    else
        assert("Unhandled use of all_on_pushButton_clicked() slot");
}

void MainWindow::all_on_lineEdit_editingFinished()
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        assert("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
    {
        if(!mLineEdit_launchBoxPath_blocker)
        {
            QFileInfo selectedDir = QFileInfo(QDir::cleanPath(QDir::fromNativeSeparators(ui->lineEdit_launchBoxPath->text())));

            if(selectedDir.exists() && selectedDir.isDir())
                checkLaunchBoxInput(selectedDir.absolutePath());
            else
            {
                ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
                setInputStage(PATHS);
            }
        }
        else
            mLineEdit_launchBoxPath_blocker--;
    }
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
    {
        if(!mLineEdit_flashpointPath_blocker)
        {
            QFileInfo selectedDir = QFileInfo(QDir::cleanPath(ui->lineEdit_flashpointPath->text()));

            if(selectedDir.exists() && selectedDir.isDir())
                checkFlashpointInput(selectedDir.absolutePath());
            else
            {
                ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
                setInputStage(PATHS);
            }
        }
        else
            mLineEdit_flashpointPath_blocker--;
    }
    else
        assert("Unhandled use of all_on_linedEdit_textEdited() slot");
}

void MainWindow::all_on_lineEdit_textEdited() // Required due to an oversight with QLineEdit::editingFinished() TODO: Make sure this works
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        assert("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
        mLineEdit_launchBoxPath_blocker = 0;
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
        mLineEdit_flashpointPath_blocker = 0;
    else
        assert("Unhandled use of all_on_linedEdit_textEdited() slot");
}

void MainWindow::all_on_lineEdit_returnPressed() // Required due to an oversight with QLineEdit::editingFinished() TODO: Make sure this works
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        assert("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
        mLineEdit_launchBoxPath_blocker = 2;
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
        mLineEdit_flashpointPath_blocker = 2;
    else
        assert("Unhandled use of all_on_linedEdit_returnPressed() slot");
}

void MainWindow::all_on_listWidget_itemChanged(QListWidgetItem* item) // Proxy for "onItemChecked"
{
    // Get the object that called this slot
    QListWidget* senderListWidget = qobject_cast<QListWidget *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderListWidget == nullptr)
        assert("Pointer conversion to line edit failed");

    if(senderListWidget == ui->listWidget_platformChoices && !mAlteringListWidget)
        importSelectionReaction(item, ui->listWidget_platformChoices);
    else if(senderListWidget == ui->listWidget_playlistChoices && !mAlteringListWidget)
        importSelectionReaction(item, ui->listWidget_playlistChoices);
    else
        assert("Unhandled use of all_on_listWidget_itemChanged() slot");
}
