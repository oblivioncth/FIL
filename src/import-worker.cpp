// Unit Include
#include "import-worker.h"

// Standard Library Includes
#include <filesystem>

// Project Includes
#include "clifp.h"

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
      mOptionSet(optionSet)
{
    // Initialize progress trackers
    initializeProgressTrackerGroup(ProgressGroup::AddAppPreload, 2);
    initializeProgressTrackerGroup(ProgressGroup::AddAppMatchImport, 1);
    initializeProgressTrackerGroup(ProgressGroup::ImageDownload, 2);
    initializeProgressTrackerGroup(ProgressGroup::ImageTransfer, 2);
    initializeProgressTrackerGroup(ProgressGroup::GameImport, 2);
    initializeProgressTrackerGroup(ProgressGroup::PlaylistGameMatchImport, 2);
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
void ImportWorker::initializeProgressTrackerGroup(ProgressGroup group, quint64 scaler)
{
    mCurrentProgress.insert(group, 0, scaler);
    mTotalProgress.insert(group, 0, scaler);
}

const QList<QUuid> ImportWorker::preloadPlaylists(Fp::Db::QueryBuffer& playlistQuery)
{
    QList<QUuid> targetPlaylistIds;

    for(int i = 0; i < playlistQuery.size; i++)
    {
        // Advance to next record
        playlistQuery.result.next();

        // Form playlist from record
        Fp::PlaylistBuilder fpPb;
        fpPb.wId(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_ID).toString());
        fpPb.wTitle(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_TITLE).toString());
        fpPb.wDescription(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_DESCRIPTION).toString());
        fpPb.wAuthor(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_AUTHOR).toString());

        // Build playlist
        Fp::Playlist playlist = fpPb.build();

        // Add to cache
        mPlaylistsCache[playlist.getId()] = playlist;

        // Add to ID list
        targetPlaylistIds.append(playlist.getId());
    }

    return targetPlaylistIds;
}

const QList<QUuid> ImportWorker::getPlaylistSpecificGameIds(Fp::Db::QueryBuffer& playlistGameIdQuery)
{
    QList<QUuid> playlistSpecGameIds;

    for(int i = 0; i < playlistGameIdQuery.size; i++)
    {
        // Advance to next record
        playlistGameIdQuery.result.next();

        // Add ID to list
        playlistSpecGameIds.append(QUuid(playlistGameIdQuery.result.value(Fp::Db::Table_Playlist_Game::COL_GAME_ID).toString()));
    }

    return playlistSpecGameIds;
}

Qx::GenericError ImportWorker::transferImage(bool symlink, QString sourcePath, QString destinationPath)
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
        return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_SRC_UNAVAILABLE.arg(sourcePath), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);

    // Return if image is already up-to-date
    if(destinationOccupied)
    {
        if(destinationInfo.isSymLink() && symlink)
            return Qx::GenericError();
        else
        {
            QFile source(sourcePath);
            QFile destination(destinationPath);
            QString sourceChecksum;
            QString destinationChecksum;

            if(Qx::calculateFileChecksum(sourceChecksum, source, QCryptographicHash::Md5).wasSuccessful() &&
               Qx::calculateFileChecksum(destinationChecksum, destination, QCryptographicHash::Md5).wasSuccessful() &&
               sourceChecksum.compare(destinationChecksum, Qt::CaseInsensitive) == 0)
                return Qx::GenericError();
        }
    }

    // Ensure destination path exists
    if(!destinationDir.mkpath("."))
        return Qx::GenericError(Qx::GenericError::Error, ERR_CANT_MAKE_DIR.arg(destinationDir.absolutePath()), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);

    // Determine backup path
    QString backupPath = destinationInfo.absolutePath() + '/' + destinationInfo.baseName() + Fe::Install::MODIFIED_FILE_EXT;

    // Temporarily backup image if it already exists (also acts as deletion marking in case images for the title were removed in an update)
    if(destinationOccupied)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_BACKUP.arg(destinationPath), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer
    if(symlink)
    {
        if(!QFile::copy(sourcePath, destinationPath))
        {
            QFile::rename(backupPath, destinationPath); // Restore Backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_COPY.arg(sourcePath, destinationPath), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);
        }
        else if(QFile::exists(backupPath))
            QFile::remove(backupPath);
        else
            mFrontendInstall->addPurgeableImagePath(destinationPath); // Only queue image to be removed on failure if its new, so existing images aren't deleted on revert
    }
    else
    {
        std::filesystem::create_symlink(sourcePath.toStdString(), destinationPath.toStdString(), linkError);
        if(linkError)
        {
            QFile::rename(backupPath, destinationPath); // Restore Backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_LINK.arg(sourcePath, destinationPath), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);
        }
        else if(QFile::exists(backupPath))
            QFile::remove(backupPath);
        else
            mFrontendInstall->addPurgeableImagePath(destinationPath); // Only queue image to be removed on failure if its new, so existing images aren't deleted on revert
    }

    // Return null error on success
    return Qx::GenericError();
}

ImportWorker::ImportResult ImportWorker::processPlatformGames(Qx::GenericError& errorReport, std::unique_ptr<Fe::PlatformDoc>& platformDoc, Fp::Db::QueryBuffer& gameQueryResult)
{
    // Add/Update games
    for(int j = 0; j < gameQueryResult.size; j++)
    {
        // Advance to next record
        gameQueryResult.result.next();

        // Form game from record
        Fp::GameBuilder fpGb;
        fpGb.wId(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ID).toString());
        fpGb.wTitle(gameQueryResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString());
        fpGb.wSeries(gameQueryResult.result.value(Fp::Db::Table_Game::COL_SERIES).toString());
        fpGb.wDeveloper(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString());
        fpGb.wPublisher(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString());
        fpGb.wDateAdded(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DATE_ADDED).toString());
        fpGb.wDateModified(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DATE_MODIFIED).toString());
        fpGb.wPlatform(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PLATFORM).toString());
        fpGb.wBroken(gameQueryResult.result.value(Fp::Db::Table_Game::COL_BROKEN).toString());
        fpGb.wPlayMode(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PLAY_MODE).toString());
        fpGb.wStatus(gameQueryResult.result.value(Fp::Db::Table_Game::COL_STATUS).toString());
        fpGb.wNotes(gameQueryResult.result.value(Fp::Db::Table_Game::COL_NOTES).toString());
        fpGb.wSource(gameQueryResult.result.value(Fp::Db::Table_Game::COL_SOURCE).toString());
        fpGb.wAppPath(gameQueryResult.result.value(Fp::Db::Table_Game::COL_APP_PATH).toString());
        fpGb.wLaunchCommand(gameQueryResult.result.value(Fp::Db::Table_Game::COL_LAUNCH_COMMAND).toString());
        fpGb.wReleaseDate(gameQueryResult.result.value(Fp::Db::Table_Game::COL_RELEASE_DATE).toString());
        fpGb.wVersion(gameQueryResult.result.value(Fp::Db::Table_Game::COL_VERSION).toString());
        fpGb.wOriginalDescription(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ORIGINAL_DESC).toString());
        fpGb.wLanguage(gameQueryResult.result.value(Fp::Db::Table_Game::COL_LANGUAGE).toString());
        fpGb.wOrderTitle(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ORDER_TITLE).toString());
        fpGb.wLibrary(gameQueryResult.result.value(Fp::Db::Table_Game::COL_LIBRARY).toString());

        // Build FP game and add to document
        Fp::Game builtGame = fpGb.build();
        const Fe::Game* addedGame = platformDoc->addGame(builtGame);

        // Add ID to imported game cache
        mImportedGameIdsCache.insert(addedGame->getId());

        // Get image information
        QFileInfo logoLocalInfo(mFlashpointInstall->imageLocalPath(Fp::ImageType::Logo, addedGame->getId()));
        QFileInfo ssLocalInfo(mFlashpointInstall->imageLocalPath(Fp::ImageType::Screenshot, addedGame->getId()));

        // Setup image downloads if applicable
        if(mOptionSet.downloadImages)
        {
            if(!logoLocalInfo.exists())
            {
                QUrl logoRemotePath = mFlashpointInstall->imageRemoteUrl(Fp::ImageType::Logo, addedGame->getId());
                mImageDownloadManager.appendTask(Qx::DownloadTask{logoRemotePath, logoLocalInfo.path()});
            }
            else
                emit progressMaximumChanged(mTotalProgress.decrement(ProgressGroup::ImageDownload)); // Already exists, remove download step from progress bar

            if(!ssLocalInfo.exists())
            {
                QUrl ssRemotePath = mFlashpointInstall->imageRemoteUrl(Fp::ImageType::Screenshot, addedGame->getId());
                mImageDownloadManager.appendTask(Qx::DownloadTask{ssRemotePath, ssLocalInfo.path()});
            }
            else
                emit progressMaximumChanged(mTotalProgress.decrement(ProgressGroup::ImageDownload)); // Already exists, remove download step from progress bar
        }

        // Perform immediate image handling if applicable
        if(mOptionSet.imageMode == Fe::Install::Reference)
        {
            if(mFrontendInstall->imageRefType() == Fe::Install::ImageRefType::Single)
            {
                mFrontendInstall->referenceImage(Fp::ImageType::Logo, logoLocalInfo.path(), *addedGame);
                mFrontendInstall->referenceImage(Fp::ImageType::Screenshot, ssLocalInfo.path(), *addedGame);
            }
            else if(mFrontendInstall->imageRefType() == Fe::Install::ImageRefType::Platform)
            {
                platformDoc->setGameImageReference(Fp::ImageType::Logo, addedGame->getId(), logoLocalInfo.path());
                platformDoc->setGameImageReference(Fp::ImageType::Screenshot, addedGame->getId(), ssLocalInfo.path());
            }
        }
        else // Setup transfer tasks if applicable
        {
            if(logoLocalInfo.exists() || mOptionSet.downloadImages)
            {
                QString logoDestPath = mFrontendInstall->imageDestinationPath(Fp::ImageType::Logo, *addedGame);
                mImageTransferJobs.append(ImageTransferJob{logoLocalInfo.path(), logoDestPath});
            }
            else
                mTotalProgress.decrement(ProgressGroup::ImageTransfer); // Can't transfer image that doesn't/won't exist

            if(ssLocalInfo.exists() || mOptionSet.downloadImages)
            {
                QString ssDestPath = mFrontendInstall->imageDestinationPath(Fp::ImageType::Screenshot, *addedGame);
                mImageTransferJobs.append(ImageTransferJob{ssLocalInfo.path(), ssDestPath});
            }
            else
                mTotalProgress.decrement(ProgressGroup::ImageTransfer);
        }

        // Update progress dialog value for game addition
        if(mCanceled)
        {
            errorReport = Qx::GenericError();
            return Canceled;
        }
        else
            emit progressValueChanged(mCurrentProgress.increment(GameImport));
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processPlatformAddApps(Qx::GenericError& errorReport, std::unique_ptr<Fe::PlatformDoc>& platformDoc)
{
    // Add applicable additional apps
    for (QSet<Fp::AddApp>::iterator j = mAddAppsCache.begin(); j != mAddAppsCache.end();)
    {
        // If the current platform doc contains the game this add app belongs to, convert and add it, then remove it from cache
        if (platformDoc->containsGame((*j).getParentId()))
        {
           platformDoc->addAddApp(*j);
           j = mAddAppsCache.erase(j); // clazy:exclude=strict-iterators Oversight of clazy since there's no QSet::erase(QSet::const_iterator) anymore
        }
        else
           ++j;

        // Update progress dialog value
        if(mCanceled)
        {
            errorReport = Qx::GenericError();
            return Canceled;
        }
        else
            emit progressValueChanged(mCurrentProgress.increment(ProgressGroup::AddAppMatchImport));
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::preloadAddApps(Qx::GenericError& errorReport, Fp::Db::QueryBuffer& addAppQuery)
{
    mAddAppsCache.reserve(addAppQuery.size);
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Advance to next record
        addAppQuery.result.next();

        // Form additional app from record
        Fp::AddAppBuilder fpAab;
        fpAab.wId(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_ID).toString());
        fpAab.wAppPath(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_APP_PATH).toString());
        fpAab.wAutorunBefore(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_AUTORUN).toString());
        fpAab.wLaunchCommand(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_LAUNCH_COMMAND).toString());
        fpAab.wName(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());
        fpAab.wWaitExit(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_WAIT_EXIT).toString());
        fpAab.wParentId(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toString());

        // Build additional app
        Fp::AddApp additionalApp = fpAab.build();

        // Add to cache
        mAddAppsCache.insert(additionalApp);

        // Update progress dialog value
        if(mCanceled)
        {
           errorReport = Qx::GenericError();
           return Canceled;
        }
        else
            emit progressValueChanged(mCurrentProgress.increment(ProgressGroup::AddAppPreload));
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processGames(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& primary, QList<Fp::Db::QueryBuffer>& playlistSpecific)
{    
    // Status tracking
    ImportResult platformImportStatus;

    // Track total platforms that have been handled
    qsizetype remainingPlatforms = primary.size() + playlistSpecific.size();

    // Use lambda to handle both lists due to major overlap
    struct StepLabels
    {
        const QString& games;
        const QString& addApps;
    };

    auto platformsHandler = [&remainingPlatforms, &errorReport, this](QList<Fp::Db::QueryBuffer>& platformQueryResults, StepLabels labels) -> ImportResult {
        ImportResult result;

        for(int i = 0; i < platformQueryResults.size(); i++)
        {
            Fp::Db::QueryBuffer& currentQueryResult = platformQueryResults[i];

            // Open frontend platform doc
            std::unique_ptr<Fe::PlatformDoc> currentPlatformDoc;
            Qx::GenericError platformReadError = mFrontendInstall->openPlatformDoc(currentPlatformDoc, currentQueryResult.source, mOptionSet.updateOptions);

            // Stop import if error occurred
            if(platformReadError.isValid())
            {
                // Emit import failure
                errorReport = platformReadError;
                return Failed;
            }

            //---Import games---------------------------------------
            emit progressStepChanged(labels.games.arg(currentQueryResult.source));
            if((result = processPlatformGames(errorReport, currentPlatformDoc, currentQueryResult)) != Successful)
                return result;

            //---Import additional apps----------------------------
            emit progressStepChanged(labels.addApps.arg(currentQueryResult.source));

            // Note current cache size
            qsizetype preAddAppCacheSize = mAddAppsCache.size();

            // Import platform additional apps
            if((result = processPlatformAddApps(errorReport, currentPlatformDoc)) != Successful)
                return result;

            /*
             * Account for reduced total progress due to consumed additional apps.
             * All platforms have to scan the entire additional app cache so each following platform
             * won't need to check the additional apps that were imported by this one
             */
            qsizetype consumedAddApps = preAddAppCacheSize - mAddAppsCache.size();
            quint64 progressReduction = consumedAddApps * (remainingPlatforms - 1); // Don't count current list
            mTotalProgress.reduce(ProgressGroup::AddAppMatchImport, progressReduction);
            emit progressMaximumChanged(mTotalProgress.total());

            //---Finalize document----------------------------------
            currentPlatformDoc->finalize();

            // Forfeit document lease and save it
            Qx::GenericError saveError;
            if((saveError = mFrontendInstall->savePlatformDoc(std::move(currentPlatformDoc))).isValid())
            {
                errorReport = saveError;
                return Failed;
            }

            // Reduce remaining platform count
            remainingPlatforms--;
        }

        // Return success
        errorReport = Qx::GenericError();
        return Successful;
    };

    // Import primary platforms
    if((platformImportStatus = platformsHandler(primary, {.games = STEP_IMPORTING_PLATFORM_GAMES, .addApps = STEP_IMPORTING_PLATFORM_ADD_APPS })) != Successful)
        return platformImportStatus;

    // Import playlist specific platforms
    if((platformImportStatus = platformsHandler(playlistSpecific, {.games = STEP_IMPORTING_PLAYLIST_GAMES, .addApps = STEP_IMPORTING_PLAYLIST_SPEC_ADD_APPS })) != Successful)
        return platformImportStatus;

    // Return success
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processPlaylists(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& playlistGameQueries)
{
    for(Fp::Db::QueryBuffer& currentPlaylistGameResult : playlistGameQueries)
    {
        // Get corresponding playlist from cache
        Fp::Playlist currentPlaylist = mPlaylistsCache.value(QUuid(currentPlaylistGameResult.source));

        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLAYLIST_GAMES.arg(currentPlaylist.getTitle()));

        // Open frontend playlist doc
        std::unique_ptr<Fe::PlaylistDoc> currentPlaylistDoc;
        Qx::GenericError playlistReadError = mFrontendInstall->openPlaylistDoc(currentPlaylistDoc, currentPlaylist.getTitle(), mOptionSet.updateOptions);

        // Stop import if error occurred
        if(playlistReadError.isValid())
        {
            errorReport = playlistReadError;
            return Failed;
        }

        // Convert and set playlist header
        currentPlaylistDoc->setPlaylistHeader(currentPlaylist);

        // Add/Update playlist games
        for(int i = 0; i < currentPlaylistGameResult.size; i++)
        {
            // Advance to next record
            currentPlaylistGameResult.result.next();

            // Only process the playlist game if it was included in import
            if(mImportedGameIdsCache.contains(QUuid(currentPlaylistGameResult.result.value(Fp::Db::Table_Playlist_Game::COL_GAME_ID).toString())))
            {
                // Form game from record
                Fp::PlaylistGameBuilder fpPgb;
                fpPgb.wId(currentPlaylistGameResult.result.value(Fp::Db::Table_Playlist_Game::COL_ID).toString());
                fpPgb.wPlaylistId(currentPlaylistGameResult.result.value(Fp::Db::Table_Playlist_Game::COL_PLAYLIST_ID).toString());
                fpPgb.wOrder(currentPlaylistGameResult.result.value(Fp::Db::Table_Playlist_Game::COL_ORDER).toString());
                fpPgb.wGameId(currentPlaylistGameResult.result.value(Fp::Db::Table_Playlist_Game::COL_GAME_ID).toString());

                // Build FP playlist game and add
                currentPlaylistDoc->addPlaylistGame(fpPgb.build());
            }

            // Update progress dialog value
            if(mCanceled)
            {
                errorReport = Qx::GenericError();
                return Canceled;
            }
            else
                emit progressValueChanged(mCurrentProgress.increment(ProgressGroup::PlaylistGameMatchImport));
        }

        // Finalize document
        currentPlaylistDoc->finalize();

        // Forfeit document lease and save it
        Qx::GenericError saveError;
        if((saveError = mFrontendInstall->savePlaylistDoc(std::move(currentPlaylistDoc))).isValid())
        {
            errorReport = saveError;
            return Failed;
        }
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processImages(Qx::GenericError& errorReport, const QList<Fp::Db::QueryBuffer>& playlistSpecGameQueries)
{
    // Download images if applicable
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
        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::sslErrors, this, [&](Qx::GenericError errorMsg, bool* ignore) {
            emit blockingErrorOccured(mBlockingErrorResponse, errorMsg, QMessageBox::Yes | QMessageBox::Abort);
            *ignore = *mBlockingErrorResponse == QMessageBox::Yes;
        });

        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::authenticationRequired, this, &ImportWorker::authenticationRequired);
        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::proxyAuthenticationRequired, this, &ImportWorker::authenticationRequired);

        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::downloadFinished, this, [this]() { // clazy:exclude=lambda-in-connect
                emit progressValueChanged(mCurrentProgress.increment(ProgressGroup::ImageDownload));
        });

        // Start download
        Qx::DownloadManagerReport downloadReport = mImageDownloadManager.processQueue();

        // Handle result
        if(!downloadReport.wasSuccessful())
        {
            // Notify GUI Thread of error
            emit blockingErrorOccured(mBlockingErrorResponse, downloadReport.errorInfo(), QMessageBox::Abort | QMessageBox::Ignore);

            // Check response
            if(*mBlockingErrorResponse == QMessageBox::Abort)
            {
                errorReport = Qx::GenericError();
                return Canceled;
            }
        }
    }

    // Import images if applicable
    if(mOptionSet.imageMode == Fe::Install::Reference && mFrontendInstall->imageRefType() == Fe::Install::ImageRefType::Bulk)
    {
        // Update progress dialog label
        emit progressStepChanged(STEP_SETTING_IMAGE_REFERENCES);

        // Create playlist specific platforms set
        QStringList playlistSpecPlatforms;
        for(const Fp::Db::QueryBuffer& query : qAsConst(playlistSpecGameQueries))
            playlistSpecPlatforms.append(query.source);

        // Initiate frontend reference routine
        Qx::GenericError referenceError = mFrontendInstall->bulkReferenceImages(QDir::toNativeSeparators(mFlashpointInstall->logosDirectory().absolutePath()),
                                                                                QDir::toNativeSeparators(mFlashpointInstall->screenshotsDirectory().absolutePath()),
                                                                                mImportSelections.platforms + playlistSpecPlatforms);
        if(referenceError.isValid())
        {
            // Emit import failure
            errorReport = referenceError;
            return Failed;
        }
    }
    else if(mOptionSet.imageMode != Fe::Install::Reference)
    {
        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_IMAGES);

        // Setup for image transfers
        Qx::GenericError imageTransferError; // Error return reference
        *mBlockingErrorResponse = QMessageBox::NoToAll; // Default to choice "NoToAll" in case the signal is not correctly connected using Qt::BlockingQueuedConnection
        bool ignoreAllTransferErrors = false; // NoToAll response tracker

        for(const ImageTransferJob& imageJob : qAsConst(mImageTransferJobs))
        {
            while((imageTransferError = transferImage(mOptionSet.imageMode == Fe::Install::Link, imageJob.sourcePath, imageJob.destPath)).isValid() && !ignoreAllTransferErrors)
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
               errorReport = Qx::GenericError();
               return Canceled;
           }
           else
               emit progressValueChanged(mCurrentProgress.increment(ProgressGroup::ImageTransfer));
        }
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

//Public
ImportWorker::ImportResult ImportWorker::doImport(Qx::GenericError& errorReport)
{
    //-Setup----------------------------------------------------------------

    // Import step status
    ImportResult importStepStatus;

    // Process query status
    QSqlError queryError;

    // Initial query buffers
    QList<Fp::Db::QueryBuffer> gameQueries;
    QList<Fp::Db::QueryBuffer> playlistSpecGameQueries;
    Fp::Db::QueryBuffer addAppQuery;
    Fp::Db::QueryBuffer playlistQueries;
    QList<Fp::Db::QueryBuffer> playlistGameQueries;

    // Link CLIFp to frontend
    mFrontendInstall->linkClifpPath(CLIFp::standardCLIFpPath(*mFlashpointInstall));

    // Get flashpoint database
    Fp::Db* fpDatabase = mFlashpointInstall->database();

    //-Pre-loading-------------------------------------------------------------

    // Make initial playlists query
    queryError = fpDatabase->queryPlaylistsByName(playlistQueries, mImportSelections.playlists);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Pre-load Playlists, add to cache and create ID list
    const QList<QUuid> targetPlaylistIds = preloadPlaylists(playlistQueries);

    // Make initial game query
    queryError = fpDatabase->queryGamesByPlatform(gameQueries, mImportSelections.platforms, mOptionSet.inclusionOptions);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Make initial playlist specific game query if applicable
    if(mOptionSet.playlistMode == PlaylistGameMode::ForceAll)
    {
        Fp::Db::QueryBuffer pgIdQuery;

        // Make playlist game ID query
        queryError = fpDatabase->queryPlaylistGameIds(pgIdQuery, targetPlaylistIds);
        if(queryError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
            return Failed;
        }

        // Get playlist game ID list
        const QList<QUuid> targetPlaylistGameIds = getPlaylistSpecificGameIds(pgIdQuery);

        // Make unselected platforms list
        QStringList availablePlatforms = fpDatabase->platformList();
        QStringList unselectedPlatforms = QStringList(availablePlatforms);
        for(const QString& selPlatform : qAsConst(mImportSelections.platforms))
            unselectedPlatforms.removeAll(selPlatform);

        // Make game query
        queryError = fpDatabase->queryGamesByPlatform(playlistSpecGameQueries, unselectedPlatforms, mOptionSet.inclusionOptions, targetPlaylistGameIds);
        if(queryError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
            return Failed;
        }
    }

    // Make initial add apps query
    queryError = fpDatabase->queryAllAddApps(addAppQuery);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Make initial playlist games query
    queryError = fpDatabase->queryPlaylistGamesByPlaylist(playlistGameQueries, targetPlaylistIds);
    if(queryError.isValid())
    {
       errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
       return Failed;
    }

    //-Determine Workload-------------------------------------------------
    /* TODO: Maybe add weighting system (instead of all tasks just being a progress bar unit of 1) so that image
     * operations aren't as dominant in worst case image scenarios (i.e. all to download and transfer)
    */
    quint64 totalGameCount = 0;

    // Additional App pre-load
    mTotalProgress.setValue(ProgressGroup::AddAppPreload, addAppQuery.size);

    // All games
    for(const Fp::Db::QueryBuffer& query : qAsConst(gameQueries))
    {
        mTotalProgress.increase(ProgressGroup::GameImport, query.size);
        totalGameCount += query.size;
    }

    // All playlist specific games
    for(const Fp::Db::QueryBuffer& query : qAsConst(playlistSpecGameQueries))
    {
        mTotalProgress.increase(ProgressGroup::GameImport, query.size);
        totalGameCount += query.size;
    }

    // Screenshot and Logo downloads
    if(mOptionSet.downloadImages)
        mTotalProgress.setValue(ProgressGroup::ImageDownload, totalGameCount * 2);

    // Screenshot and Logo transfer
    if(mOptionSet.imageMode != Fe::Install::ImageMode::Reference)
        mTotalProgress.setValue(ProgressGroup::ImageTransfer, totalGameCount * 2);

    // All playlist games
    for(const Fp::Db::QueryBuffer& query : qAsConst(playlistGameQueries))
        mTotalProgress.increase(ProgressGroup::PlaylistGameMatchImport, query.size);

    // All checks of Additional Apps
    quint64 passes = addAppQuery.size * (gameQueries.size() + playlistSpecGameQueries.size());
    mTotalProgress.setValue(ProgressGroup::AddAppMatchImport, passes);

    // Re-prep progress dialog
    emit progressMaximumChanged(mTotalProgress.total());
    emit progressStepChanged(STEP_ADD_APP_PRELOAD);

    //-Primary Import Stages-------------------------------------------------

    // Pre-load additional apps
    if((importStepStatus = preloadAddApps(errorReport, addAppQuery)) != Successful)
        return importStepStatus;

    // Process games and additional apps by platform (primary and playlist specific)
    if((importStepStatus = processGames(errorReport, gameQueries, playlistSpecGameQueries)) != Successful)
        return importStepStatus;

    // Process images
    if((importStepStatus = processImages(errorReport, playlistSpecGameQueries)) != Successful)
        return importStepStatus;

    // Process playlists
    if((importStepStatus = processPlaylists(errorReport, playlistGameQueries)) != Successful)
        return importStepStatus;

    // Check for final cancellation
    if(mCanceled)
    {
        errorReport = Qx::GenericError();
        return Canceled;
    }

    // Reset install
    mFrontendInstall->softReset();

    // Report successful import completion
    errorReport = Qx::GenericError();
    return Successful;
}

//-Slots---------------------------------------------------------------------------------------------------------
//Public Slots:
void ImportWorker::notifyCanceled() { mCanceled = true; }
