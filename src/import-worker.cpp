#include "import-worker.h"
#include "clifp.h"

//===============================================================================================================
// IMPORT WORKER
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
ImportWorker::ImportWorker(std::shared_ptr<FP::Install> fpInstallForWork,
                           std::shared_ptr<Fe::Install> feInstallForWork,
                           ImportSelections importSelections,
                           OptionSet optionSet) :
      mFlashpointInstall(fpInstallForWork),
      mFrontendInstall(feInstallForWork),
      mImportSelections(importSelections),
      mOptionSet(optionSet) {}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private
const QList<QUuid> ImportWorker::preloadPlaylists(FP::DB::QueryBuffer& playlistQuery)
{
    QList<QUuid> targetPlaylistIDs;

    for(int i = 0; i < playlistQuery.size; i++)
    {
        // Advance to next record
        playlistQuery.result.next();

        // Form playlist from record
        FP::PlaylistBuilder fpPb;
        fpPb.wID(playlistQuery.result.value(FP::DB::Table_Playlist::COL_ID).toString());
        fpPb.wTitle(playlistQuery.result.value(FP::DB::Table_Playlist::COL_TITLE).toString());
        fpPb.wDescription(playlistQuery.result.value(FP::DB::Table_Playlist::COL_DESCRIPTION).toString());
        fpPb.wAuthor(playlistQuery.result.value(FP::DB::Table_Playlist::COL_AUTHOR).toString());

        // Build playlist
        FP::Playlist playlist = fpPb.build();

        // Add to cache
        mPlaylistsCache[playlist.getID()] = playlist;

        // Add to ID list
        targetPlaylistIDs.append(playlist.getID());
    }

    return targetPlaylistIDs;
}

const QList<QUuid> ImportWorker::getPlaylistSpecificGameIDs(FP::DB::QueryBuffer& playlistGameIDQuery)
{
    QList<QUuid> playlistSpecGameIDs;

    for(int i = 0; i < playlistGameIDQuery.size; i++)
    {
        // Advance to next record
        playlistGameIDQuery.result.next();

        // Add ID to list
        playlistSpecGameIDs.append(QUuid(playlistGameIDQuery.result.value(FP::DB::Table_Playlist_Game::COL_GAME_ID).toString()));
    }

    return playlistSpecGameIDs;
}

ImportWorker::ImportResult ImportWorker::preloadAddApps(Qx::GenericError& errorReport, FP::DB::QueryBuffer& addAppQuery)
{
    mAddAppsCache.reserve(addAppQuery.size);
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Advance to next record
        addAppQuery.result.next();

        // Form additional app from record
        FP::AddAppBuilder fpAab;
        fpAab.wID(addAppQuery.result.value(FP::DB::Table_Add_App::COL_ID).toString());
        fpAab.wAppPath(addAppQuery.result.value(FP::DB::Table_Add_App::COL_APP_PATH).toString());
        fpAab.wAutorunBefore(addAppQuery.result.value(FP::DB::Table_Add_App::COL_AUTORUN).toString());
        fpAab.wLaunchCommand(addAppQuery.result.value(FP::DB::Table_Add_App::COL_LAUNCH_COMMAND).toString());
        fpAab.wName(addAppQuery.result.value(FP::DB::Table_Add_App::COL_NAME).toString());
        fpAab.wWaitExit(addAppQuery.result.value(FP::DB::Table_Add_App::COL_WAIT_EXIT).toString());
        fpAab.wParentID(addAppQuery.result.value(FP::DB::Table_Add_App::COL_PARENT_ID).toString());

        // Build additional app
        FP::AddApp additionalApp = fpAab.build();

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

ImportWorker::ImportResult ImportWorker::processGames(Qx::GenericError& errorReport, QList<FP::DB::QueryBuffer>& gameQueries, bool playlistSpecific)
{
    for(int i = 0; i < gameQueries.size(); i++)
    {
        // Get current result
        FP::DB::QueryBuffer& currentPlatformGameResult = gameQueries[i];

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
            FP::GameBuilder fpGb;
            fpGb.wID(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_ID).toString());
            fpGb.wTitle(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_TITLE).toString());
            fpGb.wSeries(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_SERIES).toString());
            fpGb.wDeveloper(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_DEVELOPER).toString());
            fpGb.wPublisher(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_PUBLISHER).toString());
            fpGb.wDateAdded(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_DATE_ADDED).toString());
            fpGb.wDateModified(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_DATE_MODIFIED).toString());
            fpGb.wPlatform(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_PLATFORM).toString());
            fpGb.wBroken(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_BROKEN).toString());
            fpGb.wPlayMode(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_PLAY_MODE).toString());
            fpGb.wStatus(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_STATUS).toString());
            fpGb.wNotes(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_NOTES).toString());
            fpGb.wSource(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_SOURCE).toString());
            fpGb.wAppPath(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_APP_PATH).toString());
            fpGb.wLaunchCommand(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_LAUNCH_COMMAND).toString());
            fpGb.wReleaseDate(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_RELEASE_DATE).toString());
            fpGb.wVersion(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_VERSION).toString());
            fpGb.wOriginalDescription(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_ORIGINAL_DESC).toString());
            fpGb.wLanguage(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_LANGUAGE).toString());
            fpGb.wOrderTitle(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_ORDER_TITLE).toString());
            fpGb.wLibrary(currentPlatformGameResult.result.value(FP::DB::Table_Game::COL_LIBRARY).toString());

            // Build FP game and add to document
            FP::Game builtGame = fpGb.build();
            const Fe::Game* addedGame = currentPlatformDoc->addGame(builtGame);

            // Add ID to imported game cache
            mImportedGameIDsCache.insert(addedGame->getId());

            // Setup for ensuring image transfer
            Qx::GenericError imageTransferError; // Error return reference
            *mBlockingErrorResponse = QMessageBox::NoToAll; // Default to choice "NoToAll" incase the signal is not correctly connected using Qt::BlockingQueuedConnection
            bool skipAllImages = false; // NoToAll response tracker

            // Transfer game images if applicable
            if(mOptionSet.imageMode != Fe::Install::Reference)
            {
                while(!skipAllImages &&
                      (imageTransferError = mFrontendInstall->importImage(mOptionSet.imageMode, Fe::Install::Logo, mFlashpointInstall->logosDirectory(), *addedGame)).isValid())
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

                while(!skipAllImages &&
                      (imageTransferError = mFrontendInstall->importImage(mOptionSet.imageMode, Fe::Install::Screenshot, mFlashpointInstall->screenshotsDirectory(), *addedGame)).isValid())
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

        // Update progress dialog label
        emit progressStepChanged((playlistSpecific ? STEP_IMPORTING_PLAYLIST_SPEC_ADD_APPS : STEP_IMPORTING_PLATFORM_ADD_APPS).arg(currentPlatformGameResult.source));

        // Add applicable additional apps
        for (QSet<FP::AddApp>::iterator j = mAddAppsCache.begin(); j != mAddAppsCache.end();)
        {
            // If the current platform doc contains the game this add app belongs to, convert and add it, then remove it from cache
            if (currentPlatformDoc->containsGame((*j).getParentID()))
            {
               currentPlatformDoc->addAddApp(*j);
               j = mAddAppsCache.erase(j);

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

ImportWorker::ImportResult ImportWorker::processPlaylists(Qx::GenericError& errorReport, QList<FP::DB::QueryBuffer>& playlistGameQueries)
{
    for(FP::DB::QueryBuffer& currentPlaylistGameResult : playlistGameQueries)
    {
        // Get corresponding playlist from cache
        FP::Playlist currentPlaylist = mPlaylistsCache.value(QUuid(currentPlaylistGameResult.source));

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
            if(mImportedGameIDsCache.contains(QUuid(currentPlaylistGameResult.result.value(FP::DB::Table_Playlist_Game::COL_GAME_ID).toString())))
            {
                // Form game from record
                FP::PlaylistGameBuilder fpPgb;
                fpPgb.wID(currentPlaylistGameResult.result.value(FP::DB::Table_Playlist_Game::COL_ID).toString());
                fpPgb.wPlaylistID(currentPlaylistGameResult.result.value(FP::DB::Table_Playlist_Game::COL_PLAYLIST_ID).toString());
                fpPgb.wOrder(currentPlaylistGameResult.result.value(FP::DB::Table_Playlist_Game::COL_ORDER).toString());
                fpPgb.wGameID(currentPlaylistGameResult.result.value(FP::DB::Table_Playlist_Game::COL_GAME_ID).toString());

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

//Public
ImportWorker::ImportResult ImportWorker::doImport(Qx::GenericError& errorReport)
{
    // Import step status
    ImportResult importStepStatus;

    // Process query status
    QSqlError queryError;

    // Initial query buffers
    QList<FP::DB::QueryBuffer> gameQueries;
    QList<FP::DB::QueryBuffer> playlistSpecGameQueries;
    FP::DB::QueryBuffer addAppQuery;
    FP::DB::QueryBuffer playlistQueries;
    QList<FP::DB::QueryBuffer> playlistGameQueries;

    // Link Clifp to frontend
    mFrontendInstall->linkClifpPath(CLIFp::standardCLIFpPath(*mFlashpointInstall));

    // Get flashpoint database
    FP::DB* fpDatabase = mFlashpointInstall->database();

    // Make initial playlists query
    queryError = fpDatabase->queryPlaylistsByName(playlistQueries, mImportSelections.playlists);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Pre-load Playlists, add to cache and create ID list
    const QList<QUuid> targetPlaylistIDs = preloadPlaylists(playlistQueries);

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
        FP::DB::QueryBuffer pgIDQuery;

        // Make playlist game ID query
        queryError = fpDatabase->queryPlaylistGameIDs(pgIDQuery, targetPlaylistIDs);
        if(queryError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
            return Failed;
        }

        // Get playlist game ID list
        const QList<QUuid> targetPlaylistGameIDs = getPlaylistSpecificGameIDs(pgIDQuery);

        // Make unselected platforms list
        QStringList availablePlatforms = fpDatabase->platformList();
        QStringList unselectedPlatforms = QStringList(availablePlatforms);
        for(const QString& selPlatform : qAsConst(mImportSelections.platforms))
            unselectedPlatforms.removeAll(selPlatform);

        // Make game query
        queryError = fpDatabase->queryGamesByPlatform(playlistSpecGameQueries, unselectedPlatforms, mOptionSet.inclusionOptions, targetPlaylistGameIDs);
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
    queryError = fpDatabase->queryPlaylistGamesByPlaylist(playlistGameQueries, targetPlaylistIDs);
    if(queryError.isValid())
    {
       errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
       return Failed;
    }

    // Determine workload
    mCurrentProgressValue = 0;
    mMaximumProgressValue = addAppQuery.size; // Additional App pre-load
    for(const FP::DB::QueryBuffer& query : qAsConst(gameQueries)) // All games
        mMaximumProgressValue += query.size;
    for(const FP::DB::QueryBuffer& query : qAsConst(playlistSpecGameQueries)) // All playlist specific games
        mMaximumProgressValue += query.size;
    for(const FP::DB::QueryBuffer& query : qAsConst(playlistGameQueries)) // All playlist games
        mMaximumProgressValue += query.size;
    mMaximumProgressValue += addAppQuery.size * gameQueries.size() + addAppQuery.size * playlistSpecGameQueries.size(); // All checks of Additional Apps

    // Re-prep progress dialog
    emit progressMaximumChanged(mMaximumProgressValue);
    emit progressStepChanged(STEP_ADD_APP_PRELOAD);

    // Pre-load additional apps
    if((importStepStatus = preloadAddApps(errorReport, addAppQuery)) != Successful)
        return importStepStatus;

    // Process games and additional apps by platform
    if((importStepStatus = processGames(errorReport, gameQueries, false)) != Successful)
        return importStepStatus;

    // Process playlist specific games and additional apps by platform
    if((importStepStatus = processGames(errorReport, playlistSpecGameQueries, true)) != Successful)
        return importStepStatus;

    // Set image references if applicable
    if(mOptionSet.imageMode == Fe::Install::Reference)
    {
        if(mFrontendInstall->imageRefType() == Fe::Install::ImageRefType::Single)
        {
            throw std::runtime_error("Not implemented");
            //TODO Implement (also probably needs to go somewhere else), though probably best to leave
            // like this until a frontend that needs it is found to make it easier to determine the
            // best approach
        }
        else
        {
            // Update progress dialog label
            emit progressStepChanged(STEP_SETTING_IMAGE_REFERENCES);

            // Create playlist pecific platforms set
            QStringList playlistSpecPlatforms;
            for(const FP::DB::QueryBuffer& query : qAsConst(playlistSpecGameQueries))
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
    }

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
