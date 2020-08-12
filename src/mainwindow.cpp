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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include <filesystem>


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
    mHasLinkPermissions = testForLinkPermissions();
    initializeForms();
}

//-Destructor----------------------------------------------------------------------------------------------------
MainWindow::~MainWindow() { delete ui; }

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
    mExistingItemColor = ui->label_existingItemColor->palette().color(QPalette::Window);

    ui->radioButton_launchBoxLink->setEnabled(mHasLinkPermissions);
    ui->radioButton_flashpointLink->setEnabled(mHasLinkPermissions);

    setInputStage(Paths);

    // TODO: THIS IS FOR DEBUG PURPOSES. REMOVE
    checkLaunchBoxInput("D:/LaunchBox");
    checkFlashpointInput("C:/Users/Player/Desktop/FP");
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
            ui->radioButton_onlyAdd->setChecked(true);
            ui->radioButton_updateExisting->setChecked(false);
            ui->checkBox_removeObsolete->setChecked(true);

            if(mHasLinkPermissions)
            {
                ui->radioButton_launchBoxLink->setChecked(true);
                ui->radioButton_launchBoxCopy->setChecked(false);
            }
            else
            {
                ui->radioButton_launchBoxCopy->setChecked(true);
                ui->radioButton_launchBoxLink->setChecked(false);
            }

            ui->radioButton_flashpointLink->setChecked(false);
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
        mLaunchBoxInstall = std::make_unique<LB::Install>(installPath);
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
            setInputStage(InputStage::Imports);

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
        postIOError(MSG_LB_XML_UNEXPECTED_ERROR, existingCheck);

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

void MainWindow::postXMLReadError(QString mainText, Qx::XmlStreamReaderError xmlError)
{
    QMessageBox xmlErrorMsg;
    xmlErrorMsg.setIcon(QMessageBox::Critical);
    xmlErrorMsg.setText(mainText);
    xmlErrorMsg.setInformativeText(xmlError.getText());
    xmlErrorMsg.setStandardButtons(QMessageBox::Ok);

    xmlErrorMsg.exec();
}

void MainWindow::postGenericError(QString mainText, QString informativeText)
{
    QMessageBox genericErrorMsg;
    genericErrorMsg.setIcon(QMessageBox::Critical);
    genericErrorMsg.setText(mainText);
    genericErrorMsg.setInformativeText(informativeText);
    genericErrorMsg.setStandardButtons(QMessageBox::Ok);

    genericErrorMsg.exec();
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

LB::Install::ImageMode MainWindow::getSelectedImageOption() const
{
    return ui->radioButton_launchBoxCopy->isChecked() ? LB::Install::LB_Copy : ui->radioButton_launchBoxLink->isChecked() ? LB::Install::LB_Link : LB::Install::FP_Link;
}

void MainWindow::importProcess()
{
    // Pre-import message
    QMessageBox::information(this, QApplication::applicationName(), MSG_PRE_IMPORT);

    // Create progress dialog, set initial busy state and show
    QProgressDialog importProgressDialog(PD_LABEL_FP_DB_INITIAL_QUERY, PD_BUTTON_CANCEL, 0, 0, this);
    importProgressDialog.setWindowTitle(CAPTION_IMPORTING);
    importProgressDialog.setWindowModality(Qt::WindowModal);
    importProgressDialog.show();

    // Start import
    ImportResult result = coreImportProcess(&importProgressDialog);
    if(result == Successful)
    {
        // Deploy CLIFp
        while(!mFlashpointInstall->deployCLIFp())
            if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP, QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
                break;

        // Post-import message
        QMessageBox::information(this, QApplication::applicationName(), MSG_POST_IMPORT);

        // Ensure progress dialog is closed
        importProgressDialog.close();
    }
    else if(result == Canceled)
    {
        QMessageBox::critical(this, CAPTION_REVERT, MSG_USER_CANCELED);

        // Ensure progress dialog is closed
        importProgressDialog.close();

        revertAllLaunchBoxChanges();
    }
    else
    {
        // Show general next steps message
        QMessageBox::warning(this, CAPTION_REVERT, MSG_HAVE_TO_REVERT);

        // Ensure progress dialog is closed
        importProgressDialog.close();

        revertAllLaunchBoxChanges();
    }
}

MainWindow::ImportResult MainWindow::coreImportProcess(QProgressDialog* pd)
{
    // Grab options
    QStringList platformsToImport = getSelectedPlatforms();
    QStringList playlistsToImport = getSelectedPlaylists();
    LB::Install::UpdateOptions updateOptions = getSelectedUpdateOptions();

    // Process query status
    QSqlError queryError;

    // Caches
    QSet<FP::AddApp> addAppsCache;
    QHash<QUuid, LB::PlaylistGame::EntryDetails> playlistGameDetailsCache;

    // Initial query buffers
    QList<FP::Install::DBQueryBuffer> gameQueries;
    FP::Install::DBQueryBuffer addAppQuery;
    FP::Install::DBQueryBuffer playlistQueries;
    QList<QPair<FP::Install::DBQueryBuffer, FP::Playlist>> playlistGameQueries;

    // Make initial game query
    queryError = mFlashpointInstall->initialGameQuery(gameQueries, platformsToImport);
    if(queryError.isValid())
    {
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, queryError);
        return Failed;
    }

    // Make initial add apps query
    queryError = mFlashpointInstall->initialAddAppQuery(addAppQuery);
    if(queryError.isValid())
    {
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, queryError);
        return Failed;
    }

    // Make initial playlists query
    queryError = mFlashpointInstall->initialPlaylistQuery(playlistQueries, playlistsToImport);
    if(queryError.isValid())
    {
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, queryError);
        return Failed;
    }

    // Close database connection since it's no longer needed
    mFlashpointInstall->closeDatabaseConnection();

    // Process Playlists and list for playlist game query
    QList<FP::Playlist> targetKnownPlaylists;
    for(int i = 0; i < playlistQueries.size; i++)
    {
        // Advance to next record
        playlistQueries.result.next();

        // Form playlist from record
        FP::PlaylistBuilder fpPb;
        fpPb.wID(playlistQueries.result.value(FP::Install::DBTable_Playlist::COL_ID).toString());
        fpPb.wTitle(playlistQueries.result.value(FP::Install::DBTable_Playlist::COL_TITLE).toString());
        fpPb.wDescription(playlistQueries.result.value(FP::Install::DBTable_Playlist::COL_DESCRIPTION).toString());
        fpPb.wAuthor(playlistQueries.result.value(FP::Install::DBTable_Playlist::COL_AUTHOR).toString());

        // Build playlist and add to list
        targetKnownPlaylists.append(fpPb.build());
    }

    // Make initial playlist games query
    queryError = mFlashpointInstall->initialPlaylistGameQuery(playlistGameQueries, targetKnownPlaylists);
    if(queryError.isValid())
    {
        postSqlError(MSG_FP_DB_UNEXPECTED_ERROR, queryError);
        return Failed;
    }

    // Determine workload
    int maximumSteps = addAppQuery.size; // Additional App pre-load
    for(FP::Install::DBQueryBuffer& query : gameQueries) // All games
        maximumSteps += query.size;
    for(QPair<FP::Install::DBQueryBuffer, FP::Playlist>& query : playlistGameQueries) // All playlist games
        maximumSteps += query.first.size;
    maximumSteps += addAppQuery.size * gameQueries.size(); // All checks of Additional Apps

    // Re-prep progress dialog
    pd->setMaximum(maximumSteps);
    pd->setLabelText(PD_LABEL_ADD_APP_PRELOAD);

    // Pre-load additional apps
    addAppsCache.reserve(addAppQuery.size);
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Advance to next record
        addAppQuery.result.next();

        // Form additional app from record
        FP::AddAppBuilder fpAab;
        fpAab.wID(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_ID).toString());
        fpAab.wAppPath(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString());
        fpAab.wAutorunBefore(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_AUTORUN).toString());
        fpAab.wLaunchCommand(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_LAUNCH_COMMNAND).toString());
        fpAab.wName(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString());
        fpAab.wWaitExit(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_WAIT_EXIT).toString());
        fpAab.wParentID(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_PARENT_ID).toString());

        // Build additional app
        FP::AddApp additionalApp = fpAab.build();

        // Add to cache
        addAppsCache.insert(additionalApp);

        // Update progress dialog value
        if(pd->wasCanceled())
            return Canceled;
        else
            pd->setValue(pd->value() + 1);
    }

    // Image import error message
    QMessageBox imageErrorMsg;
    imageErrorMsg.setWindowTitle(CAPTION_IMAGE_ERR);
    imageErrorMsg.setInformativeText("Retry?");
    imageErrorMsg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    imageErrorMsg.setDefaultButton(QMessageBox::Yes);

    // Process games and additional apps by platform
    for(FP::Install::DBQueryBuffer& currentPlatformGameResult : gameQueries)
    {           
        // Update progress dialog label
        pd->setLabelText(PD_LABEL_IMPORTING_PLATFORM_GAMES.arg(currentPlatformGameResult.source));

        // Open LB platform doc
        LB::Install::XMLHandle docRequest = {LB::Install::Platform, currentPlatformGameResult.source};
        std::unique_ptr<LB::Install::XMLDoc> currentPlatformXML;
        Qx::XmlStreamReaderError platformReadError = mLaunchBoxInstall->openXMLDocument(currentPlatformXML, docRequest, updateOptions);

        // Stop import if error occured
        if(platformReadError.isValid())
        {
            postXMLReadError(MSG_LB_XML_UNEXPECTED_ERROR, platformReadError);
            return Failed;
        }

        // Add/Update games
        for(int i = 0; i < currentPlatformGameResult.size; i++)
        {
            // Advance to next record
            currentPlatformGameResult.result.next();

            // Form game from record
            FP::GameBuilder fpGb;
            fpGb.wID(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_ID).toString());
            fpGb.wTitle(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_TITLE).toString());
            fpGb.wSeries(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_SERIES).toString());
            fpGb.wDeveloper(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_DEVELOPER).toString());
            fpGb.wPublisher(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_PUBLISHER).toString());
            fpGb.wDateAdded(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_DATE_ADDED).toString());
            fpGb.wDateModified(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_DATE_MODIFIED).toString());
            fpGb.wPlatform(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_PLATFORM).toString());
            fpGb.wBroken(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_BROKEN).toString());
            fpGb.wPlayMode(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_PLAY_MODE).toString());
            fpGb.wStatus(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_STATUS).toString());
            fpGb.wNotes(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_NOTES).toString());
            fpGb.wSource(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_SOURCE).toString());
            fpGb.wAppPath(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_APP_PATH).toString());
            fpGb.wLaunchCommand(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_LAUNCH_COMMAND).toString());
            fpGb.wReleaseDate(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_RELEASE_DATE).toString());
            fpGb.wVersion(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_VERSION).toString());
            fpGb.wOriginalDescription(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_ORIGINAL_DESC).toString());
            fpGb.wLanguage(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_LANGUAGE).toString());
            fpGb.wOrderTitle(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_ORDER_TITLE).toString());

            // Convert and convert FP game to LB game and add to document
            LB::Game builtGame = LB::Game(fpGb.build(), mFlashpointInstall->getOFLIbPath());
            currentPlatformXML->addGame(builtGame);

            // Transfer game images
            QString imageError;
            while(!mLaunchBoxInstall->transferImages(imageError, getSelectedImageOption(), mFlashpointInstall->getLogosDirectory(), mFlashpointInstall->getScrenshootsDirectory(), builtGame))
            {
                imageErrorMsg.setText(imageError);
                if(imageErrorMsg.exec() == QMessageBox::No)
                    break;
            }

            // Update progress dialog value
            if(pd->wasCanceled())
                return Canceled;
            else
                pd->setValue(pd->value() + 1);
        }

        // Update progress dialog label
        pd->setLabelText(PD_LABEL_IMPORTING_PLATFORM_ADD_APPS.arg(currentPlatformGameResult.source));

        // Add applicable additional apps
        for (QSet<FP::AddApp>::iterator i = addAppsCache.begin(); i != addAppsCache.end();)
        {
            // If the current platform doc contains the game this add app belongs to, convert and add it, then remove it from cache
            if (currentPlatformXML->containsGame((*i).getParentID()))
            {
                currentPlatformXML->addAddApp(LB::AddApp(*i, mFlashpointInstall->getOFLIbPath()));
                i = addAppsCache.erase(i);

                // Reduce progress dialog maximum by total iterations cut
                pd->setMaximum(pd->maximum() - gameQueries.size());
            }
            else
                ++i;

            // Update progress dialog value
            if(pd->wasCanceled())
                return Canceled;
            else
                pd->setValue(pd->value() + 1);
        }

        // Finalize document
        currentPlatformXML->finalize();

        // Add final game details to Playlist Game lookup cache
        for (QHash<QUuid, LB::Game>::const_iterator i = currentPlatformXML->getFinalGames().constBegin();
             i != currentPlatformXML->getFinalGames().constEnd(); ++i)
            playlistGameDetailsCache[i.key()] = {i.value().getTitle(), QFileInfo(i.value().getAppPath()).fileName(), i.value().getPlatform()};

        // Forefit doucment lease and save it
        if(!mLaunchBoxInstall->saveXMLDocument(std::move(currentPlatformXML)))
        {
            postGenericError(LB::Install::populateErrorWithTarget(LB::Install::XMLWriter::ERR_WRITE_FAILED, docRequest));
            return Failed;
        }
    }

    // Process playlists
    for(QPair<FP::Install::DBQueryBuffer, FP::Playlist>& currentPlaylistGameResult : playlistGameQueries)
    {
        // Update progress dialog label
        pd->setLabelText(PD_LABEL_IMPORTING_PLAYLIST_GAMES.arg(currentPlaylistGameResult.first.source));

        // Open LB playlist doc
        LB::Install::XMLHandle docRequest = {LB::Install::Playlist, currentPlaylistGameResult.first.source};
        std::unique_ptr<LB::Install::XMLDoc> currentPlaylistXML;
        Qx::XmlStreamReaderError playlistReadError = mLaunchBoxInstall->openXMLDocument(currentPlaylistXML, docRequest, updateOptions);

        // Stop import if error occured
        if(playlistReadError.isValid())
        {
            postXMLReadError(MSG_LB_XML_UNEXPECTED_ERROR, playlistReadError);
            return Failed;
        }

        // Convert and set playlist header
        currentPlaylistXML->setPlaylistHeader(LB::PlaylistHeader(currentPlaylistGameResult.second));

        // Add/Update playlist games
        for(int i = 0; i < currentPlaylistGameResult.first.size; i++)
        {
            // Advance to next record
            currentPlaylistGameResult.first.result.next();

            // Form game from record
            FP::PlaylistGameBuilder fpPgb;
            fpPgb.wID(currentPlaylistGameResult.first.result.value(FP::Install::DBTable_Playlist_Game::COL_ID).toString());
            fpPgb.wPlaylistID(currentPlaylistGameResult.first.result.value(FP::Install::DBTable_Playlist_Game::COL_PLAYLIST_ID).toString());
            fpPgb.wOrder(currentPlaylistGameResult.first.result.value(FP::Install::DBTable_Playlist_Game::COL_ORDER).toString());
            fpPgb.wGameID(currentPlaylistGameResult.first.result.value(FP::Install::DBTable_Playlist_Game::COL_ID).toString());

            // Build and convert FP game to LB game and add to document
            currentPlaylistXML->addPlaylistGame(LB::PlaylistGame(fpPgb.build(), playlistGameDetailsCache));

            // Update progress dialog value
            if(pd->wasCanceled())
                return Canceled;
            else
                pd->setValue(pd->value() + 1);
        }

        // Finalize document
        currentPlaylistXML->finalize();

        // Forefit doucment lease and save it
        if(!mLaunchBoxInstall->saveXMLDocument(std::move(currentPlaylistXML)))
        {
            postGenericError(LB::Install::populateErrorWithTarget(LB::Install::XMLWriter::ERR_WRITE_FAILED, docRequest));
            return Failed;
        }
    }

    // Reset install
    mLaunchBoxInstall->softReset();

    // Return true on success
    return Successful;
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
    revertError.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::NoAll);
    revertError.setDefaultButton(QMessageBox::Yes);

    // Progress
    QProgressDialog reversionProgress(CAPTION_REVERT, QString(), 0, mLaunchBoxInstall->getRevertQueueCount(), this);
    reversionProgress.setWindowModality(Qt::WindowModal);

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
            else if(retryChoice == QMessageBox::NoAll)
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
            while(!tempFlashpointInstallinstallPath.deployCLIFp())
                if(QMessageBox::critical(this, CAPTION_CLIFP_ERR, MSG_FP_CANT_DEPLOY_CLIFP, QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry) == QMessageBox::Cancel)
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
                                                                (QFileInfo::exists(ui->lineEdit_launchBoxPath->text()) ? ui->lineEdit_launchBoxPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_launchBoxPath->setText(QDir::toNativeSeparators(selectedDir));
            checkLaunchBoxInput(selectedDir);
        }
    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE,
                                                                (QFileInfo::exists(ui->label_flashPointPath->text()) ? ui->label_flashPointPath->text() : QDir::currentPath()));

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
                                                                                          ui->checkBox_removeObsolete->text()));
    else if(senderPushButton == ui->pushButton_imageModeHelp)
        QMessageBox::information(this, CAPTION_IMAGE_MODE_HELP, MSG_IMAGE_MODE_HELP.arg(ui->radioButton_launchBoxCopy->text(),
                                                                                        ui->radioButton_launchBoxLink->text(),
                                                                                        ui->radioButton_flashpointLink->text()));
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
    else if(senderListWidget == ui->listWidget_playlistChoices && !mAlteringListWidget)
        importSelectionReaction(item, ui->listWidget_playlistChoices);
    else
        assert("Unhandled use of all_on_listWidget_itemChanged() slot");
}
