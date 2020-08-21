#include <QSet>
#include <QFile>
#include <QFileDialog>
#include <QtXml>
#include <assert.h>
#include <QFileInfo>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <filesystem>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include "qx-windows.h"

//===============================================================================================================
// MAIN WINDOW
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Register metatypes
    qRegisterMetaType<ImportWorker::ImportResult>("ImportResult");
    qRegisterMetaType<Qx::GenericError>("GenericError)");
    qRegisterMetaType<std::shared_ptr<int>>("shared_ptr<int>");

    // General setup
    ui->setupUi(this);
    QApplication::setApplicationName(VER_PRODUCTNAME_STR);
    setWindowTitle(VER_PRODUCTNAME_STR);
    mHasLinkPermissions = testForLinkPermissions();
    initializeForms();

    // Setup UI update workaround timer
    mUIUpdateWorkaroundTimer.setInterval(IMPORT_UI_UPD_INTERVAL);
    connect(&mUIUpdateWorkaroundTimer, &QTimer::timeout, this, &MainWindow::updateUI); // Process events at minimum rate

    // Check if Flashpoint is running
    if(Qx::processIsRunning(QFileInfo(FP::Install::MAIN_EXE_PATH).fileName()))
        QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_CLOSE_PROMPT);
}

//-Destructor----------------------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
bool MainWindow::testForLinkPermissions()
{
    QTemporaryDir testLinkDir;
    if(testLinkDir.isValid())
    {
        QFile testLinkTarget(testLinkDir.filePath("linktarget.tmp"));

        if(testLinkTarget.open(QIODevice::WriteOnly))
        {
            testLinkTarget.close();
            std::error_code symlinkError;
            std::filesystem::create_symlink(testLinkTarget.fileName().toStdString(), testLinkDir.filePath("testlink.tmp").toStdString(), symlinkError);

            if(!symlinkError)
                return true;
        }
    }

    // Default
    return false;
}


void MainWindow::initializeForms()
{
    // Capture existing item color from label for use in platform/playlist selection lists
    mExistingItemColor = ui->label_existingItemColor->palette().color(QPalette::Window);

    // Setup main forms
    ui->radioButton_launchBoxLink->setEnabled(mHasLinkPermissions);
    ui->radioButton_flashpointLink->setEnabled(mHasLinkPermissions);
    ui->radioButton_launchBoxLink->setChecked(mHasLinkPermissions);
    ui->radioButton_launchBoxCopy->setChecked(!mHasLinkPermissions);
    setInputStage(Paths);

    // TODO: THIS IS FOR DEBUG PURPOSES. REMOVE
    checkLaunchBoxInput("D:/LaunchBox");
    checkFlashpointInput("E:/Downloads/qBittorrent/Flashpoint 8.1 Ultimate/Flashpoint");
}

void MainWindow::setInputStage(InputStage stage)
{
    switch(stage)
    {
        case Paths:
            ui->groupBox_importSelection->setEnabled(false);
            mAlteringListWidget = true;
            ui->listWidget_platformChoices->clear();
            ui->listWidget_playlistChoices->clear();
            mAlteringListWidget = false;
            ui->groupBox_updateMode->setEnabled(false);
            ui->groupBox_imageMode->setEnabled(false);
            ui->pushButton_startImport->setEnabled(false);
        break;

        case Imports:
            ui->groupBox_importSelection->setEnabled(true);
            ui->groupBox_imageMode->setEnabled(true);
        break;
    }
}

void MainWindow::checkLaunchBoxInput(QString installPath)
{
    if(LB::Install::pathIsValidInstall(installPath))
    {
        mLaunchBoxInstall = std::make_shared<LB::Install>(installPath);
        ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Valid_Install.png"));
        if(mFlashpointInstall)
            gatherInstallInfo();
    }
    else
    {
        ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
        mLaunchBoxInstall.reset();
        setInputStage(Paths);
        QMessageBox::critical(this, QApplication::applicationName(), MSG_LB_INSTALL_INVALID);
    }
}

void MainWindow::checkFlashpointInput(QString installPath)
{
    if(FP::Install::pathIsValidtInstall(installPath))
    {
        mFlashpointInstall = std::make_shared<FP::Install>(installPath);

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
        setInputStage(Paths);
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
            // Show selections
            populateImportSelectionBoxes();

            // Advance to next input stage
            setInputStage(InputStage::Imports);
        }
        else
        {
            mLaunchBoxInstall.reset();
            ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
            setInputStage(Paths);
        }
    }
    else
    {
        mFlashpointInstall.reset();
        ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
        setInputStage(Paths);
    }
}

void MainWindow::populateImportSelectionBoxes()
{
    // Populate import selection boxes
    mAlteringListWidget = true;
    ui->listWidget_platformChoices->clear();
    ui->listWidget_playlistChoices->clear();
    ui->listWidget_platformChoices->addItems(mFlashpointInstall->getPlatformList());
    ui->listWidget_playlistChoices->addItems(mFlashpointInstall->getPlaylistList());

    // Set item attributes
    QListWidgetItem* currentItem;

    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
    {
        currentItem = ui->listWidget_platformChoices->item(i);
        currentItem->setFlags(currentItem->flags() | Qt::ItemIsUserCheckable);
        currentItem->setCheckState(Qt::Unchecked);

        if(mLaunchBoxInstall->getExistingPlatforms().contains(LB::Install::XMLDoc::makeFileNameLBKosher(currentItem->text())))
            currentItem->setBackground(QBrush(mExistingItemColor));
    }

    for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
    {
        currentItem = ui->listWidget_playlistChoices->item(i);
        currentItem->setFlags(currentItem->flags() | Qt::ItemIsUserCheckable);
        currentItem->setCheckState(Qt::Unchecked);

        if(mLaunchBoxInstall->getExistingPlaylists().contains(LB::Install::XMLDoc::makeFileNameLBKosher(currentItem->text())))
            currentItem->setBackground(QBrush(mExistingItemColor));
    }

    mAlteringListWidget = false;

    // Disable update mode box and import start button since no items will be selected after this operation
    ui->groupBox_updateMode->setEnabled(false);
    ui->pushButton_startImport->setEnabled(false);
}

bool MainWindow::parseLaunchBoxData()
{
    // IO Error check instance
    Qx::IOOpReport existingCheck;

    // Get list of existing platforms and playlists
    existingCheck = mLaunchBoxInstall->populateExistingItems();

    // IO Error Check
    if(!existingCheck.wasSuccessful())
        postIOError(MSG_LB_XML_UNEXPECTED_ERROR, existingCheck);

    // Return true on success
    return existingCheck.wasSuccessful();
}

bool MainWindow::parseFlashpointData()
{
    // Get and open connection to Flashpoint SQLite database, check that it is valid and allow for retries
    QSqlDatabase fpDB;

    // General error holder
    QSqlError errorCheck;

    // Check that the connection is/can be opened and allow for retries
    if(!mFlashpointInstall->databaseConnectionOpenInThisThread())
    {
        while((errorCheck = mFlashpointInstall->openThreadDatabaseConnection()).isValid())
            if(QMessageBox::critical(this, QApplication::applicationName(), MSG_FP_DB_CANT_CONNECT.arg(errorCheck.text()),
                                     QMessageBox::Retry | QMessageBox::Abort, QMessageBox::Retry) == QMessageBox::Abort)
                return false;
    }
    // Ensure the database contains the required tables
    QSet<QString> missingTables;
    errorCheck = mFlashpointInstall->checkDatabaseForRequiredTables(missingTables);

    // SQL Error Check
    if(errorCheck.isValid())
    {
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, errorCheck);
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
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, errorCheck);
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
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, errorCheck);

    // Return true on success
    return !errorCheck.isValid();

}

bool MainWindow::installsHaveChanged()
{
    // TODO: Make this check more thorough

    // Check LB existing items
    QSet<QString> currentPlatforms = mLaunchBoxInstall->getExistingPlatforms();
    QSet<QString> currentPlaylists = mLaunchBoxInstall->getExistingPlaylists();

    if(!mLaunchBoxInstall->populateExistingItems().wasSuccessful())
        return true;

    if(currentPlatforms != mLaunchBoxInstall->getExistingPlatforms() || currentPlaylists != mLaunchBoxInstall->getExistingPlaylists())
        return true;

    return false;
}

void MainWindow::redoInputChecks()
{
    // Get existing locations
    QString launchBoxPath = mLaunchBoxInstall->getPath();
    QString flashpointPath = mFlashpointInstall->getPath();

    // Clear existing installs
    mLaunchBoxInstall.reset();
    mFlashpointInstall.reset();

    // Check them again
    checkLaunchBoxInput(launchBoxPath);
    checkFlashpointInput(flashpointPath);
}

void MainWindow::postSqlError(QString mainText, QSqlError sqlError)
{
    QMessageBox sqlErrorMsg;
    sqlErrorMsg.setIcon(QMessageBox::Critical);
    sqlErrorMsg.setText(mainText);
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

void MainWindow::postIOError(QString mainText, Qx::IOOpReport report)
{
    QMessageBox ioErrorMsg;
    ioErrorMsg.setIcon(QMessageBox::Critical);
    ioErrorMsg.setText(mainText);
    ioErrorMsg.setInformativeText(report.getOutcome());
    ioErrorMsg.setStandardButtons(QMessageBox::Ok);

    ioErrorMsg.exec();
}

int MainWindow::postGenericError(Qx::GenericError error, QMessageBox::StandardButtons choices)
{
    // Prepare dialog
    QMessageBox genericErrorMessage;
    if(!error.getCaption().isEmpty())
        genericErrorMessage.setWindowTitle(error.getCaption());
    if(!error.getPrimaryInfo().isEmpty())
        genericErrorMessage.setText(error.getPrimaryInfo());
    if(!error.getSecondaryInfo().isEmpty())
        genericErrorMessage.setInformativeText(error.getSecondaryInfo());
    if(!error.getDetailedInfo().isEmpty())
        genericErrorMessage.setDetailedText(error.getDetailedInfo());
    genericErrorMessage.setStandardButtons(choices);
    genericErrorMessage.setIcon(QMessageBox::Critical);

    return genericErrorMessage.exec();
}

void MainWindow::importSelectionReaction(QListWidgetItem* item, QWidget* parent)
{
    // TODO: Some code here is leftover from when playlist selection could trigger this function. Possibly no longer needed
    if(item->checkState() == Qt::Checked)
    {
        ui->pushButton_startImport->setEnabled(true);
        ui->groupBox_updateMode->setEnabled(parent == ui->listWidget_platformChoices &&
                                            mLaunchBoxInstall->getExistingPlatforms().contains(LB::Install::XMLDoc::makeFileNameLBKosher(item->text())));
//        ui->groupBox_updateMode->setEnabled((parent == ui->listWidget_platformChoices && mLaunchBoxInstall->getExistingPlatforms().contains(item->text())) ||
//                                            (parent == ui->listWidget_playlistChoices && mLaunchBoxInstall->getExistingPlaylists().contains(item->text())));
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

                if(mLaunchBoxInstall->getExistingPlatforms().contains(LB::Install::XMLDoc::makeFileNameLBKosher(ui->listWidget_platformChoices->item(i)->text())))
                    keepUpdateGroupEnabled = true;
            }
        }

//        // Check playlist choices if needed
//        if(!keepUpdateGroupEnabled || !keepStartButtonEnabled)
//        {
//            for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
//            {
//                if(ui->listWidget_playlistChoices->item(i)->checkState() == Qt::Checked)
//                {
//                    keepStartButtonEnabled = true;

//                    if(mLaunchBoxInstall->getExistingPlaylists().contains(ui->listWidget_playlistChoices->item(i)->text()))
//                        keepUpdateGroupEnabled = true;
//                }
//            }
//        }

        // Apply state changes
        ui->groupBox_updateMode->setEnabled(keepUpdateGroupEnabled);
        ui->pushButton_startImport->setEnabled(keepStartButtonEnabled);
    }

}

QSet<QString> MainWindow::getSelectedPlatforms(bool fileNameLegal) const
{
    QSet<QString> selectedPlatforms;

    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
        if(ui->listWidget_platformChoices->item(i)->checkState() == Qt::Checked)
            selectedPlatforms.insert(fileNameLegal ? LB::Install::XMLDoc::makeFileNameLBKosher(ui->listWidget_platformChoices->item(i)->text()) :
                                                     ui->listWidget_platformChoices->item(i)->text());

    return selectedPlatforms;
}

QSet<QString> MainWindow::getSelectedPlaylists(bool fileNameLegal) const
{
    QSet<QString> selectedPlaylists;

    for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
        if(ui->listWidget_playlistChoices->item(i)->checkState() == Qt::Checked)
            selectedPlaylists.insert(fileNameLegal ? LB::Install::XMLDoc::makeFileNameLBKosher(ui->listWidget_playlistChoices->item(i)->text()) :
                                                     ui->listWidget_playlistChoices->item(i)->text());

    return selectedPlaylists;
}

LB::Install::GeneralOptions MainWindow::getSelectedGeneralOptions() const
{
    return {ui->action_includeExtreme->isChecked()};
}

LB::Install::UpdateOptions MainWindow::getSelectedUpdateOptions() const
{
    return {ui->radioButton_onlyAdd->isChecked() ? LB::Install::OnlyNew : LB::Install::NewAndExisting, ui->checkBox_removeMissing->isChecked() };
}

LB::Install::ImageMode MainWindow::getSelectedImageOption() const
{
    return ui->radioButton_launchBoxCopy->isChecked() ? LB::Install::LB_Copy : ui->radioButton_launchBoxLink->isChecked() ? LB::Install::LB_Link : LB::Install::FP_Link;
}

void MainWindow::prepareImport()
{
    // Check that install contents haven't been altered
    if(installsHaveChanged())
    {
        QMessageBox::warning(this, QApplication::applicationName(), MSG_INSTALL_CONTENTS_CHANGED);
        redoInputChecks();
        return;
    }

    // Warn user if they are changing existing files
    if(mLaunchBoxInstall->getExistingPlatforms().intersects(getSelectedPlatforms(true)) || mLaunchBoxInstall->getExistingPlaylists().intersects(getSelectedPlaylists(true)))
        if(QMessageBox::warning(this, QApplication::applicationName(), MSG_PRE_EXISTING_IMPORT, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel)
            return;

    // Warn user if Flashpoint is running
    // Check if Flashpoint is running
    if(Qx::processIsRunning(QFileInfo(FP::Install::MAIN_EXE_PATH).fileName()))
        QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_CLOSE_PROMPT);

    // Only allow proceeding if LB isn't running
    bool lbRunning;
    while((lbRunning = Qx::processIsRunning(LB::Install::MAIN_EXE_PATH)))
        if(QMessageBox::critical(this, QApplication::applicationName(), MSG_LB_CLOSE_PROMPT, QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
            break;

    if(!lbRunning)
    {
        // Create progress dialog, set initial busy state and show
        mImportProgressDialog = std::make_unique<QProgressDialog>(STEP_FP_DB_INITIAL_QUERY, "Cancel", 0, 10000, this); //Arbitrarily high maximum so initial percentage is 0
        mImportProgressDialog->setWindowTitle(CAPTION_IMPORTING);
        mImportProgressDialog->setWindowModality(Qt::WindowModal);
        mImportProgressDialog->setAutoReset(false);
        mImportProgressDialog->setAutoClose(false);
        mImportProgressDialog->setMinimumDuration(0); // Always show pd
        mImportProgressDialog->setValue(0); // Get pd to show
        QApplication::processEvents(); // Force show immediately

        // Setup import worker
        ImportWorker importWorker(mFlashpointInstall, mLaunchBoxInstall,
                                  {getSelectedPlatforms(), getSelectedPlaylists()},
                                  {getSelectedUpdateOptions(), getSelectedImageOption(), getSelectedGeneralOptions()});

        // Create process update connections
        connect(&importWorker, &ImportWorker::progressStepChanged, mImportProgressDialog.get(), &QProgressDialog::setLabelText);
        connect(&importWorker, &ImportWorker::progressMaximumChanged, mImportProgressDialog.get(), &QProgressDialog::setMaximum);
        connect(&importWorker, &ImportWorker::progressValueChanged, mImportProgressDialog.get(), &QProgressDialog::setValue);
        connect(mImportProgressDialog.get(), &QProgressDialog::canceled, &importWorker, &ImportWorker::notifyCanceled);

        // Create UI update timer reset connection
        connect(&importWorker, &ImportWorker::progressValueChanged, this, &MainWindow::resetUpdateTimer); // Reset refresh timer since setValue already processes events

        // Import error tracker
        Qx::GenericError importError;

        // Start UI update timer
        mUIUpdateWorkaroundTimer.start();

        // Start import and forward result to handler
        handleImportResult(importWorker.doImport(importError), importError);
    }
}

void MainWindow::revertAllLaunchBoxChanges()
{
    // Trackers
    bool tempSkip = false;
    bool alwaysSkip = false;
    QString currentError;
    int retryChoice;

    // Revert rrror Message
    QMessageBox revertError;
    revertError.setWindowTitle(CAPTION_REVERT_ERR);
    revertError.setInformativeText("Retry?");
    revertError.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);
    revertError.setDefaultButton(QMessageBox::Yes);

    // Progress
    QProgressDialog reversionProgress(CAPTION_REVERT, QString(), 0, mLaunchBoxInstall->getRevertQueueCount(), this);
    reversionProgress.setWindowModality(Qt::WindowModal);
    reversionProgress.setAutoReset(false);

    while(mLaunchBoxInstall->revertNextChange(currentError, alwaysSkip || tempSkip) != 0)
    {
        // Check for error
        if(currentError.isNull())
        {
            tempSkip = false;
            reversionProgress.setValue(reversionProgress.value() + 1);
        }
        else
        {
            revertError.setText(currentError);
            retryChoice = revertError.exec();

            if(retryChoice == QMessageBox::No)
                tempSkip = true;
            else if(retryChoice == QMessageBox::NoToAll)
                alwaysSkip = true;
        }        
    }

    // Ensure progress dialog is closed
    reversionProgress.close();

    // Reset instance
    mLaunchBoxInstall->softReset();
}

void MainWindow::standaloneCLIFpDeploy()
{
    // Browse for install
    QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE, QDir::currentPath());

    if(!selectedDir.isEmpty())
    {
        if(FP::Install::pathIsValidtInstall(selectedDir))
        {
            FP::Install tempFlashpointInstallinstallPath(selectedDir);

            if(!tempFlashpointInstallinstallPath.matchesTargetVersion())
                QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);

            // Deploy exe
            QString deployError;
            while(!tempFlashpointInstallinstallPath.deployCLIFp(deployError))
                if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP.arg(deployError), QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
                    break;
        }
        else
            QMessageBox::critical(this, QApplication::applicationName(), MSG_FP_INSTALL_INVALID);
    }
}

//-Slots---------------------------------------------------------------------------------------------------------
//Private:
void MainWindow::all_on_action_triggered()
{
    // Get the object that called this slot
    QAction* senderAction = qobject_cast<QAction *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderAction == nullptr)
        assert("Pointer conversion to action failed");

    // Determine sender and take corresponding action
    if(senderAction == ui->action_exit)
        QApplication::quit();
    else if(senderAction == ui->action_deployCLIFp)
        standaloneCLIFpDeploy();
    else if(senderAction == ui->action_goToCLIFpGitHub)
        QDesktopServices::openUrl(URL_CLIFP_GITHUB);
    else if(senderAction == ui->action_goToOFLIbGitHub)
        QDesktopServices::openUrl(URL_OFLIB_GITHUB);
    else if(senderAction == ui->action_goToLBForums)
        QDesktopServices::openUrl(URL_LB_FORUMS);
    else
        assert("Unhandled use of all_on_action_triggered() slot");
}

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
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_LAUNCHBOX_BROWSE,
                                                                (QFile::exists(ui->lineEdit_launchBoxPath->text()) ? ui->lineEdit_launchBoxPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_launchBoxPath->setText(QDir::toNativeSeparators(selectedDir));
            checkLaunchBoxInput(selectedDir);
        }
    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE,
                                                                (QFile::exists(ui->label_flashPointPath->text()) ? ui->label_flashPointPath->text() : QDir::currentPath()));

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
        QMessageBox::information(this, CAPTION_UPDATE_MODE_HELP, MSG_UPDATE_MODE_HELP.arg(ui->radioButton_onlyAdd->text(),
                                                                                          ui->radioButton_updateExisting->text(),
                                                                                          ui->checkBox_removeMissing->text(),
                                                                                          ui->action_includeExtreme->text()));
    else if(senderPushButton == ui->pushButton_imageModeHelp)
        QMessageBox::information(this, CAPTION_IMAGE_MODE_HELP, MSG_IMAGE_MODE_HELP.arg(ui->radioButton_launchBoxCopy->text(),
                                                                                        ui->radioButton_launchBoxLink->text(),
                                                                                        ui->radioButton_flashpointLink->text()));
    else if(senderPushButton == ui->pushButton_startImport)
        prepareImport();
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
                setInputStage(Paths);
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
                setInputStage(Paths);
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
//    else if(senderListWidget == ui->listWidget_playlistChoices && !mAlteringListWidget) TODO: Playlists currently only get games from selected platforms,
//        importSelectionReaction(item, ui->listWidget_playlistChoices);                        so triggering this here is no longer required. Possibly remove
    else
        assert("Unhandled use of all_on_listWidget_itemChanged() slot");
}

void MainWindow::resetUpdateTimer() { mUIUpdateWorkaroundTimer.start(); }
void MainWindow::updateUI() { QApplication::processEvents(); }

void MainWindow::handleBlockingError(std::shared_ptr<int> response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices)
{
    // Post error and get response
    int userChoice = postGenericError(blockingError, choices);

    // If applicable return selection
    if(response)
        *response = userChoice;
}

void MainWindow::handleImportResult(ImportWorker::ImportResult importResult, Qx::GenericError errorReport)
{
    // Close progress dialog
    mImportProgressDialog->close();

    // Stop UI update timer
    mUIUpdateWorkaroundTimer.stop();

    // Post error report if present
    if(errorReport.isValid())
        postGenericError(errorReport, QMessageBox::Ok);

    if(importResult == ImportWorker::Successful)
    {
        // Deploy CLIFp
        QString deployError;
        while(!mFlashpointInstall->deployCLIFp(deployError))
            if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP.arg(deployError), QMessageBox::Retry | QMessageBox::Ignore, QMessageBox::Retry) == QMessageBox::Ignore)
                break;

        // Post-import message
        QMessageBox::information(this, QApplication::applicationName(), MSG_POST_IMPORT);

        // Update selection lists to reflect newly existing platforms
        gatherInstallInfo();
    }
    else if(importResult == ImportWorker::Canceled)
    {
        QMessageBox::critical(this, CAPTION_REVERT, MSG_USER_CANCELED);
        revertAllLaunchBoxChanges();
    }
    else
    {
        // Show general next steps message
        QMessageBox::warning(this, CAPTION_REVERT, MSG_HAVE_TO_REVERT);
        revertAllLaunchBoxChanges();
    }
}
