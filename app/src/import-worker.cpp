// Unit Include
#include "import-worker.h"

// Standard Library Includes
#include <filesystem>

// Qx Includes
#include <qx/core/qx-regularexpression.h>

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

const QList<QUuid> ImportWorker::preloadPlaylists(Fp::Db::QueryBuffer& playlistQuery)
{
    QList<QUuid> targetPlaylistIds;

    for(int i = 0; i < playlistQuery.size; i++)
    {
        // Advance to next record
        playlistQuery.result.next();

        // Form playlist from record
        Fp::Playlist::Builder fpPb;
        fpPb.wId(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_ID).toString());
        fpPb.wTitle(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_TITLE).toString());
        fpPb.wDescription(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_DESCRIPTION).toString());
        fpPb.wAuthor(playlistQuery.result.value(Fp::Db::Table_Playlist::COL_AUTHOR).toString());

        // Build playlist
        Fp::Playlist playlist = fpPb.build();

        // Add to cache
        mPlaylistsCache[playlist.id()] = playlist;

        // Add to ID list
        targetPlaylistIds.append(playlist.id());
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
        else if(!destinationInfo.isSymLink() && !symlink)
        {
            QFile source(sourcePath);
            QFile destination(destinationPath);
            QString sourceChecksum;
            QString destinationChecksum;

            if(!Qx::calculateFileChecksum(sourceChecksum, source, QCryptographicHash::Md5).isFailure() &&
               !Qx::calculateFileChecksum(destinationChecksum, destination, QCryptographicHash::Md5).isFailure() &&
               sourceChecksum.compare(destinationChecksum, Qt::CaseInsensitive) == 0)
                return Qx::GenericError();
        }
        // Image is always updated if changing between Link/Copy
    }

    // Ensure destination path exists
    if(!destinationDir.mkpath("."))
        return Qx::GenericError(Qx::GenericError::Error, ERR_CANT_MAKE_DIR.arg(destinationDir.absolutePath()), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);

    // Determine backup path
    QString backupPath = Fe::Install::filePathToBackupPath(destinationInfo.absoluteFilePath());

    // Temporarily backup image if it already exists (also acts as deletion marking in case images for the title were removed in an update)
    if(destinationOccupied)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_BACKUP.arg(destinationPath), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer
    if(symlink)
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
            mFrontendInstall->addRevertableFile(destinationPath); // Only queue image to be removed on failure if its new, so existing images aren't deleted on revert
    }
    else
    {
        if(!QFile::copy(sourcePath, destinationPath))
        {
            QFile::rename(backupPath, destinationPath); // Restore Backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_COPY.arg(sourcePath, destinationPath), IMAGE_RETRY_PROMPT, QString(), CAPTION_IMAGE_ERR);
        }
        else if(QFile::exists(backupPath))
            QFile::remove(backupPath);
        else
            mFrontendInstall->addRevertableFile(destinationPath); // Only queue image to be removed on failure if its new, so existing images aren't deleted on revert
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
        Fp::Game::Builder fpGb;
        fpGb.wId(gameQueryResult.result.value(Fp::Db::Table_Game::COL_ID).toString());
        fpGb.wTitle(gameQueryResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wSeries(gameQueryResult.result.value(Fp::Db::Table_Game::COL_SERIES).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wDeveloper(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wPublisher(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString().remove(Qx::RegularExpression::LINE_BREAKS));
        fpGb.wDateAdded(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DATE_ADDED).toString());
        fpGb.wDateModified(gameQueryResult.result.value(Fp::Db::Table_Game::COL_DATE_MODIFIED).toString());
        fpGb.wPlatform(gameQueryResult.result.value(Fp::Db::Table_Game::COL_PLATFORM).toString());
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
            errorReport = Qx::GenericError();
            return Canceled;
        }
        else
            mProgressManager.group(Pg::GameImport)->incrementValue();
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
           errorReport = Qx::GenericError();
           return Canceled;
        }
        else
            mProgressManager.group(Pg::AddAppPreload)->incrementValue();
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

    auto platformsHandler = [&remainingPlatforms, &errorReport, this](QList<Fp::Db::QueryBuffer>& platformQueryResults, QString label) -> ImportResult {
        ImportResult result;

        for(int i = 0; i < platformQueryResults.size(); i++)
        {
            Fp::Db::QueryBuffer& currentQueryResult = platformQueryResults[i];

            // Open frontend platform doc
            std::unique_ptr<Fe::PlatformDoc> currentPlatformDoc;
            Qx::GenericError platformReadError = mFrontendInstall->checkoutPlatformDoc(currentPlatformDoc, currentQueryResult.source);

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
            Qx::GenericError saveError;
            if((saveError = mFrontendInstall->commitPlatformDoc(std::move(currentPlatformDoc))).isValid())
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
    if((platformImportStatus = platformsHandler(primary, STEP_IMPORTING_PLATFORM_SETS)) != Successful)
        return platformImportStatus;

    // Import playlist specific platforms
    if((platformImportStatus = platformsHandler(playlistSpecific, STEP_IMPORTING_PLAYLIST_SPEC_SETS)) != Successful)
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
        emit progressStepChanged(STEP_IMPORTING_PLAYLIST_GAMES.arg(currentPlaylist.title()));

        // Open frontend playlist doc
        std::unique_ptr<Fe::PlaylistDoc> currentPlaylistDoc;
        Qx::GenericError playlistReadError = mFrontendInstall->checkoutPlaylistDoc(currentPlaylistDoc, currentPlaylist.title());

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
                Fp::PlaylistGame::Builder fpPgb;
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
                mProgressManager.group(Pg::PlaylistGameMatchImport)->incrementValue();
        }

        // Finalize document
        currentPlaylistDoc->finalize();

        // Check for internal doc errors
        if(currentPlaylistDoc->hasError())
        {
            errorReport = currentPlaylistDoc->error();
            return Failed;
        }

        // Forfeit document lease and save it
        Qx::GenericError saveError;
        if((saveError = mFrontendInstall->commitPlaylistDoc(std::move(currentPlaylistDoc))).isValid())
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
        connect(&mImageDownloadManager, &Qx::SyncDownloadManager::sslErrors, this, [&](Qx::GenericError errorMsg, bool* ignore) {
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
            emit blockingErrorOccured(mBlockingErrorResponse, downloadReport.errorInfo(), QMessageBox::Abort | QMessageBox::Ignore);

            // Check response
            if(*mBlockingErrorResponse == QMessageBox::Abort)
            {
                errorReport = Qx::GenericError();
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

    Qx::GenericError imageExchangeError = mFrontendInstall->preImageProcessing(imageTransferJobs, bulkSources);

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
        if(imageTransferJobs.size() != mProgressManager.group(Pg::ImageTransfer)->maximum())
            mProgressManager.group(Pg::ImageTransfer)->setMaximum(imageTransferJobs.size());

        // Setup for image transfers
        Qx::GenericError imageTransferError; // Error return reference
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
               errorReport = Qx::GenericError();
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

    // Get flashpoint database
    Fp::Db* fpDatabase = mFlashpointInstall->database();

    //-Pre-loading-------------------------------------------------------------

    // Make initial playlists query
    queryError = fpDatabase->queryPlaylistsByName(playlistQueries, mImportSelections.playlists, mOptionSet.inclusionOptions);
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
        queryError = fpDatabase->queryGamesByPlatform(playlistSpecGameQueries, unselectedPlatforms, mOptionSet.inclusionOptions, &targetPlaylistGameIds);
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

    // Bail if there's no work to be done
    if(gameQueries.isEmpty() && playlistSpecGameQueries.isEmpty() && playlistGameQueries.isEmpty())
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

    // All playlist games
    if(!playlistGameQueries.isEmpty())
    {
        Qx::ProgressGroup* pgPlaylistGameMatchImport = initializeProgressGroup(Pg::PlaylistGameMatchImport, 2);
        for(const Fp::Db::QueryBuffer& query : qAsConst(playlistGameQueries))
            pgPlaylistGameMatchImport->increaseMaximum(query.size);
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
    if((importStepStatus = processImages(errorReport, playlistSpecGameQueries)) != Successful)
        return importStepStatus;

    // Process playlists
    if(!playlistGameQueries.isEmpty())
    {
        // Handle Frontend specific pre-playlist tasks
        errorReport = mFrontendInstall->prePlaylistsImport();
        if(errorReport.isValid())
            return Failed;

        if((importStepStatus = processPlaylists(errorReport, playlistGameQueries)) != Successful)
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