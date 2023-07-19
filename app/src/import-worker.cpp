// Unit Include
#include "import-worker.h"

// Standard Library Includes
#include <filesystem>

// Qx Includes
#include <qx/core/qx-regularexpression.h>

// Project Includes
#include "clifp.h"

//===============================================================================================================
// ImageTransferError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
ImageTransferError::ImageTransferError(Type t, const QString& src, const QString& dest) :
    mType(t),
    mSourcePath(src),
    mDestinationPath(dest)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool ImageTransferError::isValid() const { return mType != NoError; }
ImageTransferError::Type ImageTransferError::type() const { return mType; }

//Private:
Qx::Severity ImageTransferError::deriveSeverity() const { return Qx::Err; }
quint32 ImageTransferError::deriveValue() const { return mType; }
QString ImageTransferError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString ImageTransferError::deriveSecondary() const { return IMAGE_RETRY_PROMPT; }

QString ImageTransferError::deriveDetails() const
{
    QString det;
    if(!mSourcePath.isEmpty())
        det += SRC_PATH_TEMPLATE.arg(mSourcePath) + '\n';
    if(!mDestinationPath.isEmpty())
    {
        if(!det.isEmpty())
            det += '\n';
        det += DEST_PATH_TEMPLATE.arg(mDestinationPath) + '\n';
    }

    return det;
};

QString ImageTransferError::deriveCaption() const { return CAPTION_IMAGE_ERR; };

//===============================================================================================================
// IMPORT WORKER
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
ImportWorker::ImportWorker(std::shared_ptr<Fp::Install> fpInstallForWork,
                           std::shared_ptr<Fe::Install> feInstallForWork,
                           ImportSelections importSelections,
                           OptionSet optionSet) :
      mFlashpointInstall(fpInstallForWork),
      mFrontendInstall(feInstallForWork),
      mImportSelections(importSelections),
      mOptionSet(optionSet),
      mCurrentProgress(0),
      mCanceled(false)
{}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
Qx::ProgressGroup* ImportWorker::initializeProgressGroup(const QString& groupName, quint64 weight)
{
   Qx::ProgressGroup* pg = mProgressManager.addGroup(groupName);
   pg->setWeight(weight);
   pg->setValue(0);
   pg->setMaximum(0);
   return pg;
}

Qx::Error ImportWorker::preloadPlaylists(QList<Fp::Playlist>& targetPlaylists)
{
    // Reset playlists

    // Get all playlists
    Fp::PlaylistManager* plm = mFlashpointInstall->playlistManager();

    if(Qx::Error loadErr = plm->populate(); loadErr.isValid())
        return loadErr;

    // Create copy of playlists list
    targetPlaylists = plm->playlists();

    // Strip list to only cover targets
    const QStringList& plNames = mImportSelections.playlists;
    bool inclAnim = mOptionSet.inclusionOptions.includeAnimations;

    targetPlaylists.removeIf([&plNames, inclAnim](const Fp::Playlist& pl){
        return !plNames.contains(pl.title()) ||
               (pl.library() == Fp::Db::Table_Game::ENTRY_ANIM_LIBRARY && !inclAnim);
    });

    return Qx::Error();
}

QList<QUuid> ImportWorker::getPlaylistSpecificGameIds(const QList<Fp::Playlist>& playlists)
{
    QList<QUuid> playlistSpecGameIds;

    for(const Fp::Playlist& pl : playlists)
        for(const Fp::PlaylistGame& plg : pl.playlistGames())
            playlistSpecGameIds.append(plg.gameId());

    return playlistSpecGameIds;
}

ImageTransferError ImportWorker::transferImage(bool symlink, QString sourcePath, QString destinationPath)
{
    /* TODO: Ideally the error handlers here don't need to include "Retry?" text and therefore need less use of QString::arg(); however, this largely
     * would require use of a button labeled "Ignore All" so that the errors could presented as is without a prompt, with the prompt being inferred
     * through the button choices "Retry", "Ignore", and "Ignore All", but currently the last is not a standard button and due to how Qx::GenericError
     * is implemented custom buttons aren't feasible. Honestly maybe just try adding it to Qt and see if its accepted.
     */

    // Image info
    QFileInfo destinationInfo(destinationPath);
    QDir destinationDir(destinationInfo.absolutePath());
    bool destinationOccupied = destinationInfo.exists() && (destinationInfo.isFile() || destinationInfo.isSymLink());

    // Return if source in unexpectedly missing (i.e. download failure)
    if(!QFile::exists(sourcePath))
        return ImageTransferError(ImageTransferError::ImageSourceUnavailable, sourcePath);

    // Return if image is already up-to-date
    if(destinationOccupied)
    {
        if(destinationInfo.isSymLink() && symlink)
            return ImageTransferError();
        else if(!destinationInfo.isSymLink() && !symlink)
        {
            QFile source(sourcePath);
            QFile destination(destinationPath);
            QString sourceChecksum;
            QString destinationChecksum;

            if(!Qx::calculateFileChecksum(sourceChecksum, source, QCryptographicHash::Md5).isFailure() &&
               !Qx::calculateFileChecksum(destinationChecksum, destination, QCryptographicHash::Md5).isFailure() &&
               sourceChecksum.compare(destinationChecksum, Qt::CaseInsensitive) == 0)
                return ImageTransferError();
        }
        // Image is always updated if changing between Link/Copy
    }

    // Ensure destination path exists
    if(!destinationDir.mkpath("."))
        return ImageTransferError(ImageTransferError::CantCreateDirectory, QString(), destinationDir.absolutePath());

    // Determine backup path
    QString backupPath = Fe::Install::filePathToBackupPath(destinationInfo.absoluteFilePath());

    // Temporarily backup image if it already exists (also acts as deletion marking in case images for the title were removed in an update)
    if(destinationOccupied)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return ImageTransferError(ImageTransferError::ImageWontBackup, QString(), destinationPath);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer
    if(symlink)
    {
        std::filesystem::create_symlink(sourcePath.toStdString(), destinationPath.toStdString(), linkError);
        if(linkError)
        {
            QFile::rename(backupPath, destinationPath); // Restore Backup
            return ImageTransferError(ImageTransferError::ImageWontLink, sourcePath, destinationPath);
        }
        else if(QFile::exists(backupPath))
            QFile::remove(backupPath);
        else
            mFrontendInstall->addRevertableFile(destinationPath); // Only queue image to be removed on failure if its new, so existing images aren't deleted on revert
    }
    else
    {
        if(!QFile::copy(sourcePath, destinationPath))
        {
            QFile::rename(backupPath, destinationPath); // Restore Backup
            return ImageTransferError(ImageTransferError::ImageWontCopy, sourcePath, destinationPath);
        }
        else if(QFile::exists(backupPath))
            QFile::remove(backupPath);
        else
            mFrontendInstall->addRevertableFile(destinationPath); // Only queue image to be removed on failure if its new, so existing images aren't deleted on revert
    }

    // Return null error on success
    return ImageTransferError();
}

ImportWorker::ImportResult ImportWorker::processPlatformGames(Qx::Error& errorReport, std::unique_ptr<Fe::PlatformDoc>& platformDoc, Fp::Db::QueryBuffer& gameQueryResult)
{
    // Add/Update games
    for(int j = 0; j < gameQueryResult.size; j++)
    {
        // Advance to next record
        gameQueryResult.result.next();

        // Form game from record
        Fp::Game::Builder fpGb;
        fpGb.wId(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ID).toString());
        fpGb.wTitle(gameQueryResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wSeries(gameQueryResult.result.value(Fp::Db::Table_Game::COL_SERIES).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wDeveloper(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wPublisher(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wDateAdded(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DATE_ADDED).toString());
        fpGb.wDateModified(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DATE_MODIFIED).toString());
        fpGb.wBroken(gameQueryResult.result.value(Fp::Db::Table_Game::COL_BROKEN).toString());
        fpGb.wPlayMode(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PLAY_MODE).toString());
        fpGb.wStatus(gameQueryResult.result.value(Fp::Db::Table_Game::COL_STATUS).toString());
        fpGb.wNotes(gameQueryResult.result.value(Fp::Db::Table_Game::COL_NOTES).toString());
        fpGb.wSource(gameQueryResult.result.value(Fp::Db::Table_Game::COL_SOURCE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wAppPath(gameQueryResult.result.value(Fp::Db::Table_Game::COL_APP_PATH).toString());
        fpGb.wLaunchCommand(gameQueryResult.result.value(Fp::Db::Table_Game::COL_LAUNCH_COMMAND).toString());
        fpGb.wReleaseDate(gameQueryResult.result.value(Fp::Db::Table_Game::COL_RELEASE_DATE).toString());
        fpGb.wVersion(gameQueryResult.result.value(Fp::Db::Table_Game::COL_VERSION).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wOriginalDescription(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ORIGINAL_DESC).toString());
        fpGb.wLanguage(gameQueryResult.result.value(Fp::Db::Table_Game::COL_LANGUAGE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wOrderTitle(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ORDER_TITLE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wLibrary(gameQueryResult.result.value(Fp::Db::Table_Game::COL_LIBRARY).toString());
        fpGb.wPlatformName(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PLATFORM_NAME).toString());

        Fp::Game builtGame = fpGb.build();

        // Construct full game set
        Fp::Set::Builder sb;
        sb.wGame(builtGame); // From above
        sb.wAddApps(mAddAppsCache.values(builtGame.id())); // All associated additional apps from cache
        mAddAppsCache.remove(builtGame.id());

        Fp::Set builtSet = sb.build();

        // Get image information
        QFileInfo logoLocalInfo(mFlashpointInstall->imageLocalPath(Fp::ImageType::Logo, builtGame.id()));
        QFileInfo ssLocalInfo(mFlashpointInstall->imageLocalPath(Fp::ImageType::Screenshot, builtGame.id()));

        // Add set to doc
        QString checkedLogoPath = (logoLocalInfo.exists() || mOptionSet.downloadImages) ? logoLocalInfo.absoluteFilePath() : QString();
        QString checkedScreenshotPath = (ssLocalInfo.exists() || mOptionSet.downloadImages) ? ssLocalInfo.absoluteFilePath() : QString();
        platformDoc->addSet(builtSet, Fe::ImageSources(checkedLogoPath, checkedScreenshotPath));

        // Add ID to imported game cache
        mImportedGameIdsCache.insert(builtGame.id());

        // Setup image downloads if applicable
        if(mOptionSet.downloadImages)
        {
            if(!logoLocalInfo.exists())
            {
                QUrl logoRemotePath = mFlashpointInstall->imageRemoteUrl(Fp::ImageType::Logo, builtGame.id());
                mImageDownloadManager.appendTask(Qx::DownloadTask{logoRemotePath, logoLocalInfo.absoluteFilePath()});
            }
            else
                mProgressManager.group(Pg::ImageDownload)->decrementMaximum(); // Already exists, remove download step from progress bar

            if(!ssLocalInfo.exists())
            {
                QUrl ssRemotePath = mFlashpointInstall->imageRemoteUrl(Fp::ImageType::Screenshot, builtGame.id());
                mImageDownloadManager.appendTask(Qx::DownloadTask{ssRemotePath, ssLocalInfo.absoluteFilePath()});
            }
            else
                mProgressManager.group(Pg::ImageDownload)->decrementMaximum(); // Already exists, remove download step from progress bar
        }

        // Handle image transfer progress
        if(mOptionSet.imageMode == Fe::ImageMode::Copy || mOptionSet.imageMode == Fe::ImageMode::Link)
        {
            // Adjust progress if images aren't available
            if(checkedLogoPath.isEmpty())
                mProgressManager.group(Pg::ImageTransfer)->decrementMaximum(); // Can't transfer image that doesn't/won't exist
            if(checkedScreenshotPath.isEmpty())
                mProgressManager.group(Pg::ImageTransfer)->decrementMaximum(); // Can't transfer image that doesn't/won't exist
        }

        // Update progress dialog value for game addition
        if(mCanceled)
        {
            errorReport = Qx::Error();
            return Canceled;
        }
        else
            mProgressManager.group(Pg::GameImport)->incrementValue();
    }

    // Report successful step completion
    errorReport = Qx::Error();
    return Successful;
}

void ImportWorker::cullUnimportedPlaylistGames(QList<Fp::Playlist>& playlists)
{
    const auto& idCache = mImportedGameIdsCache;
    for(auto& pl : playlists)
    {
        pl.playlistGames().removeIf([idCache](const Fp::PlaylistGame& plg){
            return !idCache.contains(plg.gameId());
        });
    }
}

ImportWorker::ImportResult ImportWorker::preloadAddApps(Qx::Error& errorReport, Fp::Db::QueryBuffer& addAppQuery)
{
    mAddAppsCache.reserve(addAppQuery.size);
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Advance to next record
        addAppQuery.result.next();

        // Form additional app from record
        Fp::AddApp::Builder fpAab;
        fpAab.wId(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_ID).toString());
        fpAab.wAppPath(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_APP_PATH).toString());
        fpAab.wAutorunBefore(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_AUTORUN).toString());
        fpAab.wLaunchCommand(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_LAUNCH_COMMAND).toString());
        fpAab.wName(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_NAME).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpAab.wWaitExit(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_WAIT_EXIT).toString());
        fpAab.wParentId(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toString());

        // Build additional app
        Fp::AddApp additionalApp = fpAab.build();

        // Add to cache
        mAddAppsCache.insert(additionalApp.parentId(), additionalApp);

        // Update progress dialog value
        if(mCanceled)
        {
           errorReport = Qx::Error();
           return Canceled;
        }
        else
            mProgressManager.group(Pg::AddAppPreload)->incrementValue();
    }

    // Report successful step completion
    errorReport = Qx::Error();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processGames(Qx::Error& errorReport, QList<Fp::Db::QueryBuffer>& primary, QList<Fp::Db::QueryBuffer>& playlistSpecific)
{    
    // Status tracking
    ImportResult platformImportStatus;

    // Track total platforms that have been handled
    qsizetype remainingPlatforms = primary.size() + playlistSpecific.size();

    // Use lambda to handle both lists due to major overlap
    auto platformsHandler = [&remainingPlatforms, &errorReport, this](QList<Fp::Db::QueryBuffer>& platformQueryResults, QString label) -> ImportResult {
        ImportResult result;

        for(int i = 0; i < platformQueryResults.size(); i++)
        {
            Fp::Db::QueryBuffer& currentQueryResult = platformQueryResults[i];

            // Open frontend platform doc
            std::unique_ptr<Fe::PlatformDoc> currentPlatformDoc;
            Fe::DocHandlingError platformReadError = mFrontendInstall->checkoutPlatformDoc(currentPlatformDoc, currentQueryResult.source);

            // Stop import if error occurred
            if(platformReadError.isValid())
            {
                // Emit import failure
                errorReport = platformReadError;
                return Failed;
            }

            //---Import games---------------------------------------
            emit progressStepChanged(label.arg(currentQueryResult.source));
            if((result = processPlatformGames(errorReport, currentPlatformDoc, currentQueryResult)) != Successful)
                return result;

            //---Finalize document----------------------------------
            currentPlatformDoc->finalize();

            // Check for internal doc errors
            if(currentPlatformDoc->hasError())
            {
                errorReport = currentPlatformDoc->error();
                return Failed;
            }

            // Forfeit document lease and save it
            Fe::DocHandlingError saveError;
            if((saveError = mFrontendInstall->commitPlatformDoc(std::move(currentPlatformDoc))).isValid())
            {
                errorReport = saveError;
                return Failed;
            }

            // Reduce remaining platform count
            remainingPlatforms--;
        }

        // Return success
        errorReport = Qx::Error();
        return Successful;
    };

    // Import primary platforms
    if((platformImportStatus = platformsHandler(primary, STEP_IMPORTING_PLATFORM_SETS)) != Successful)
        return platformImportStatus;

    // Import playlist specific platforms
    if((platformImportStatus = platformsHandler(playlistSpecific, STEP_IMPORTING_PLAYLIST_SPEC_SETS)) != Successful)
        return platformImportStatus;

    // Return success
    errorReport = Qx::Error();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processPlaylists(Qx::Error& errorReport, const QList<Fp::Playlist>& playlists)
{
    for(const auto& currentPlaylist : playlists)
    {
        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLAYLISTS.arg(currentPlaylist.title()));

        // Open frontend playlist doc
        std::unique_ptr<Fe::PlaylistDoc> currentPlaylistDoc;
        Fe::DocHandlingError playlistReadError = mFrontendInstall->checkoutPlaylistDoc(currentPlaylistDoc, currentPlaylist.title());

        // Stop import if error occurred
        if(playlistReadError.isValid())
        {
            errorReport = playlistReadError;
            return Failed;
        }

        // Convert and set playlist header
        currentPlaylistDoc->setPlaylistData(currentPlaylist);

        // Finalize document
        currentPlaylistDoc->finalize();

        // Check for internal doc errors
        if(currentPlaylistDoc->hasError())
        {
            errorReport = currentPlaylistDoc->error();
            return Failed;
        }

        // Forfeit document lease and save it
        Fe::DocHandlingError saveError;
        if((saveError = mFrontendInstall->commitPlaylistDoc(std::move(currentPlaylistDoc))).isValid())
        {
            errorReport = saveError;
            return Failed;
        }

        // Update progress dialog value
        if(mCanceled)
        {
            errorReport = Qx::Error();
            return Canceled;
        }
        else
            mProgressManager.group(Pg::PlaylistImport)->incrementValue();
    }

    // Report successful step completion
    errorReport = Qx::Error();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processImages(Qx::Error& errorReport)
{
    //-Image Download---------------------------------------------------------------------------------
    if(mOptionSet.downloadImages && mImageDownloadManager.hasTasks())
    {
        // Update progress dialog label
        emit progressStepChanged(STEP_DOWNLOADING_IMAGES);

        // Configure manager
        mImageDownloadManager.setMaxSimultaneous(2);
        mImageDownloadManager.setOverwrite(false); // Should be no attempts to overwrite, but here just in case
        mImageDownloadManager.setStopOnError(false); // Get as many images as possible;
        mImageDownloadManager.setSkipEnumeration(true); // Since progress is being tracked by task count, pre-enumeration of download size is unnecessary
        mImageDownloadManager.setTransferTimeout(5000); // 5 seconds max to start downloading image before moving on

        // Make connections
        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::sslErrors, this, [&](Qx::Error errorMsg, bool* ignore) {
            emit blockingErrorOccured(mBlockingErrorResponse, errorMsg, QMessageBox::Yes | QMessageBox::Abort);
            *ignore = *mBlockingErrorResponse == QMessageBox::Yes;
        });

        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::authenticationRequired, this, &ImportWorker::authenticationRequired);
        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::proxyAuthenticationRequired, this, &ImportWorker::authenticationRequired);

        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::downloadFinished, this, [this]() { // clazy:exclude=lambda-in-connect
                mProgressManager.group(Pg::ImageDownload)->incrementValue();
        });

        // Start download
        Qx::DownloadManagerReport downloadReport = mImageDownloadManager.processQueue();

        // Handle result
        if(!downloadReport.wasSuccessful())
        {
            // Notify GUI Thread of error
            emit blockingErrorOccured(mBlockingErrorResponse, downloadReport, QMessageBox::Abort | QMessageBox::Ignore);

            // Check response
            if(*mBlockingErrorResponse == QMessageBox::Abort)
            {
                errorReport = Qx::Error();
                return Canceled;
            }
        }
    }

    //-Image Import---------------------------------------------------------------------------

    // Update progress dialog label
    emit progressStepChanged(STEP_IMPORTING_IMAGES);

    // Provide frontend with bulk reference locations and acquire any transfer tasks
    QList<Fe::Install::ImageMap> imageTransferJobs;
    Fe::ImageSources bulkSources;
    if(mOptionSet.imageMode == Fe::ImageMode::Reference)
    {
        bulkSources.setLogoPath(QDir::toNativeSeparators(mFlashpointInstall->logosDirectory().absolutePath()));
        bulkSources.setScreenshotPath(QDir::toNativeSeparators(mFlashpointInstall->screenshotsDirectory().absolutePath()));
    }

    Qx::Error imageExchangeError = mFrontendInstall->preImageProcessing(imageTransferJobs, bulkSources);

    if(imageExchangeError.isValid())
    {
        // Emit import failure
        errorReport = imageExchangeError;
        return Failed;
    }

    // Perform transfers if required
    if(mOptionSet.imageMode == Fe::ImageMode::Copy || mOptionSet.imageMode == Fe::ImageMode::Link)
    {
        /*
         * Account for potential mismatch between assumed and actual job count.
         * For example, this may happen with infinity if a game hasn't been clicked on, as the logo
         * will have been downloaded but not the screenshot
         */
        if(static_cast<quint64>(imageTransferJobs.size()) != mProgressManager.group(Pg::ImageTransfer)->maximum())
            mProgressManager.group(Pg::ImageTransfer)->setMaximum(imageTransferJobs.size());

        // Setup for image transfers
        ImageTransferError imageTransferError; // Error return reference
        *mBlockingErrorResponse = QMessageBox::NoToAll; // Default to choice "NoToAll" in case the signal is not correctly connected using Qt::BlockingQueuedConnection
        bool ignoreAllTransferErrors = false; // NoToAll response tracker

        for(const Fe::Install::ImageMap& imageJob : qAsConst(imageTransferJobs))
        {
            while((imageTransferError = transferImage(mOptionSet.imageMode == Fe::ImageMode::Link, imageJob.sourcePath, imageJob.destPath)).isValid() && !ignoreAllTransferErrors)
            {
                // Notify GUI Thread of error
                emit blockingErrorOccured(mBlockingErrorResponse, imageTransferError,
                                          QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);

                // Check response
                if(*mBlockingErrorResponse == QMessageBox::No)
                   break;
                else if(*mBlockingErrorResponse == QMessageBox::NoToAll)
                   ignoreAllTransferErrors = true;
            }

           // Update progress dialog value
           if(mCanceled)
           {
               errorReport = Qx::Error();
               return Canceled;
           }
           else
               mProgressManager.group(Pg::ImageTransfer)->incrementValue();
        }
    }
    else if(!imageTransferJobs.isEmpty())
        qWarning() << Q_FUNC_INFO << "the frontend provided image transfers when the mode wasn't link/copy";

    // Handle frontend specific actions
    mFrontendInstall->postImageProcessing();

    // Report successful step completion
    errorReport = Qx::Error();
    return Successful;
}

//Public
ImportWorker::ImportResult ImportWorker::doImport(Qx::Error& errorReport)
{
    //-Setup----------------------------------------------------------------

    // Import step status
    ImportResult importStepStatus;

    // Process query status
    Fp::DbError queryError;

    // Initial query buffers
    QList<Fp::Db::QueryBuffer> gameQueries;
    QList<Fp::Db::QueryBuffer> playlistSpecGameQueries;
    Fp::Db::QueryBuffer addAppQuery;

    // Get flashpoint database
    Fp::Db* fpDatabase = mFlashpointInstall->database();

    //-Pre-loading-------------------------------------------------------------

    // Pre-load playlists
    QList<Fp::Playlist> targetPlaylists;
    Qx::Error plError = preloadPlaylists(targetPlaylists);
    if(plError.isValid())
    {
        errorReport = plError;
        return Failed;
    }

    // Make initial game query
    queryError = fpDatabase->queryGamesByPlatform(gameQueries, mImportSelections.platforms, mOptionSet.inclusionOptions);
    if(queryError.isValid())
    {
        errorReport = queryError;
        return Failed;
    }

    // Make initial playlist specific game query if applicable
    if(mOptionSet.playlistMode == PlaylistGameMode::ForceAll)
    {
        // Get playlist game ID list
        const QList<QUuid> targetPlaylistGameIds = getPlaylistSpecificGameIds(targetPlaylists);

        // Make unselected platforms list
        QStringList availablePlatforms = fpDatabase->platformNames();
        QStringList unselectedPlatforms = QStringList(availablePlatforms);
        for(const QString& selPlatform : qAsConst(mImportSelections.platforms))
            unselectedPlatforms.removeAll(selPlatform);

        // Make game query
        queryError = fpDatabase->queryGamesByPlatform(playlistSpecGameQueries, unselectedPlatforms, mOptionSet.inclusionOptions, &targetPlaylistGameIds);
        if(queryError.isValid())
        {
            errorReport = queryError;
            return Failed;
        }
    }

    // Make initial add apps query
    queryError = fpDatabase->queryAllAddApps(addAppQuery);
    if(queryError.isValid())
    {
        errorReport = queryError;
        return Failed;
    }

    // Bail if there's no work to be done
    if(gameQueries.isEmpty() && playlistSpecGameQueries.isEmpty() && targetPlaylists.isEmpty())
        return Taskless;

    //-Determine Workload-------------------------------------------------
    quint64 totalGameCount = 0;

    // Additional App pre-load
    Qx::ProgressGroup* pgAddAppPreload = initializeProgressGroup(Pg::AddAppPreload, 2);
    pgAddAppPreload->setMaximum(addAppQuery.size);

    // Initialize game query progress group since there will always be at least one game to import
    Qx::ProgressGroup* pgGameImport = initializeProgressGroup(Pg::GameImport, 2);

    // All games
    for(const Fp::Db::QueryBuffer& query : qAsConst(gameQueries))
    {
        pgGameImport->increaseMaximum(query.size);
        totalGameCount += query.size;
    }

    // All playlist specific games
    for(const Fp::Db::QueryBuffer& query : qAsConst(playlistSpecGameQueries))
    {
        pgGameImport->increaseMaximum(query.size);
        totalGameCount += query.size;
    }

    // Screenshot and Logo downloads
    if(mOptionSet.downloadImages)
    {
        Qx::ProgressGroup* pgImageDownload = initializeProgressGroup(Pg::ImageDownload, 3);
        pgImageDownload->setMaximum(totalGameCount * 2);
    }

    // Screenshot and Logo transfer
    if(mOptionSet.imageMode != Fe::ImageMode::Reference)
    {
        Qx::ProgressGroup* pgImageTransfer = initializeProgressGroup(Pg::ImageTransfer, 3);
        pgImageTransfer->setMaximum(totalGameCount * 2);
    }

    // All playlists
    if(!targetPlaylists.isEmpty())
    {
        Qx::ProgressGroup* pgPlaylistImport = initializeProgressGroup(Pg::PlaylistImport, 4);
        pgPlaylistImport->increaseMaximum(targetPlaylists.size());
    }

    // Connect progress manager signal
    connect(&mProgressManager, &Qx::GroupedProgressManager::progressUpdated, this, &ImportWorker::pmProgressUpdated);

    //-Handle Frontend Specific Import Setup------------------------------
    QStringList playlistSpecPlatforms;
    for(const Fp::Db::QueryBuffer& query : qAsConst(playlistSpecGameQueries))
        playlistSpecPlatforms.append(query.source);

    Fe::Install::ImportDetails details{
        .updateOptions = mOptionSet.updateOptions,
        .imageMode = mOptionSet.imageMode,
        .clifpPath = CLIFp::standardCLIFpPath(*mFlashpointInstall),
        .involvedPlatforms = mImportSelections.platforms + playlistSpecPlatforms,
        .involvedPlaylists = mImportSelections.playlists
    };

    errorReport = mFrontendInstall->preImport(details);
    if(errorReport.isValid())
        return Failed;

    //-Set Progress Indicator To First Step----------------------------------
    emit progressMaximumChanged(mProgressManager.maximum());
    emit progressStepChanged(STEP_ADD_APP_PRELOAD);

    //-Primary Import Stages-------------------------------------------------

    // Pre-load additional apps
    if((importStepStatus = preloadAddApps(errorReport, addAppQuery)) != Successful)
        return importStepStatus;

    // Handle Frontend specific pre-platform tasks
    errorReport = mFrontendInstall->prePlatformsImport();
    if(errorReport.isValid())
        return Failed;

    // Process games and additional apps by platform (primary and playlist specific)
    if((importStepStatus = processGames(errorReport, gameQueries, playlistSpecGameQueries)) != Successful)
        return importStepStatus;

    // Handle Frontend specific post-platform tasks
    errorReport = mFrontendInstall->postPlatformsImport();
    if(errorReport.isValid())
        return Failed;

    // Process images
    if((importStepStatus = processImages(errorReport)) != Successful)
        return importStepStatus;

    // Process playlists
    if(!targetPlaylists.isEmpty())
    {
        // Remove un-imported games from playlists
        cullUnimportedPlaylistGames(targetPlaylists);

        // Handle Frontend specific pre-playlist tasks
        errorReport = mFrontendInstall->prePlaylistsImport();
        if(errorReport.isValid())
            return Failed;

        if((importStepStatus = processPlaylists(errorReport, targetPlaylists)) != Successful)
            return importStepStatus;

        // Handle Frontend specific pre-playlist tasks
        errorReport = mFrontendInstall->postPlaylistsImport();
        if(errorReport.isValid())
            return Failed;
    }

    // Handle Frontend specific cleanup
    emit progressStepChanged(STEP_FINALIZING);
    errorReport = mFrontendInstall->postImport();
    if(errorReport.isValid())
        return Failed;

    // Check for final cancellation
    if(mCanceled)
    {
        errorReport = Qx::Error();
        return Canceled;
    }

    // Reset install
    mFrontendInstall->softReset();

    // Report successful import completion
    errorReport = Qx::Error();
    return Successful;
}

//-Slots---------------------------------------------------------------------------------------------------------
//Private Slots:
void ImportWorker::pmProgressUpdated(quint64 currentProgress)
{
    /* NOTE: This is required because if the value isn't actually different than the current when
     * the connected QProgressDialog::setValue() is triggered then processEvents() won't be called.
     * This is a problem because the fixed range of QGroupedProgressManager of 0-100 means that groups
     * with a high number of steps won't actually trigger an emissions of the manager valueChanged() signal
     * until a large enough number of those steps have been completed to increase its weighted sum by 1.
     * processEvents() needs to be called every time progress is updated even a little in order to
     * keep the progress dialog responsive.
     *
     * This needs to be removed if ImportWorker is ever returned to its own thread.
     */
    if(mCurrentProgress != currentProgress)
    {
        mCurrentProgress = currentProgress;
        emit progressValueChanged(currentProgress);
    }
    else
        qApp->processEvents();
}

//Public Slots:
void ImportWorker::notifyCanceled() { mCanceled = true; }
