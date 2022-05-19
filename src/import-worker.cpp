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
{}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private
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
            emit progressValueChanged(++mCurrentProgressValue);
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

Qx::GenericError ImportWorker::transferImage(bool symlink, QString sourcePath, QString destinationPath)
{
    // Image info
    QFileInfo destinationInfo(destinationPath);
    QFileInfo sourceInfo(sourcePath);
    QDir destinationDir(destinationInfo.absolutePath());
    bool destinationOccupied = destinationInfo.exists() && (destinationInfo.isFile() || destinationInfo.isSymLink());
    bool sourceAvailable = sourceInfo.exists();

    // Return if image is already up-to-date
    if(sourceAvailable && destinationOccupied)
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
        return Qx::GenericError(Qx::GenericError::Error, ERR_CANT_MAKE_DIR, destinationDir.absolutePath(), QString(), CAPTION_IMAGE_ERR);

    // Determine backup path
    QString backupPath = destinationInfo.absolutePath() + '/' + destinationInfo.baseName() + Fe::Install::MODIFIED_FILE_EXT;

    // Temporarily backup image if it already exists (also acts as deletion marking in case images for the title were removed in an update)
    if(destinationOccupied && sourceAvailable)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_BACKUP, destinationPath, QString(), CAPTION_IMAGE_ERR);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer if source is available
    if(sourceAvailable)
    {
        if(symlink)
        {
            if(!QFile::copy(sourcePath, destinationPath))
            {
                QFile::rename(backupPath, destinationPath); // Restore Backup
                return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_COPY.arg(sourcePath), destinationPath, QString(), CAPTION_IMAGE_ERR);
            }
            else if(QFile::exists(backupPath))
                QFile::remove(backupPath);
            else
                mFrontendInstall->addPurgeableImagePath(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
        }
        else
        {
            std::filesystem::create_symlink(sourcePath.toStdString(), destinationPath.toStdString(), linkError);
            if(linkError)
            {
                QFile::rename(backupPath, destinationPath); // Restore Backup
                return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_LINK.arg(sourcePath), destinationPath, QString(), CAPTION_IMAGE_ERR);
            }
            else if(QFile::exists(backupPath))
                QFile::remove(backupPath);
            else
                mFrontendInstall->addPurgeableImagePath(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
        }
    }

    // Return null error on success
    return Qx::GenericError();
}

ImportWorker::ImportResult ImportWorker::processGames(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& gameQueries, bool playlistSpecific)
{
    for(int i = 0; i < gameQueries.size(); i++)
    {
        // Get current result
        Fp::Db::QueryBuffer& currentPlatformGameResult = gameQueries[i];

        // Update progress dialog label
        emit progressStepChanged((playlistSpecific ? STEP_IMPORTING_PLAYLIST_SPEC_GAMES : STEP_IMPORTING_PLATFORM_GAMES).arg(currentPlatformGameResult.source));

        // Open frontend platform doc
        std::unique_ptr<Fe::PlatformDoc> currentPlatformDoc;
        Qx::GenericError platformReadError = mFrontendInstall->openPlatformDoc(currentPlatformDoc, currentPlatformGameResult.source, mOptionSet.updateOptions);

        // Stop import if error occured
        if(platformReadError.isValid())
        {
            // Emit import failure
            errorReport = platformReadError;
            return Failed;
        }

        // Add/Update games
        for(int j = 0; j < currentPlatformGameResult.size; j++)
        {
            // Advance to next record
            currentPlatformGameResult.result.next();

            // Form game from record
            Fp::GameBuilder fpGb;
            fpGb.wId(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_ID).toString());
            fpGb.wTitle(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString());
            fpGb.wSeries(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_SERIES).toString());
            fpGb.wDeveloper(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString());
            fpGb.wPublisher(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString());
            fpGb.wDateAdded(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_DATE_ADDED).toString());
            fpGb.wDateModified(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_DATE_MODIFIED).toString());
            fpGb.wPlatform(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_PLATFORM).toString());
            fpGb.wBroken(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_BROKEN).toString());
            fpGb.wPlayMode(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_PLAY_MODE).toString());
            fpGb.wStatus(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_STATUS).toString());
            fpGb.wNotes(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_NOTES).toString());
            fpGb.wSource(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_SOURCE).toString());
            fpGb.wAppPath(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_APP_PATH).toString());
            fpGb.wLaunchCommand(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_LAUNCH_COMMAND).toString());
            fpGb.wReleaseDate(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_RELEASE_DATE).toString());
            fpGb.wVersion(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_VERSION).toString());
            fpGb.wOriginalDescription(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_ORIGINAL_DESC).toString());
            fpGb.wLanguage(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_LANGUAGE).toString());
            fpGb.wOrderTitle(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_ORDER_TITLE).toString());
            fpGb.wLibrary(currentPlatformGameResult.result.value(Fp::Db::Table_Game::COL_LIBRARY).toString());

            // Build FP game and add to document
            Fp::Game builtGame = fpGb.build();
            const Fe::Game* addedGame = currentPlatformDoc->addGame(builtGame);

            // Add ID to imported game cache
            mImportedGameIdsCache.insert(addedGame->getId());

            // Get image information
            QString logoLocalPath = mFlashpointInstall->imageLocalPath(Fp::ImageType::Logo, addedGame->getId());
            QString ssLocalPath =  mFlashpointInstall->imageLocalPath(Fp::ImageType::Screenshot, addedGame->getId());

            // Setup image downloads if applicable
            if(mOptionSet.downloadImages)
            {
                if(!QFileInfo::exists(logoLocalPath))
                {
                    QUrl logoRemotePath = mFlashpointInstall->imageRemoteUrl(Fp::ImageType::Logo, addedGame->getId());
                    mImageDownloadManager.appendTask(Qx::DownloadTask{logoRemotePath, logoLocalPath});
                }
                else
                    emit progressMaximumChanged(--mMaximumProgressValue); // Already exists, remove download step from progress bar

                if(!QFileInfo::exists(ssLocalPath))
                {
                    QUrl ssRemotePath = mFlashpointInstall->imageRemoteUrl(Fp::ImageType::Screenshot, addedGame->getId());
                    mImageDownloadManager.appendTask(Qx::DownloadTask{ssRemotePath, logoLocalPath});
                }
                else
                    emit progressMaximumChanged(--mMaximumProgressValue); // Already exists, remove download step from progress bar
            }

            // Perform immediate image handling if applicable
            if(mOptionSet.imageMode == Fe::Install::Reference)
            {
                if(mFrontendInstall->imageRefType() == Fe::Install::ImageRefType::Single)
                {
                    mFrontendInstall->referenceImage(Fp::ImageType::Logo, logoLocalPath, *addedGame);
                    mFrontendInstall->referenceImage(Fp::ImageType::Screenshot, ssLocalPath, *addedGame);
                }
                else if(mFrontendInstall->imageRefType() == Fe::Install::ImageRefType::Platform)
                {
                    currentPlatformDoc->setGameImageReference(Fp::ImageType::Logo, addedGame->getId(), logoLocalPath);
                    currentPlatformDoc->setGameImageReference(Fp::ImageType::Screenshot, addedGame->getId(), ssLocalPath);
                }
            }
            else // Setup transfer tasks if applicable
            {
                QString logoDestPath = mFrontendInstall->imageDestinationPath(Fp::ImageType::Logo, *addedGame);
                QString ssDestPath = mFrontendInstall->imageDestinationPath(Fp::ImageType::Screenshot, *addedGame);
                mImageTransferJobs.append(ImageTransferJob{logoLocalPath, logoDestPath});
                mImageTransferJobs.append(ImageTransferJob{ssLocalPath, ssDestPath});
            }

            // Update progress dialog value for game addition
            if(mCanceled)
            {
                errorReport = Qx::GenericError();
                return Canceled;
            }
            else
                emit progressValueChanged(++mCurrentProgressValue);
        }

        // Update progress dialog label
        emit progressStepChanged((playlistSpecific ? STEP_IMPORTING_PLAYLIST_SPEC_ADD_APPS : STEP_IMPORTING_PLATFORM_ADD_APPS).arg(currentPlatformGameResult.source));

        // Add applicable additional apps
        for (QSet<Fp::AddApp>::iterator j = mAddAppsCache.begin(); j != mAddAppsCache.end();)
        {
            // If the current platform doc contains the game this add app belongs to, convert and add it, then remove it from cache
            if (currentPlatformDoc->containsGame((*j).getParentId()))
            {
               currentPlatformDoc->addAddApp(*j);
               j = mAddAppsCache.erase(j); // clazy:exclude=strict-iterators Oversight of clazy since there's no QSet::erase(QSet::const_iterator) anymore

               // Reduce progress dialog maximum by total iterations cut from future platforms
               mMaximumProgressValue = mMaximumProgressValue - gameQueries.size() + i + 1; // Max = Max - (size - (i + 1))
               emit progressMaximumChanged(mMaximumProgressValue);
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
                emit progressValueChanged(++mCurrentProgressValue);
        }

        // Finalize document
        currentPlatformDoc->finalize();

        // Forefit doucment lease and save it
        Qx::GenericError saveError;
        if((saveError = mFrontendInstall->savePlatformDoc(std::move(currentPlatformDoc))).isValid())
        {
            errorReport = saveError;
            return Failed;
        }

    }

    // Report successful step completion
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

        // Stop import if error occured
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
                emit progressValueChanged(++mCurrentProgressValue);
        }

        // Finalize document
        currentPlaylistDoc->finalize();

        // Forefit doucment lease and save it
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
        mImageDownloadManager.setAutoAbort(false); // Get as many images as possible;

        // Download progress tracker
        int lastTaskCount = mImageDownloadManager.taskCount();

        // Make connections
        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::sslErrors, this, [&](Qx::GenericError errorMsg, bool* abort) {
            emit blockingErrorOccured(mBlockingErrorResponse, errorMsg, QMessageBox::Yes | QMessageBox::Abort);
            *abort = *mBlockingErrorResponse == QMessageBox::Abort;
        });

        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::authenticationRequired, this, &ImportWorker::authenticationRequired);

        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::downloadProgress, this, [this, &lastTaskCount](qint64 bytesCurrent) { // clazy:exclude=lambda-in-connect
            Q_UNUSED(bytesCurrent); // Instead, track progress via tasks instead of bytes

            int currentTaskCount = mImageDownloadManager.taskCount();
            if(currentTaskCount < lastTaskCount)
            {
                mCurrentProgressValue += lastTaskCount - currentTaskCount;
                lastTaskCount = currentTaskCount;
                emit progressValueChanged(mCurrentProgressValue);
            }
        });

        // Start download
        Qx::SyncDownloadManager::Report downloadReport = mImageDownloadManager.processQueue();

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

        // Create playlist pecific platforms set
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
        *mBlockingErrorResponse = QMessageBox::NoToAll; // Default to choice "NoToAll" incase the signal is not correctly connected using Qt::BlockingQueuedConnection
        bool skipAllImages = false; // NoToAll response tracker

        for(const ImageTransferJob& imageJob : qAsConst(mImageTransferJobs))
        {
            while(!skipAllImages && (imageTransferError = transferImage(mOptionSet.imageMode == Fe::Install::Link, imageJob.sourcePath, imageJob.destPath)).isValid())
            {
                // Notify GUI Thread of error
                emit blockingErrorOccured(mBlockingErrorResponse, imageTransferError,
                                          QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::NoToAll);

                // Check response
                if(*mBlockingErrorResponse == QMessageBox::Ignore)
                   break;
                else if(*mBlockingErrorResponse == QMessageBox::NoToAll)
                   skipAllImages = true;
            }

           // Update progress dialog value
           if(mCanceled)
           {
               errorReport = Qx::GenericError();
               return Canceled;
           }
           else
               emit progressValueChanged(++mCurrentProgressValue);
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

    // Link Clifp to frontend
    mFrontendInstall->linkClifpPath(CLIFp::standardCLIFpPath(*mFlashpointInstall));

    // Get flashpoint database
    Fp::Db* fpDatabase = mFlashpointInstall->database();

    //-Preloading-------------------------------------------------------------

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
    mCurrentProgressValue = 0;
    int totalGameCount = 0;

    // Additional App pre-load
    mMaximumProgressValue = addAppQuery.size;

    // All games
    for(const Fp::Db::QueryBuffer& query : qAsConst(gameQueries))
    {
        mMaximumProgressValue += query.size;
        totalGameCount += query.size;
    }

    // All playlist specific games
    for(const Fp::Db::QueryBuffer& query : qAsConst(playlistSpecGameQueries))
    {
        mMaximumProgressValue += query.size;
        totalGameCount += query.size;
    }

    // Screenshot and Logo downloads
    if(mOptionSet.downloadImages)
        mMaximumProgressValue += totalGameCount * 2;

    // Screenshot and Logo transfer
    if(mOptionSet.imageMode != Fe::Install::ImageMode::Reference)
        mMaximumProgressValue += totalGameCount * 2;

    // All playlist games
    for(const Fp::Db::QueryBuffer& query : qAsConst(playlistGameQueries))
        mMaximumProgressValue += query.size;

    // All checks of Additional Apps
    mMaximumProgressValue += addAppQuery.size * gameQueries.size() + addAppQuery.size * playlistSpecGameQueries.size();

    // Re-prep progress dialog
    emit progressMaximumChanged(mMaximumProgressValue);
    emit progressStepChanged(STEP_ADD_APP_PRELOAD);

    //-Primary Import Stages-------------------------------------------------

    // Pre-load additional apps
    if((importStepStatus = preloadAddApps(errorReport, addAppQuery)) != Successful)
        return importStepStatus;

    // Process games and additional apps by platform
    if((importStepStatus = processGames(errorReport, gameQueries, false)) != Successful)
        return importStepStatus;

    // Process playlist specific games and additional apps by platform
    if((importStepStatus = processGames(errorReport, playlistSpecGameQueries, true)) != Successful)
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
