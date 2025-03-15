// Unit Include
#include "controller.h"

// Qt Includes
#include <QApplication>
#include <QFileDialog>

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/widgets/qx-common-widgets.h>
#include <qx/widgets/qx-logindialog.h>

// Project Includes
#include "launcher/abstract/lr-registration.h"
#include "import/backup.h"

/* TODO: Consider having this tool deploy a .ini file (or the like) into the target launcher install
 * (with the exact location probably being guided by the specific Install child) that saves the settings
 * used for the import, so that they can be loaded again when that install is targeted by future versions
 * of the tool. Would have to account for an initial import vs update (likely just leaving the update settings
 * blank). Wouldn't be a huge difference but could be a nice little time saver.
 */

//===============================================================================================================
// Controller
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Controller::Controller() :
    mImportProperties(),
    mMainWindow(mImportProperties),
    mProgressPresenter(&mMainWindow)
{
    QApplication::setApplicationName(PROJECT_FULL_NAME);

    /*Register metatypes
     * NOTE: Qt docs note these should be needed, as always, but since Qt6 signals/slots with these types seem to
     * work fine without the following calls.
     * See https://forum.qt.io/topic/136627/undocumented-automatic-metatype-registration-in-qt6
     */
    //qRegisterMetaType<Import::Worker::ImportResult>();
    //qRegisterMetaType<Qx::Error>();
    //qRegisterMetaType<std::shared_ptr<int>>();

    // Ensure built-in CLIFp version is valid
    if(CLIFp::internalVersion().isNull())
    {
        QMessageBox::critical(&mMainWindow, CAPTION_GENERAL_FATAL_ERROR, MSG_FATAL_NO_INTERNAL_CLIFP_VER);
        QApplication::exit(1);
        return;
    }

    // Check if Flashpoint is running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_NAME))
        QMessageBox::warning(&mMainWindow, QApplication::applicationName(), MSG_FP_CLOSE_PROMPT);

    // Connect main window
    connect(&mMainWindow, &MainWindow::installPathChanged, this, &Controller::updateInstallPath);
    connect(&mMainWindow, &MainWindow::importTriggered, this, &Controller::startImport);
    connect(&mMainWindow, &MainWindow::standaloneDeployTriggered, this, &Controller::standaloneCLIFpDeploy);

    // Spawn main window
    mMainWindow.show();
    mProgressPresenter.attachWindow(mMainWindow.windowHandle()); // Must be after show() for handle to be valid
}

//-Instance Functions-------------------------------------------------------------
//Private:
void Controller::processImportResult(Import::Worker::Result importResult, const Qx::Error& errorReport)
{
    // Reset progress presenter
    mProgressPresenter.reset();

    // Post error report if present
    if(errorReport.isValid())
        Qx::postBlockingError(errorReport, QMessageBox::Ok);

    if(importResult == Import::Worker::Successful)
    {
        deployCLIFp(*mImportProperties.flashpoint(), QMessageBox::Ignore);

        // Post-import message
        QMessageBox::information(&mMainWindow, QApplication::applicationName(), MSG_POST_IMPORT);

        // Update selection lists to reflect newly existing platforms
        mImportProperties.refreshInstallData();
    }
    else if(importResult == Import::Worker::Taskless)
    {
        QMessageBox::warning(&mMainWindow, CAPTION_TASKLESS_IMPORT, MSG_NO_WORK);
    }
    else if(importResult == Import::Worker::Canceled)
    {
        QMessageBox::critical(&mMainWindow, CAPTION_REVERT, MSG_USER_CANCELED);
        revertAllLauncherChanges();
    }
    else if(importResult == Import::Worker::Failed)
    {
        // Show general next steps message
        QMessageBox::warning(&mMainWindow, CAPTION_REVERT, MSG_HAVE_TO_REVERT);
        revertAllLauncherChanges();
    }
    else
        qCritical("unhandled import worker result type.");
}

void Controller::revertAllLauncherChanges()
{
    auto launcher = mImportProperties.launcher();
    auto bm = Import::BackupManager::instance();

    if(bm->hasReversions())
    {
        // Trackers
        bool tempSkip = false;
        bool alwaysSkip = false;
        Import::BackupError currentError;
        int retryChoice;

        // Progress

        mProgressPresenter.setMinimum(0);
        mProgressPresenter.setMaximum(bm->revertQueueCount());
        mProgressPresenter.setCaption(CAPTION_REVERT);
        while(bm->revertNextChange(currentError, alwaysSkip || tempSkip) != 0)
        {
            // Check for error
            if(!currentError.isValid())
            {
                tempSkip = false;
                mProgressPresenter.setValue(mProgressPresenter.value() + 1);
            }
            else
            {
                retryChoice = Qx::postBlockingError(currentError, QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Abort, QMessageBox::Retry);

                if(retryChoice == QMessageBox::Ignore)
                    tempSkip = true;
                else if(retryChoice == QMessageBox::Abort)
                    alwaysSkip = true;
            }
        }

        // Ensure progress dialog is closed
        mProgressPresenter.reset();
    }

    // Reset instance
    launcher->softReset();
}

void Controller::deployCLIFp(const Fp::Install& fp, QMessageBox::Button abandonButton)
{
    bool willDeploy = true;

    // Check for existing CLIFp
    if(CLIFp::hasCLIFp(fp))
    {
        // Notify user if this will be a downgrade
        if(CLIFp::internalVersion() < CLIFp::installedVersion(fp))
            willDeploy = (QMessageBox::warning(&mMainWindow, CAPTION_CLIFP_DOWNGRADE, MSG_FP_CLFIP_WILL_DOWNGRADE, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==  QMessageBox::Yes);
    }

    // Deploy CLIFp if applicable
    if(willDeploy)
    {
        // Deploy exe
        QString deployError;
        while(!CLIFp::deployCLIFp(deployError, fp))
            if(QMessageBox::critical(&mMainWindow, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP.arg(deployError), QMessageBox::Retry | abandonButton, QMessageBox::Retry) == abandonButton)
                break;
    }
}

//-Signals & Slots-------------------------------------------------------------
//Private Slots:
void Controller::handleBlockingError(std::shared_ptr<int> response, const Qx::Error& blockingError, QMessageBox::StandardButtons choices)
{
    mProgressPresenter.setErrorState();

    // Post error and get response
    int userChoice = Qx::postBlockingError(blockingError, choices);

    // If applicable return selection
    if(response)
        *response = userChoice;

    mProgressPresenter.resetState();
}

void Controller::handleAuthRequest(const QString& prompt, QAuthenticator* authenticator)
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

//Public Slots:
void Controller::updateInstallPath(const QString& installPath, Import::Install type)
{
    /* NOTE: The launcher and flashpoint properties here are sometimes updated with a
     * finer granularity instead of all at once at the end in a common spot in order to
     * control the exact moment that dependent properties, like status icons, update.
     */
    QDir installDir(installPath);
    QString checkedPath = installDir.isAbsolute() && installDir.exists() ? QDir::cleanPath(installPath) : QString();

    using enum Import::Install;
    switch(type)
    {
        case Launcher:
        {
            if(checkedPath.isEmpty())
                mImportProperties.setLauncher(nullptr);
            else
            {
                std::unique_ptr<Lr::IInstall> launcher;
                launcher = Lr::Registry::acquireMatch(checkedPath);
                if(!launcher)
                    QMessageBox::critical(&mMainWindow, QApplication::applicationName(), MSG_LR_INSTALL_INVALID);

                mImportProperties.setLauncher(std::move(launcher));
            }

            break;
        }
        case Flashpoint:
        {
            if(checkedPath.isEmpty())
                mImportProperties.setFlashpoint(nullptr);
            else
            {
                std::unique_ptr<Fp::Install> flashpoint;
                flashpoint = std::make_unique<Fp::Install>(checkedPath, true);
                if(!flashpoint->isValid())
                {
                    Qx::postBlockingError(flashpoint->error(), QMessageBox::Ok);
                    flashpoint.reset();
                    mImportProperties.setFlashpoint(std::move(flashpoint));
                }
                else
                {
                    mImportProperties.setFlashpoint(std::move(flashpoint));  // Updates target series property (important for status icon)
                    if(!mImportProperties.isFlashpointTargetSeries())
                        QMessageBox::warning(&mMainWindow, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);
                }
            }

            break;
        }
    }
}

void Controller::startImport(Import::Selections sel, Import::OptionSet opt, bool mayModify)
{
    auto launcher = mImportProperties.launcher();
    auto flashpoint = mImportProperties.flashpoint();

    // Ensure launcher hasn't changed
    bool changed = true; // Assume true for if error occurs
    launcher->refreshExistingDocs(&changed);
    if(changed)
    {
        QMessageBox::warning(&mMainWindow, QApplication::applicationName(), MSG_INSTALL_CONTENTS_CHANGED);
        updateInstallPath(launcher->path(), Import::Install::Launcher); // Reprocess launcher to make sure it's the same install
        return;
    }

    // Warn user if they are changing existing files
    if(mayModify)
        if(QMessageBox::warning(&mMainWindow, QApplication::applicationName(), MSG_PRE_EXISTING_IMPORT, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel)
            return;

    // Warn user if Flashpoint is running
    // Check if Flashpoint is running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_NAME))
        QMessageBox::warning(&mMainWindow, QApplication::applicationName(), MSG_FP_CLOSE_PROMPT);

    // Only allow proceeding if launcher isn't running
    bool lrRunning;
    while((lrRunning = launcher->isRunning()))
        if(QMessageBox::critical(&mMainWindow, QApplication::applicationName(), MSG_LAUNCHER_CLOSE_PROMPT, QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
            break;

    if(lrRunning)
        return;

    // Start progress presentation
    mProgressPresenter.setCaption(CAPTION_IMPORTING);
    mProgressPresenter.setMinimum(0);
    mProgressPresenter.setMaximum(0);
    mProgressPresenter.setValue(0);
    mProgressPresenter.setBusyState();
    mProgressPresenter.setLabelText(STEP_FP_DB_INITIAL_QUERY);
    QApplication::processEvents(); // Force show progress immediately

    // Setup import worker
    Import::Worker importWorker(flashpoint, launcher, sel, opt);

    // Setup blocking error connection
    connect(&importWorker, &Import::Worker::blockingErrorOccured, this, &Controller::handleBlockingError);

    // Setup auth handler
    connect(&importWorker, &Import::Worker::authenticationRequired, this, &Controller::handleAuthRequest);

    // Create process update connections
    connect(&importWorker, &Import::Worker::progressStepChanged, &mProgressPresenter, &ProgressPresenter::setLabelText);
    connect(&importWorker, &Import::Worker::progressValueChanged, &mProgressPresenter, &ProgressPresenter::setValue);
    connect(&importWorker, &Import::Worker::progressMaximumChanged, &mProgressPresenter, &ProgressPresenter::setMaximum);
    connect(&mProgressPresenter, &ProgressPresenter::canceled, &importWorker, &Import::Worker::notifyCanceled);

    // Import error tracker
    Qx::Error importError;

    // Start import and forward result to handler
    Import::Worker::Result importResult = importWorker.doImport(importError);
    processImportResult(importResult, importError);
}

void Controller::standaloneCLIFpDeploy()
{
    // Browse for install
    QString selectedDir = QFileDialog::getExistingDirectory(&mMainWindow, CAPTION_FLASHPOINT_BROWSE, QDir::currentPath());

    if(!selectedDir.isEmpty())
    {
        Fp::Install tempFlashpointInstall(selectedDir);
        if(tempFlashpointInstall.isValid())
        {
            if(!Import::Properties::installMatchesTargetSeries(tempFlashpointInstall))
                QMessageBox::warning(&mMainWindow, QApplication::applicationName(), MSG_FP_VER_NOT_TARGET);

            deployCLIFp(tempFlashpointInstall, QMessageBox::Cancel);
        }
        else
            Qx::postBlockingError(tempFlashpointInstall.error(), QMessageBox::Ok);
    }
}
