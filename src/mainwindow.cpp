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
#include <QShowEvent>
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

    // Get built-in CLIFp version
    QTemporaryDir tempDir;
    if(tempDir.isValid())
    {
        // Create local copy of internal CLIFp.exe since internal path cannot be used with WinAPI
        QString localCopyPath = tempDir.path() + '/' + FP::Install::CLIFp::EXE_NAME;
        if(QFile::copy(":/res/file/" + FP::Install::CLIFp::EXE_NAME, localCopyPath))
            mInternalCLIFpVersion = Qx::getFileDetails(localCopyPath).getFileVersion();
    }

    // Abort if no version could be determined
    if(mInternalCLIFpVersion.isNull())
    {
        QMessageBox::critical(this, CAPTION_GENERAL_FATAL_ERROR, MSG_FATAL_NO_INTERNAL_CLIFP_VER);
        QApplication::exit(1);
    }

    // General setup
    ui->setupUi(this);
    QApplication::setApplicationName(VER_PRODUCTNAME_STR);
    mHasLinkPermissions = testForLinkPermissions();
    setWindowTitle(VER_PRODUCTNAME_STR);
    initializeWidgetEnableConditionMap();
    initializeForms();

    // Setup UI update workaround timer
    //mUIUpdateWorkaroundTimer.setInterval(IMPORT_UI_UPD_INTERVAL);
    //connect(&mUIUpdateWorkaroundTimer, &QTimer::timeout, this, &MainWindow::updateUI); // Process events at minimum rate

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

    // Add CLIFp version to deploy option
    ui->action_deployCLIFp->setText(ui->action_deployCLIFp->text() +  " " + mInternalCLIFpVersion.toString(Qx::MMRB::StringFormat::NoTrailRBZero));

    // Prepare help messages
    mArgedPlaylistGameModeHelp = MSG_PLAYLIST_GAME_MODE_HELP.arg(ui->radioButton_selectedPlatformsOnly->text(),
                                                                 ui->radioButton_forceAll->text());

    mArgedUpdateModeHelp = MSG_UPDATE_MODE_HELP.arg(ui->radioButton_onlyAdd->text(),
                                                    ui->radioButton_updateExisting->text(),
                                                    ui->checkBox_removeMissing->text(),
                                                    ui->action_includeExtreme->text());

    mArgedImageModeHelp = MSG_IMAGE_MODE_HELP.arg(ui->radioButton_copy->text(),
                                                   ui->radioButton_reference->text(),
                                                   ui->radioButton_link->text());

    // Setup main forms
    ui->radioButton_link->setEnabled(mHasLinkPermissions);
    ui->radioButton_link->setChecked(mHasLinkPermissions);
    if(!mHasLinkPermissions)
        ui->radioButton_link->setText(ui->radioButton_link->text().append(REQUIRE_ELEV));
    ui->radioButton_reference->setChecked(!mHasLinkPermissions);
    refreshWidgetEnableStates();

    // TODO: THIS IS FOR DEBUG PURPOSES
    //checkLaunchBoxInput("C:/Users/Player/Desktop/LBTest/LaunchBox");
    //checkFlashpointInput("D:/FP/Flashpoint 8.1 Ultimate");
}

void MainWindow::initializeWidgetEnableConditionMap()
{
    // Populate hashmap of ui element enable conditions
    mWidgetEnableConditionMap[ui->groupBox_importSelection] = [&](){ return mLaunchBoxInstall && mFlashpointInstall; };
    mWidgetEnableConditionMap[ui->groupBox_playlistGameMode] = [&](){ return getSelectedPlaylists().count() > 0; };
    mWidgetEnableConditionMap[ui->groupBox_updateMode] = [&](){ return isExistingPlatformSelected() ||
                                                                       isExistingPlaylistSelected() ||
                                                                       getSelectedPlaylistGameMode() ==  LB::Install::ForceAll;};
    mWidgetEnableConditionMap[ui->groupBox_imageMode] = [&](){ return mLaunchBoxInstall && mFlashpointInstall; };
    mWidgetEnableConditionMap[ui->pushButton_startImport] = [&](){ return getSelectedPlatforms().count() > 0 ||
                                                                          (getSelectedPlaylistGameMode() == LB::Install::ForceAll && getSelectedPlaylists().count() > 0); };
}

void MainWindow::checkManualInstallInput(Install install)
{
    QLineEdit* pathSource;
    QLabel* installStatusIcon;

    switch(install)
    {
        case Install::LaunchBox:
            mLaunchBoxInstall.reset(); // Detach from previous install if present
            pathSource = ui->lineEdit_launchBoxPath;
            installStatusIcon = ui->icon_launchBox_install_status;
            break;

        case Install::Flashpoint:
            mFlashpointInstall.reset(); // Detach from previous install if present
            pathSource = ui->lineEdit_flashpointPath;
            installStatusIcon = ui->icon_flashpoint_install_status;
            break;
    }

    QDir selectedDir = QDir::cleanPath(QDir::fromNativeSeparators(pathSource->text()));
    if(!pathSource->text().isEmpty() && selectedDir.exists())
        validateInstall(selectedDir.absolutePath(), install);
    else
    {
        installStatusIcon->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
        clearListWidgets();
        refreshWidgetEnableStates();
    }
}

void MainWindow::validateInstall(QString installPath, Install install)
{
    switch(install)
    {
        case Install::LaunchBox:
            mLaunchBoxInstall.reset(); // Detach from previous install if present
            if(LB::Install::pathIsValidInstall(installPath))
            {
                mLaunchBoxInstall = std::make_shared<LB::Install>(installPath);
                ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Valid_Install.png"));
            }
            else
            {
                ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
                clearListWidgets();
                refreshWidgetEnableStates();
                QMessageBox::critical(this, QApplication::applicationName(), MSG_LB_INSTALL_INVALID);
            }
            break;

        case Install::Flashpoint:
            mFlashpointInstall.reset(); // Detach from previous install if present
            FP::Install::ValidityReport fpValidity = FP::Install::checkInstallValidity(installPath, FP::Install::CompatLevel::Full);
            if(fpValidity.installValid)
            {
                mFlashpointInstall = std::make_shared<FP::Install>(installPath);

                if(mFlashpointInstall->matchesTargetVersion())
                    ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Valid_Install.png"));
                else
                {
                    ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Mismatch_Install.png"));
                    QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);
                }
            }
            else
            {
                ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
                clearListWidgets();
                refreshWidgetEnableStates();
                postGenericError(Qx::GenericError(Qx::GenericError::Critical, MSG_FP_INSTALL_INVALID, fpValidity.details), QMessageBox::Ok);
            }
            break;
    }

    if(mLaunchBoxInstall && mFlashpointInstall)
        gatherInstallInfo();
}

void MainWindow::gatherInstallInfo()
{
    // Get data in order but only continue if each step is successful
    if(parseFlashpointData())
    {
        if(parseLaunchBoxData())
        {
            // Show selection options
            populateImportSelectionBoxes();

            // Advance to next input stage
            refreshWidgetEnableStates();
        }
        else
        {
            mLaunchBoxInstall.reset();
            ui->icon_launchBox_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
            clearListWidgets();
            refreshWidgetEnableStates();
        }
    }
    else
    {
        mFlashpointInstall.reset();
        ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/res/icon/Invalid_Install.png"));
        clearListWidgets();
        refreshWidgetEnableStates();
    }
}

void MainWindow::populateImportSelectionBoxes()
{
    // Populate import selection boxes
    clearListWidgets();
    ui->listWidget_platformChoices->addItems(mFlashpointInstall->getPlatformList());
    ui->listWidget_playlistChoices->addItems(mFlashpointInstall->getPlaylistList());

    // Set item attributes
    QListWidgetItem* currentItem;

    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
    {
        currentItem = ui->listWidget_platformChoices->item(i);
        currentItem->setFlags(currentItem->flags() | Qt::ItemIsUserCheckable);
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

    // Disable update mode box and import start button since no items will be selected after this operation
    ui->groupBox_updateMode->setEnabled(false);
    ui->pushButton_startImport->setEnabled(false);
}

bool MainWindow::parseLaunchBoxData()
{
    // IO Error check instance
    Qx::IOOpReport existingCheck;

    // Get list of existing platforms and playlists
    existingCheck = mLaunchBoxInstall->populateExistingDocs(mFlashpointInstall->getPlatformList(), mFlashpointInstall->getPlaylistList());

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

    if(!mLaunchBoxInstall->populateExistingDocs(mFlashpointInstall->getPlatformList(), mFlashpointInstall->getPlaylistList()).wasSuccessful())
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

    // Check them again
    validateInstall(launchBoxPath, Install::LaunchBox);
    validateInstall(flashpointPath, Install::Flashpoint);
}

void MainWindow::clearListWidgets()
{
    ui->listWidget_platformChoices->clear();
    ui->listWidget_playlistChoices->clear();
    mPlatformItemCheckStates.clear();
    mPlaylistItemCheckStates.clear();
}

bool MainWindow::isExistingPlatformSelected()
{
    // Check platform choices
    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
    {
        if(ui->listWidget_platformChoices->item(i)->checkState() == Qt::Checked &&
           mLaunchBoxInstall->getExistingPlatforms().contains(ui->listWidget_platformChoices->item(i)->text()))
            return true;
    }

    // Return false if no match
    return false;
}

bool MainWindow::isExistingPlaylistSelected()
{
    // Check platform choices
    for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
    {
        if(ui->listWidget_playlistChoices->item(i)->checkState() == Qt::Checked &&
           mLaunchBoxInstall->getExistingPlaylists().contains(ui->listWidget_playlistChoices->item(i)->text()))
            return true;
    }

    // Return false if no match
    return false;
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
    if(!error.caption().isEmpty())
        genericErrorMessage.setWindowTitle(error.caption());
    if(!error.primaryInfo().isEmpty())
        genericErrorMessage.setText(error.primaryInfo());
    if(!error.secondaryInfo().isEmpty())
        genericErrorMessage.setInformativeText(error.secondaryInfo());
    if(!error.detailedInfo().isEmpty())
        genericErrorMessage.setDetailedText(error.detailedInfo());
    genericErrorMessage.setStandardButtons(choices);
    genericErrorMessage.setIcon(error.errorLevel() == Qx::GenericError::Warning ? QMessageBox::Warning : QMessageBox::Critical);

    return genericErrorMessage.exec();
}

void MainWindow::refreshWidgetEnableStates()
{
    QHash<QWidget*, std::function<bool(void)>>::const_iterator i;
    for(i = mWidgetEnableConditionMap.begin(); i != mWidgetEnableConditionMap.end(); i++)
        i.key()->setEnabled(i.value()());
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

FP::Install::InclusionOptions MainWindow::getSelectedInclusionOptions() const
{
    return {ui->action_includeExtreme->isChecked(), ui->action_includeAnimations->isChecked()};
}

LB::UpdateOptions MainWindow::getSelectedUpdateOptions() const
{
    return {ui->radioButton_onlyAdd->isChecked() ? LB::OnlyNew : LB::NewAndExisting, ui->checkBox_removeMissing->isChecked() };
}

LB::Install::ImageMode MainWindow::getSelectedImageMode() const
{
    return ui->radioButton_copy->isChecked() ? LB::Install::Copy : ui->radioButton_reference->isChecked() ? LB::Install::Reference : LB::Install::Link;
}

LB::Install::PlaylistGameMode MainWindow::getSelectedPlaylistGameMode() const
{
    return ui->radioButton_selectedPlatformsOnly->isChecked() ? LB::Install::SelectedPlatform : LB::Install::ForceAll;
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
    QStringList selPlatforms = getSelectedPlatforms();
    QStringList selPlaylists = getSelectedPlaylists();

    if(mLaunchBoxInstall->getExistingPlatforms().intersects(QSet<QString>(selPlatforms.begin(), selPlatforms.end())) ||
       mLaunchBoxInstall->getExistingPlaylists().intersects(QSet<QString>(selPlaylists.begin(), selPlaylists.end())) ||
       (getSelectedPlaylistGameMode() == LB::Install::ForceAll && mLaunchBoxInstall->getExistingPlatforms().count() > 0))
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
        // Create progress dialog, set initial state and show
        mImportProgressDialog = std::make_unique<QProgressDialog>(STEP_FP_DB_INITIAL_QUERY, "Cancel", 0, 10000, this); // Arbitrarily high maximum so initial percentage is 0
        mImportProgressDialog->setWindowTitle(CAPTION_IMPORTING);
        mImportProgressDialog->setWindowModality(Qt::WindowModal);
        mImportProgressDialog->setAutoReset(false);
        mImportProgressDialog->setAutoClose(false);
        mImportProgressDialog->setMinimumDuration(0); // Always show pd
        mImportProgressDialog->setValue(0); // Get pd to show

        // Get taskbar progress indicator and set it up  TODO: Remove for Qt6
        QWinTaskbarProgress* tbProgress = mWindowTaskbarButton->progress();
        tbProgress->setMinimum(0);
        tbProgress->setMaximum(10000); // Arbitrarily high maximum so initial percentage is 0
        tbProgress->setValue(0);
        tbProgress->resume(); // Ensure possible previous errors are cleared
        tbProgress->setVisible(true);

        // Force show progress immediately
        QApplication::processEvents();

        // Setup import worker
        ImportWorker importWorker(mFlashpointInstall, mLaunchBoxInstall,
                                  {selPlatforms, selPlaylists},
                                  {getSelectedUpdateOptions(), getSelectedImageMode(), getSelectedPlaylistGameMode(), getSelectedInclusionOptions()});

        // Setup blocking error connection
        connect(&importWorker, &ImportWorker::blockingErrorOccured, this, &MainWindow::handleBlockingError);

        // Create process update connections
        connect(&importWorker, &ImportWorker::progressStepChanged, mImportProgressDialog.get(), &QProgressDialog::setLabelText);
        connect(&importWorker, &ImportWorker::progressMaximumChanged, mImportProgressDialog.get(), &QProgressDialog::setMaximum);
        connect(&importWorker, &ImportWorker::progressMaximumChanged, tbProgress, &QWinTaskbarProgress::setMaximum);
        connect(&importWorker, &ImportWorker::progressValueChanged, mImportProgressDialog.get(), &QProgressDialog::setValue);
        connect(&importWorker, &ImportWorker::progressValueChanged, tbProgress, &QWinTaskbarProgress::setValue);
        connect(mImportProgressDialog.get(), &QProgressDialog::canceled, &importWorker, &ImportWorker::notifyCanceled);

        // Create UI update timer reset connection
        //connect(&importWorker, &ImportWorker::progressValueChanged, this, &MainWindow::resetUpdateTimer); // Reset refresh timer since setValue already processes events

        // Import error tracker
        Qx::GenericError importError;

        // Start UI update timer
        //mUIUpdateWorkaroundTimer.start();

        // Start import and forward result to handler
        ImportWorker::ImportResult importResult = importWorker.doImport(importError);
        handleImportResult(importResult, importError);
    }
}

void MainWindow::revertAllLaunchBoxChanges()
{
    // Trackers
    bool tempSkip = false;
    bool alwaysSkip = false;
    QString currentError;
    int retryChoice;

    // Revert error Message
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
        FP::Install::ValidityReport fpValidity = FP::Install::checkInstallValidity(selectedDir, FP::Install::CompatLevel::Full);
        if(fpValidity.installValid)
        {
            FP::Install tempFlashpointInstall(selectedDir);

            if(!tempFlashpointInstall.matchesTargetVersion())
                QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);

            bool willDeploy = true;

            // Check for existing CLIFp
            if(tempFlashpointInstall.hasCLIFp())
            {
                // Notify user if this will be a downgrade
                if(mInternalCLIFpVersion < tempFlashpointInstall.currentCLIFpVersion())
                    willDeploy = (QMessageBox::warning(this, CAPTION_CLIFP_DOWNGRADE, MSG_FP_CLFIP_WILL_DOWNGRADE, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==  QMessageBox::Yes);
            }

            // Deploy CLIFp if applicable
            if(willDeploy)
            {
                // Deploy exe
                QString deployError;
                while(!tempFlashpointInstall.deployCLIFp(deployError))
                    if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP.arg(deployError), QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
                        break;
            }
        }
        else
            postGenericError(Qx::GenericError(Qx::GenericError::Critical, MSG_FP_INSTALL_INVALID, fpValidity.details), QMessageBox::Ok);
    }
}

//Protected:
void MainWindow::showEvent(QShowEvent* event)
{
    // Call standard function
    QMainWindow::showEvent(event);

    // Configure taskbar button TODO: Remove for Qt6
    mWindowTaskbarButton = new QWinTaskbarButton(this);
    mWindowTaskbarButton->setWindow(this->windowHandle());
}

//-Slots---------------------------------------------------------------------------------------------------------
//Private:
void MainWindow::all_on_action_triggered()
{
    // Get the object that called this slot
    QAction* senderAction = qobject_cast<QAction *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderAction == nullptr)
        throw std::runtime_error("Pointer conversion to action failed");

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
        throw std::runtime_error("Unhandled use of all_on_action_triggered() slot");
}

void MainWindow::all_on_pushButton_clicked()
{
    // Get the object that called this slot
    QPushButton* senderPushButton = qobject_cast<QPushButton *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderPushButton == nullptr)
        throw std::runtime_error("Pointer conversion to push button failed");

    // Determine sender and take corresponding action
    if(senderPushButton == ui->pushButton_launchBoxBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_LAUNCHBOX_BROWSE,
                                                                (QFileInfo::exists(ui->lineEdit_launchBoxPath->text()) ? ui->lineEdit_launchBoxPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_launchBoxPath->setText(QDir::toNativeSeparators(selectedDir));
            validateInstall(selectedDir, Install::LaunchBox);
        }
    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE,
                                                                (QFileInfo::exists(ui->lineEdit_flashpointPath->text()) ? ui->lineEdit_flashpointPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_flashpointPath->setText(QDir::toNativeSeparators(selectedDir));
            validateInstall(selectedDir, Install::Flashpoint);
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
    else if(senderPushButton == ui->pushButton_playlistGameModeHelp)
        QMessageBox::information(this, CAPTION_PLAYLIST_GAME_MODE_HELP, mArgedPlaylistGameModeHelp);
    else if(senderPushButton == ui->pushButton_updateModeHelp)
        QMessageBox::information(this, CAPTION_UPDATE_MODE_HELP, mArgedUpdateModeHelp);
    else if(senderPushButton == ui->pushButton_imageModeHelp)
        QMessageBox::information(this, CAPTION_IMAGE_MODE_HELP, mArgedImageModeHelp);
    else if(senderPushButton == ui->pushButton_startImport)
        prepareImport();
    else if(senderPushButton == ui->pushButton_exit)
        QApplication::quit();
    else
        throw std::runtime_error("Unhandled use of all_on_pushButton_clicked() slot");
}

void MainWindow::all_on_lineEdit_editingFinished()
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit*>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        throw std::runtime_error("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
    {
        if(!mLineEdit_launchBoxPath_blocker)
            checkManualInstallInput(Install::LaunchBox);
        else
            mLineEdit_launchBoxPath_blocker--;
    }
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
    {
        if(!mLineEdit_flashpointPath_blocker)
           checkManualInstallInput(Install::Flashpoint);
        else
            mLineEdit_flashpointPath_blocker--;
    }
    else
        throw std::runtime_error("Unhandled use of all_on_linedEdit_textEdited() slot");
}

void MainWindow::all_on_lineEdit_textEdited() // Required due to an oversight with QLineEdit::editingFinished()
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        throw std::runtime_error("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
        mLineEdit_launchBoxPath_blocker = 0;
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
        mLineEdit_flashpointPath_blocker = 0;
    else
        throw std::runtime_error("Unhandled use of all_on_linedEdit_textEdited() slot");
}

void MainWindow::all_on_lineEdit_returnPressed() // Required due to an oversight with QLineEdit::editingFinished()
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit *>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        throw std::runtime_error("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launchBoxPath)
    {
        mLineEdit_launchBoxPath_blocker = 2;
        checkManualInstallInput(Install::LaunchBox);
    }
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
    {
        mLineEdit_flashpointPath_blocker = 2;
        checkManualInstallInput(Install::Flashpoint);
    }
    else
        throw std::runtime_error("Unhandled use of all_on_linedEdit_returnPressed() slot");
}

void MainWindow::all_on_listWidget_itemChanged(QListWidgetItem* item) // Proxy for "onItemChecked"
{
    // Get the object that called this slot
    QListWidget* senderListWidget = qobject_cast<QListWidget*>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderListWidget == nullptr)
        throw std::runtime_error("Pointer conversion to list widget failed");

    if(senderListWidget == ui->listWidget_platformChoices)
    {
        // Check if change was change in check state
        if(mPlatformItemCheckStates.contains(item) && item->checkState() != mPlatformItemCheckStates.value(item))
            refreshWidgetEnableStates();

        // Add/update check state
        mPlatformItemCheckStates[item] = item->checkState();
    }
    else if(senderListWidget == ui->listWidget_playlistChoices)
    {
        // Check if change was change in check state
        if(mPlaylistItemCheckStates.contains(item) && item->checkState() != mPlaylistItemCheckStates.value(item))
            refreshWidgetEnableStates();

        // Add/update check state
        mPlaylistItemCheckStates[item] = item->checkState();
    }
    else
        throw std::runtime_error("Unhandled use of all_on_listWidget_itemChanged() slot");
}

//void MainWindow::resetUpdateTimer() { mUIUpdateWorkaroundTimer.start(); }
//void MainWindow::updateUI() { QApplication::processEvents(); }

void MainWindow::all_on_radioButton_clicked()
{
    // Get the object that called this slot
    QRadioButton* senderRadioButton = qobject_cast<QRadioButton*>(sender());

    // Ensure the signal that trigged this slot belongs to the above class by checking for null pointer
    if(senderRadioButton == nullptr)
        throw std::runtime_error("Pointer conversion to radio button failed");

    if(senderRadioButton == ui->radioButton_selectedPlatformsOnly)
        refreshWidgetEnableStates();
    else if(senderRadioButton == ui->radioButton_forceAll)
        refreshWidgetEnableStates();
    else
        throw std::runtime_error("Unhandled use of all_on_radioButton_clicked() slot");
}

void MainWindow::handleBlockingError(std::shared_ptr<int> response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices)
{
    // Get taskbar progress and indicate error TODO: Remove for Qt6
    QWinTaskbarProgress* tbProgress = mWindowTaskbarButton->progress();
    tbProgress->stop();

    // Post error and get response
    int userChoice = postGenericError(blockingError, choices);

    // Clear taskbar error
    tbProgress->resume();

    // If applicable return selection
    if(response)
        *response = userChoice;
}

void MainWindow::handleImportResult(ImportWorker::ImportResult importResult, Qx::GenericError errorReport)
{
    // Close progress dialog and reset taskbar progress indicator
    mImportProgressDialog->close();
    mWindowTaskbarButton->progress()->reset();
    mWindowTaskbarButton->progress()->setVisible(false);

    // Stop UI update timer
    //mUIUpdateWorkaroundTimer.stop();

    // Post error report if present
    if(errorReport.isValid())
        postGenericError(errorReport, QMessageBox::Ok);

    if(importResult == ImportWorker::Successful)
    {
        bool willDeploy = true;

        // Check for existing CLIFp
        if(mFlashpointInstall->hasCLIFp())
        {
            // Notify user if this will be a downgrade
            if(mInternalCLIFpVersion < mFlashpointInstall->currentCLIFpVersion())
                willDeploy = (QMessageBox::warning(this, CAPTION_CLIFP_DOWNGRADE, MSG_FP_CLFIP_WILL_DOWNGRADE, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==  QMessageBox::Yes);
        }

        // Deploy CLIFp if applicable
        if(willDeploy)
        {
            QString deployError;
            while(!mFlashpointInstall->deployCLIFp(deployError))
                if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP.arg(deployError), QMessageBox::Retry | QMessageBox::Ignore, QMessageBox::Retry) == QMessageBox::Ignore)
                    break;
        }

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
