// Standard Library Includes
#include <assert.h>
#include <filesystem>

// Qt Includes
#include <QSet>
#include <QFile>
#include <QFileDialog>
#include <QtXml>
#include <QFileInfo>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QShowEvent>

// Qx Includes
#include <qx/gui/qx-color.h>
#include <qx/widgets/qx-common-widgets.h>
#include <qx/widgets/qx-treeinputdialog.h>
#include <qx/widgets/qx-logindialog.h>
#include <qx/windows/qx-filedetails.h>
#include <qx/core/qx-system.h>

// Project Includes
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "project_vars.h"
#include "clifp.h"

/* TODO: Consider having this tool deploy a .ini file (or the like) into the target launcher install
 * (with the exact location probably being guided by the specific Install child) that saves the settings
 * used for the import, so that they can be loaded again when that install is targeted by future versions
 * of the tool. Would have to account for an initial import vs update (likely just leaving the update settings
 * blank). Wouldn't be a huge difference but could be a nice little time saver.
 */

//===============================================================================================================
// MAIN WINDOW
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    /*Register metatypes
     * NOTE: Qt docs note these should be needed, as always, but since Qt6 signals/slots with these types seem to
     * work fine without the following calls.
     * See https://forum.qt.io/topic/136627/undocumented-automatic-metatype-registration-in-qt6
     */
    //qRegisterMetaType<ImportWorker::ImportResult>();
    //qRegisterMetaType<Qx::GenericError>();
    //qRegisterMetaType<std::shared_ptr<int>>();

    // Get built-in CLIFp version
    QTemporaryDir tempDir;
    if(tempDir.isValid())
    {
        // Create local copy of internal CLIFp.exe since internal path cannot be used with WinAPI
        QString localCopyPath = tempDir.path() + '/' + CLIFp::EXE_NAME;
        if(QFile::copy(":/file/" + CLIFp::EXE_NAME, localCopyPath))
            mInternalCLIFpVersion = Qx::FileDetails::readFileDetails(localCopyPath).fileVersion();
    }

    // Abort if no version could be determined
    if(mInternalCLIFpVersion.isNull())
    {
        QMessageBox::critical(this, CAPTION_GENERAL_FATAL_ERROR, MSG_FATAL_NO_INTERNAL_CLIFP_VER);
        mInitCompleted = false;
        return;
    }

    // General setup
    ui->setupUi(this);
    QApplication::setApplicationName(PROJECT_FULL_NAME);
    mHasLinkPermissions = testForLinkPermissions();
    setWindowTitle(PROJECT_FULL_NAME);
    initializeEnableConditionMaps();
    initializeForms();
    initializeFrontendHelpActions();

    // Check if Flashpoint is running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_NAME))
        QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_CLOSE_PROMPT);

    mInitCompleted = true;
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
    ui->action_deployCLIFp->setText(ui->action_deployCLIFp->text() +  " " + mInternalCLIFpVersion.normalized(2).toString());

    // Prepare help messages
    mArgedPlaylistGameModeHelp = MSG_PLAYLIST_GAME_MODE_HELP.arg(ui->radioButton_selectedPlatformsOnly->text(),
                                                                 ui->radioButton_forceAll->text());

    mArgedUpdateModeHelp = MSG_UPDATE_MODE_HELP.arg(ui->radioButton_onlyAdd->text(),
                                                    ui->radioButton_updateExisting->text(),
                                                    ui->checkBox_removeMissing->text());

    mArgedImageModeHelp = MSG_IMAGE_MODE_HELP.arg(ui->radioButton_copy->text(),
                                                   ui->radioButton_reference->text(),
                                                   ui->radioButton_link->text());

    // Setup main forms
    ui->label_flashpointVersion->clear();
    ui->label_frontendVersion->clear();

    // If no link permissions, inform user
    if(!mHasLinkPermissions)
        ui->radioButton_link->setText(ui->radioButton_link->text().append(REQUIRE_ELEV));

    // Perform standard widget updates
    refreshEnableStates();
    refreshCheckStates();

    // NOTE: THIS IS FOR DEBUG PURPOSES
    //checkLaunchBoxInput("C:/Users/Player/Desktop/LBTest/LaunchBox");
    //checkFlashpointInput("D:/FP/Flashpoint 8.1 Ultimate");
}

void MainWindow::initializeEnableConditionMaps()
{
    /* TODO: When Qt6 ports built-in widgets to use the C++ bindable properties system, it
     * would be great to convert this approach to using those instead, though this gets
     * tricky when checking for things that aren't easy to make a QProperty, such as when
     * checking for if the Install pointers are assigned
     */

    // Populate hash-map of widget element enable conditions
    mWidgetEnableConditionMap[ui->groupBox_importSelection] = [&](){ return mFrontendInstall && mFlashpointInstall; };
    mWidgetEnableConditionMap[ui->groupBox_playlistGameMode] = [&](){ return getSelectedPlaylists().count() > 0; };
    mWidgetEnableConditionMap[ui->groupBox_updateMode] = [&](){ return selectionsMayModify(); };
    mWidgetEnableConditionMap[ui->groupBox_imageMode] = [&](){ return mFrontendInstall && mFlashpointInstall; };

    mWidgetEnableConditionMap[ui->radioButton_reference] = [&](){
        return mFrontendInstall && mFlashpointInstall && mFrontendInstall->supportsImageMode(Fe::ImageMode::Reference);
    };
    mWidgetEnableConditionMap[ui->radioButton_link] = [&](){
        return mHasLinkPermissions && mFrontendInstall && mFlashpointInstall && mFrontendInstall->supportsImageMode(Fe::ImageMode::Link);
    };
    mWidgetEnableConditionMap[ui->radioButton_copy] = [&](){
        return mFrontendInstall && mFlashpointInstall && mFrontendInstall->supportsImageMode(Fe::ImageMode::Copy);
    };

    mWidgetEnableConditionMap[ui->pushButton_startImport] = [&](){ return getSelectedPlatforms().count() > 0 ||
                                                                          (getSelectedPlaylistGameMode() == ImportWorker::ForceAll && getSelectedPlaylists().count() > 0); };

    // Populate hash-map of action element enable conditions
    mActionEnableConditionMap[ui->action_forceDownloadImages] = [&](){ return mFlashpointInstall && mFlashpointInstall->preferences().onDemandImages; };
    mActionEnableConditionMap[ui->action_editTagFilter] = [&](){ return mFrontendInstall && mFlashpointInstall; };
}

void MainWindow::initializeFrontendHelpActions()
{
    // Add install help link for each registered install
    auto i = Fe::Install::registry().cbegin();
    auto end = Fe::Install::registry().cend();

    for(; i != end; i++)
    {
        QAction* feHelpAction = new QAction(ui->menu_frontendHelp);
        feHelpAction->setObjectName(MENU_FE_HELP_OBJ_NAME_TEMPLATE.arg(i.key()));
        feHelpAction->setText(i.key());
        feHelpAction->setIcon(QIcon(*(i->iconPath)));
        ui->menu_frontendHelp->addAction(feHelpAction);
    }
}

bool MainWindow::installMatchesTargetSeries(const Fp::Install& fpInstall)
{
    Qx::VersionNumber fpVersion = fpInstall.version();
    return TARGET_FP_VERSION_PREFIX.isPrefixOf(fpVersion);
}

void MainWindow::checkManualInstallInput(InstallType install)
{
    QLineEdit* pathSource = install == InstallType::Frontend ?
                            ui->lineEdit_frontendPath :
                            ui->lineEdit_flashpointPath;

    QDir selectedDir = QDir::cleanPath(QDir::fromNativeSeparators(pathSource->text()));
    if(!pathSource->text().isEmpty() && selectedDir.exists())
        validateInstall(selectedDir.absolutePath(), install);
    else
        invalidateInstall(install, false);
}

void MainWindow::validateInstall(QString installPath, InstallType install)
{
    switch(install)
    {
        case InstallType::Frontend:
            mFrontendInstall = Fe::Install::acquireMatch(installPath);
            if(mFrontendInstall)
            {
                ui->icon_frontend_install_status->setPixmap(QPixmap(":/ui/Valid_Install.png"));
                ui->label_frontendVersion->setText(mFrontendInstall->name() + " " + mFrontendInstall->versionString());
            }
            else
                invalidateInstall(install, true);
            break;

        case InstallType::Flashpoint:
            mFlashpointInstall = std::make_shared<Fp::Install>(installPath);
            if(mFlashpointInstall->isValid())
            {
                ui->label_flashpointVersion->setText(mFlashpointInstall->nameVersionString());
                if(installMatchesTargetSeries(*mFlashpointInstall))
                    ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/ui/Valid_Install.png"));
                else
                {
                    ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/ui/Mismatch_Install.png"));
                    QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);
                }
            }
            else
                invalidateInstall(install, true);
            break;
    }

    refreshEnableStates();

    if(mFrontendInstall && mFlashpointInstall)
        gatherInstallInfo();
}

void MainWindow::gatherInstallInfo()
{
    // Get data in order but only continue if each step is successful
    if(parseFrontendData())
    {
        // Show selection options
        populateImportSelectionBoxes();

        // Generate tab selection model
        generateTagSelectionOptions();

        // Ensure valid image mode
        refreshCheckStates();

        // Advance to next input stage
        refreshEnableStates();
    }
    else
        invalidateInstall(InstallType::Frontend, false);
}

void MainWindow::populateImportSelectionBoxes()
{
    // Populate import selection boxes
    clearListWidgets();
    ui->listWidget_platformChoices->addItems(mFlashpointInstall->database()->platformList());
    ui->listWidget_playlistChoices->addItems(mFlashpointInstall->database()->playlistList());

    // Set item attributes
    QListWidgetItem* currentItem;

    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
    {
        currentItem = ui->listWidget_platformChoices->item(i);
        currentItem->setFlags(currentItem->flags() | Qt::ItemIsUserCheckable);
        currentItem->setCheckState(Qt::Unchecked);

        if(mFrontendInstall->containsPlatform(currentItem->text()))
            currentItem->setBackground(QBrush(mExistingItemColor));
    }

    for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
    {
        currentItem = ui->listWidget_playlistChoices->item(i);
        currentItem->setFlags(currentItem->flags() | Qt::ItemIsUserCheckable);
        currentItem->setCheckState(Qt::Unchecked);

        if(mFrontendInstall->containsPlaylist(currentItem->text()))
            currentItem->setBackground(QBrush(mExistingItemColor));
    }
}

void MainWindow::generateTagSelectionOptions()
{
    // Ensure old options are dropped
    mTagSelectionModel.reset();

    // Get tag hierarchy
    QMap<int, Fp::Db::TagCategory> tagMap = mFlashpointInstall->database()->tags();

    // Create new model
    mTagSelectionModel = std::make_unique<Qx::StandardItemModel>();
    mTagSelectionModel->setAutoTristate(true);
    mTagSelectionModel->setSortRole(Qt::DisplayRole);

    // Populate model
    QStandardItem* modelRoot = mTagSelectionModel->invisibleRootItem();
    QMap<int, Fp::Db::TagCategory>::const_iterator i;

    // Add root tag categories
    for(i = tagMap.constBegin(); i != tagMap.constEnd(); ++i)
    {
        QStandardItem* rootItem = new QStandardItem(QString(i->name));
        rootItem->setData(QBrush(i->color), Qt::BackgroundRole);
        rootItem->setData(QBrush(Qx::Color::textFromBackground(i->color)), Qt::ForegroundRole);
        rootItem->setCheckState(Qt::CheckState::Checked);
        rootItem->setCheckable(true);
        QMap<int, Fp::Db::Tag>::const_iterator j;

        // Add child tags
        for(j = i->tags.constBegin(); j != i->tags.constEnd(); ++j)
        {
            QStandardItem* childItem = new QStandardItem(QString(j->primaryAlias));
            childItem->setData(j->id, USER_ROLE_TAG_ID);
            childItem->setCheckState(Qt::CheckState::Checked);
            childItem->setCheckable(true);

            rootItem->appendRow(childItem);
        }

        modelRoot->appendRow(rootItem);
    }

    // Sort
    mTagSelectionModel->sort(0);
}

bool MainWindow::parseFrontendData()
{
    // IO Error check instance
    Qx::GenericError existingCheck;

    // Get list of existing platforms and playlists
    existingCheck = mFrontendInstall->refreshExistingDocs();

    // IO Error Check
    if(existingCheck.isValid())
        Qx::postBlockingError(existingCheck);

    // Return true on success
    return !existingCheck.isValid();
}

bool MainWindow::installsHaveChanged()
{
    // TODO: Make this check more thorough

    // Check frontend existing items
    bool changed = false;
    mFrontendInstall->refreshExistingDocs(&changed);
    return changed;
}

void MainWindow::redoInputChecks()
{
    // Check existing locations again
    validateInstall(mFrontendInstall->path(), InstallType::Frontend);
    validateInstall(mFlashpointInstall->fullPath(), InstallType::Flashpoint);
}

void MainWindow::invalidateInstall(InstallType install, bool informUser)
{
    clearListWidgets();
    mTagSelectionModel.reset(); // Void tag selection model

    switch(install)
    {
        case InstallType::Frontend:
            ui->icon_frontend_install_status->setPixmap(QPixmap(":/ui/Invalid_Install.png"));
            ui->label_frontendVersion->clear();
            if(informUser)
                QMessageBox::critical(this, QApplication::applicationName(), MSG_FE_INSTALL_INVALID);
            mFrontendInstall.reset();
            break;

        case InstallType::Flashpoint:
            ui->icon_flashpoint_install_status->setPixmap(QPixmap(":/ui/Invalid_Install.png"));
            ui->label_flashpointVersion->clear();
            if(informUser)
                Qx::postBlockingError(mFlashpointInstall->error(), QMessageBox::Ok);
            mFlashpointInstall.reset();
            break;
    }

    refreshEnableStates();
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
           mFrontendInstall->containsPlatform(ui->listWidget_platformChoices->item(i)->text()))
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
           mFrontendInstall->containsPlaylist(ui->listWidget_playlistChoices->item(i)->text()))
            return true;
    }

    // Return false if no match
    return false;
}

bool MainWindow::selectionsMayModify()
{
    return isExistingPlatformSelected() || isExistingPlaylistSelected() ||
               (getSelectedPlaylistGameMode() ==  ImportWorker::ForceAll &&
                mFrontendInstall->containsAnyPlatform(mFlashpointInstall->database()->platformList()));
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

void MainWindow::postIOError(QString mainText, Qx::IoOpReport report)
{
    QMessageBox ioErrorMsg;
    ioErrorMsg.setIcon(QMessageBox::Critical);
    ioErrorMsg.setText(mainText);
    ioErrorMsg.setInformativeText(report.outcome());
    ioErrorMsg.setStandardButtons(QMessageBox::Ok);

    ioErrorMsg.exec();
}

void MainWindow::refreshEnableStates()
{
    QHash<QWidget*, std::function<bool(void)>>::const_iterator i;
    for(i = mWidgetEnableConditionMap.constBegin(); i != mWidgetEnableConditionMap.constEnd(); i++)
        i.key()->setEnabled(i.value()());

    QHash<QAction*, std::function<bool(void)>>::const_iterator j;
    for(j = mActionEnableConditionMap.constBegin(); j != mActionEnableConditionMap.constEnd(); j++)
        j.key()->setEnabled(j.value()());
}

void MainWindow::refreshCheckStates()
{
    // Determine allowed/preferred image mode order
    QList<Fe::ImageMode> modeOrder = mFrontendInstall ? mFrontendInstall->preferredImageModeOrder() : DEFAULT_IMAGE_MODE_ORDER;

    // Remove link as an option if user doesn't have permissions
    if(!mHasLinkPermissions)
        modeOrder.removeAll(Fe::ImageMode::Link);

    // Ensure an option remains
    if(modeOrder.isEmpty())
        throw std::runtime_error("MainWindow::refreshCheckStates(): At least one image import mode must be available!");

    // Move image mode selection to next preferred option if the current one is invalid
    Fe::ImageMode im = getSelectedImageMode();
    if(!modeOrder.contains(im))
    {
        Fe::ImageMode preferredMode = modeOrder.first();

        switch(preferredMode)
        {
            case Fe::ImageMode::Link:
                ui->radioButton_link->setChecked(true);
                break;
            case Fe::ImageMode::Reference:
                ui->radioButton_reference->setChecked(true);
                break;
            case Fe::ImageMode::Copy:
                ui->radioButton_copy->setChecked(true);
                break;
            default:
                qCritical("MainWindow::refreshCheckStates(): Invalid preferred image mode.");
                break;
        }
    }

    // Ensure that the force download images option is unchecked if not supported
    if(ui->action_forceDownloadImages->isChecked() && mFlashpointInstall && !mFlashpointInstall->preferences().onDemandImages)
        ui->action_forceDownloadImages->setChecked(false);
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

Fp::Db::InclusionOptions MainWindow::getSelectedInclusionOptions() const
{
    return {generateTagExlusionSet(), ui->action_includeAnimations->isChecked()};
}

Fe::UpdateOptions MainWindow::getSelectedUpdateOptions() const
{
    return {ui->radioButton_onlyAdd->isChecked() ? Fe::ImportMode::OnlyNew : Fe::ImportMode::NewAndExisting, ui->checkBox_removeMissing->isChecked() };
}

Fe::ImageMode MainWindow::getSelectedImageMode() const
{
    return ui->radioButton_copy->isChecked() ? Fe::ImageMode::Copy : ui->radioButton_reference->isChecked() ? Fe::ImageMode::Reference : Fe::ImageMode::Link;
}

ImportWorker::PlaylistGameMode MainWindow::getSelectedPlaylistGameMode() const
{
    return ui->radioButton_selectedPlatformsOnly->isChecked() ? ImportWorker::SelectedPlatform : ImportWorker::ForceAll;
}

bool MainWindow::getForceDownloadImages() const
{
    return ui->action_forceDownloadImages->isChecked();
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
    if(selectionsMayModify())
        if(QMessageBox::warning(this, QApplication::applicationName(), MSG_PRE_EXISTING_IMPORT, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel)
            return;

    // Warn user if Flashpoint is running
    // Check if Flashpoint is running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_NAME))
        QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_CLOSE_PROMPT);

    // Only allow proceeding if frontend isn't running
    bool feRunning;
    while((feRunning = Qx::processIsRunning(mFrontendInstall->executableName())))
        if(QMessageBox::critical(this, QApplication::applicationName(), MSG_FRONTEND_CLOSE_PROMPT, QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
            break;

    if(!feRunning)
    {
        // Create progress dialog, set initial state and show
        mImportProgressDialog = std::make_unique<QProgressDialog>(STEP_FP_DB_INITIAL_QUERY, "Cancel", 0, 0, this);
        mImportProgressDialog->setWindowTitle(CAPTION_IMPORTING);
        mImportProgressDialog->setWindowModality(Qt::WindowModal);
        mImportProgressDialog->setWindowFlags(mImportProgressDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        mImportProgressDialog->setAutoReset(false);
        mImportProgressDialog->setAutoClose(false);
        mImportProgressDialog->setMinimumDuration(0); // Always show pd
        mImportProgressDialog->setValue(0); // Get pd to show

        // Setup taskbar button progress indicator
        mWindowTaskbarButton->setProgressMinimum(0);
        mWindowTaskbarButton->setProgressMaximum(0);
        mWindowTaskbarButton->setProgressValue(0);
        mWindowTaskbarButton->setProgressState(Qx::TaskbarButton::Busy);

        // Force show progress immediately
        QApplication::processEvents();

        // Setup import worker
        ImportWorker::ImportSelections impSel{.platforms = getSelectedPlatforms(),
                                              .playlists =getSelectedPlaylists()};
        ImportWorker::OptionSet optSet{
            getSelectedUpdateOptions(),
            getSelectedImageMode(),
            getForceDownloadImages(),
            getSelectedPlaylistGameMode(),
            getSelectedInclusionOptions()
        };
        ImportWorker importWorker(mFlashpointInstall, mFrontendInstall, impSel, optSet);

        // Setup blocking error connection
        connect(&importWorker, &ImportWorker::blockingErrorOccured, this, &MainWindow::handleBlockingError);

        // Setup auth handler
        connect(&importWorker, &ImportWorker::authenticationRequired, this, &MainWindow::handleAuthRequest);

        // Create process update connections
        connect(&importWorker, &ImportWorker::progressStepChanged, mImportProgressDialog.get(), &QProgressDialog::setLabelText);
        connect(&importWorker, &ImportWorker::progressMaximumChanged, mImportProgressDialog.get(), &QProgressDialog::setMaximum);
        connect(&importWorker, &ImportWorker::progressMaximumChanged, mWindowTaskbarButton, &Qx::TaskbarButton::setProgressMaximum);
        connect(&importWorker, &ImportWorker::progressValueChanged, mImportProgressDialog.get(), &QProgressDialog::setValue);
        connect(&importWorker, &ImportWorker::progressValueChanged, mWindowTaskbarButton, &Qx::TaskbarButton::setProgressValue);
        connect(mImportProgressDialog.get(), &QProgressDialog::canceled, &importWorker, &ImportWorker::notifyCanceled);

        // Import error tracker
        Qx::GenericError importError;

        // Start import and forward result to handler
        ImportWorker::ImportResult importResult = importWorker.doImport(importError);
        handleImportResult(importResult, importError);
    }
}

void MainWindow::revertAllFrontendChanges()
{
    // Trackers
    bool tempSkip = false;
    bool alwaysSkip = false;
    Qx::GenericError currentError;
    int retryChoice;

    // Progress
    QProgressDialog reversionProgress(CAPTION_REVERT, QString(), 0, mFrontendInstall->revertQueueCount(), this);
    reversionProgress.setWindowModality(Qt::WindowModal);
    reversionProgress.setAutoReset(false);

    while(mFrontendInstall->revertNextChange(currentError, alwaysSkip || tempSkip) != 0)
    {
        // Check for error
        if(!currentError.isValid())
        {
            tempSkip = false;
            reversionProgress.setValue(reversionProgress.value() + 1);
        }
        else
        {
            currentError.setCaption(CAPTION_REVERT_ERR);
            retryChoice = Qx::postBlockingError(currentError, QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort, QMessageBox::Retry);

            if(retryChoice == QMessageBox::Ignore)
                tempSkip = true;
            else if(retryChoice == QMessageBox::Abort)
                alwaysSkip = true;
        }
    }

    // Ensure progress dialog is closed
    reversionProgress.close();

    // Reset instance
    mFrontendInstall->softReset();
}

void MainWindow::standaloneCLIFpDeploy()
{
    // Browse for install
    QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE, QDir::currentPath());

    if(!selectedDir.isEmpty())
    {
        Fp::Install tempFlashpointInstall(selectedDir);
        if(tempFlashpointInstall.isValid())
        {
            if(!installMatchesTargetSeries(tempFlashpointInstall))
                QMessageBox::warning(this, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);

            bool willDeploy = true;

            // Check for existing CLIFp
            if(CLIFp::hasCLIFp(tempFlashpointInstall))
            {
                // Notify user if this will be a downgrade
                if(mInternalCLIFpVersion < CLIFp::currentCLIFpVersion(tempFlashpointInstall))
                    willDeploy = (QMessageBox::warning(this, CAPTION_CLIFP_DOWNGRADE, MSG_FP_CLFIP_WILL_DOWNGRADE, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==  QMessageBox::Yes);
            }

            // Deploy CLIFp if applicable
            if(willDeploy)
            {
                // Deploy exe
                QString deployError;
                while(!CLIFp::deployCLIFp(deployError, tempFlashpointInstall, ":/file/CLIFp.exe"))
                    if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP.arg(deployError), QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
                        break;
            }
        }
        else
            Qx::postBlockingError(tempFlashpointInstall.error(), QMessageBox::Ok);
    }
}
void MainWindow::showTagSelectionDialog()
{
    // Ensure tags have been populated
    assert(mTagSelectionModel);

    // Cache current selection states
    QHash<QStandardItem*,Qt::CheckState> originalCheckStates;
    mTagSelectionModel->forEachItem([&](QStandardItem* item) { originalCheckStates[item] = item->checkState(); });

    // Create dialog
    Qx::TreeInputDialog tagSelectionDialog(this);
    tagSelectionDialog.setModel(mTagSelectionModel.get());
    tagSelectionDialog.setWindowFlags(tagSelectionDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    tagSelectionDialog.setWindowTitle(CAPTION_TAG_FILTER);
    connect(&tagSelectionDialog, &Qx::TreeInputDialog::selectNoneClicked, mTagSelectionModel.get(), &Qx::StandardItemModel::selectNone);
    connect(&tagSelectionDialog, &Qx::TreeInputDialog::selectAllClicked, mTagSelectionModel.get(), &Qx::StandardItemModel::selectAll);

    // Present dialog and capture commitment choice
    int dc = tagSelectionDialog.exec();

    // If new selections were canceled, restore previous ones
    if(dc == QDialog::Rejected)
        mTagSelectionModel->forEachItem([&](QStandardItem* item) { item->setCheckState(originalCheckStates[item]); });
}

QSet<int> MainWindow::generateTagExlusionSet() const
{
    QSet<int> exclusionSet;

    mTagSelectionModel->forEachItem([&exclusionSet](QStandardItem* item){
        if(item->data(USER_ROLE_TAG_ID).isValid() && item->checkState() == Qt::Unchecked)
            exclusionSet.insert(item->data(USER_ROLE_TAG_ID).toInt());
    });

    return exclusionSet;
}

//Protected:
void MainWindow::showEvent(QShowEvent* event)
{
    // Call standard function
    QMainWindow::showEvent(event);

    // Configure taskbar button
    mWindowTaskbarButton = new Qx::TaskbarButton(this);
    mWindowTaskbarButton->setWindow(this->windowHandle());
}

//Public:
bool MainWindow::initCompleted() { return mInitCompleted; }

//-Slots---------------------------------------------------------------------------------------------------------
//Private:
void MainWindow::all_on_action_triggered()
{
    // Get the object that called this slot
    QAction* senderAction = qobject_cast<QAction *>(sender());

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
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
    else if(senderAction == ui->action_editTagFilter)
        showTagSelectionDialog();
    else
        throw std::runtime_error("Unhandled use of all_on_action_triggered() slot");
}

void MainWindow::all_on_pushButton_clicked()
{
    // Get the object that called this slot
    QPushButton* senderPushButton = qobject_cast<QPushButton *>(sender());

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
    if(senderPushButton == nullptr)
        throw std::runtime_error("Pointer conversion to push button failed");

    // Determine sender and take corresponding action
    if(senderPushButton == ui->pushButton_frontendBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FRONTEND_BROWSE,
                                                                (QFileInfo::exists(ui->lineEdit_frontendPath->text()) ? ui->lineEdit_frontendPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_frontendPath->setText(QDir::toNativeSeparators(selectedDir));
            validateInstall(selectedDir, InstallType::Frontend);
        }
    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE,
                                                                (QFileInfo::exists(ui->lineEdit_flashpointPath->text()) ? ui->lineEdit_flashpointPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_flashpointPath->setText(QDir::toNativeSeparators(selectedDir));
            validateInstall(selectedDir, InstallType::Flashpoint);
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

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
    if(senderLineEdit == nullptr)
        throw std::runtime_error("Pointer conversion to line edit failed");

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_frontendPath)
        checkManualInstallInput(InstallType::Frontend);
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
        checkManualInstallInput(InstallType::Flashpoint);
    else
        throw std::runtime_error("Unhandled use of all_on_linedEdit_textEdited() slot");
}

void MainWindow::all_on_listWidget_itemChanged(QListWidgetItem* item) // Proxy for "onItemChecked"
{
    // Get the object that called this slot
    QListWidget* senderListWidget = qobject_cast<QListWidget*>(sender());

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
    if(senderListWidget == nullptr)
        throw std::runtime_error("Pointer conversion to list widget failed");

    if(senderListWidget == ui->listWidget_platformChoices)
    {
        // Check if change was change in check state
        if(mPlatformItemCheckStates.contains(item) && item->checkState() != mPlatformItemCheckStates.value(item))
            refreshEnableStates();

        // Add/update check state
        mPlatformItemCheckStates[item] = item->checkState();
    }
    else if(senderListWidget == ui->listWidget_playlistChoices)
    {
        // Check if change was change in check state
        if(mPlaylistItemCheckStates.contains(item) && item->checkState() != mPlaylistItemCheckStates.value(item))
            refreshEnableStates();

        // Add/update check state
        mPlaylistItemCheckStates[item] = item->checkState();
    }
    else
        throw std::runtime_error("Unhandled use of all_on_listWidget_itemChanged() slot");
}

void MainWindow::all_on_radioButton_clicked()
{
    // Get the object that called this slot
    QRadioButton* senderRadioButton = qobject_cast<QRadioButton*>(sender());

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
    if(senderRadioButton == nullptr)
        throw std::runtime_error("Pointer conversion to radio button failed");

    if(senderRadioButton == ui->radioButton_selectedPlatformsOnly)
        refreshEnableStates();
    else if(senderRadioButton == ui->radioButton_forceAll)
        refreshEnableStates();
    else
        throw std::runtime_error("Unhandled use of all_on_radioButton_clicked() slot");
}

void MainWindow::all_on_menu_triggered(QAction *action)
{
    // Get the object that called this slot
    QMenu* senderMenu = qobject_cast<QMenu*>(sender());

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
    if(senderMenu == nullptr)
        throw std::runtime_error("Pointer conversion to menu failed");

    if(senderMenu == ui->menu_frontendHelp)
    {
        // Get associated help URL and open it
        QRegularExpressionMatch frontendMatch = MENU_FE_HELP_KEY_REGEX.match(action->objectName());

        if(frontendMatch.hasMatch())
        {
            QString frontendName = frontendMatch.captured("frontend");
            if(!frontendName.isNull() && Fe::Install::registry().contains(frontendName))
            {
                const QUrl* helpUrl = Fe::Install::registry()[frontendName].helpUrl;
                QDesktopServices::openUrl(*helpUrl);
                return;
            }
        }

        qWarning() << Q_FUNC_INFO << "Frontend help action name could not be determined.";
    }
    else
        throw std::runtime_error("Unhandled use of all_on_menu_triggered() slot");
}

void MainWindow::handleBlockingError(std::shared_ptr<int> response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices)
{
    // Get taskbar progress and indicate error
    mWindowTaskbarButton->setProgressState(Qx::TaskbarButton::Stopped);

    // Post error and get response
    int userChoice = Qx::postBlockingError(blockingError, choices);

    // Clear taskbar error
    mWindowTaskbarButton->setProgressState(Qx::TaskbarButton::Normal);

    // If applicable return selection
    if(response)
        *response = userChoice;
}

void MainWindow::handleAuthRequest(QString prompt, QAuthenticator* authenticator)
{
    Qx::LoginDialog ld;
    ld.setPrompt(prompt);

    int choice = ld.exec();

    if(choice == QDialog::Accepted)
    {
        authenticator->setUser(ld.username());
        authenticator->setPassword(ld.password());
    }
}

void MainWindow::handleImportResult(ImportWorker::ImportResult importResult, Qx::GenericError errorReport)
{
    // Close progress dialog and reset taskbar progress indicator
    mImportProgressDialog->close();
    mWindowTaskbarButton->resetProgress();
    mWindowTaskbarButton->setProgressState(Qx::TaskbarButton::Hidden);

    // Post error report if present
    if(errorReport.isValid())
        Qx::postBlockingError(errorReport, QMessageBox::Ok);

    if(importResult == ImportWorker::Successful)
    {
        bool willDeploy = true;

        // Check for existing CLIFp
        if(CLIFp::hasCLIFp(*mFlashpointInstall))
        {
            // Notify user if this will be a downgrade
            if(mInternalCLIFpVersion < CLIFp::currentCLIFpVersion(*mFlashpointInstall))
                willDeploy = (QMessageBox::warning(this, CAPTION_CLIFP_DOWNGRADE, MSG_FP_CLFIP_WILL_DOWNGRADE, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==  QMessageBox::Yes);
        }

        // Deploy CLIFp if applicable
        if(willDeploy)
        {
            QString deployError;
            while(!CLIFp::deployCLIFp(deployError, *mFlashpointInstall, ":/file/CLIFp.exe"))
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
        revertAllFrontendChanges();
    }
    else
    {
        // Show general next steps message
        QMessageBox::warning(this, CAPTION_REVERT, MSG_HAVE_TO_REVERT);
        revertAllFrontendChanges();
    }
}
