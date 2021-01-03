#include "importworker.h"

//===============================================================================================================
// CORE IMPORT WORKER
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
ImportWorker::ImportWorker(std::shared_ptr<FP::Install> fpInstallForWork,
                           std::shared_ptr<LB::Install> lbInstallForWork,
                           ImportSelections importSelections,
                           OptionSet optionSet)
    : mFlashpointInstall(fpInstallForWork),
      mLaunchBoxInstall(lbInstallForWork),
      mImportSelections(importSelections),
      mOptionSet(optionSet) {}

//-Slots---------------------------------------------------------------------------------------------------------
//Public Slots:
ImportWorker::ImportResult ImportWorker::doImport(Qx::GenericError& errorReport)
{
    // Prepare response "return" variable (pointer) for possible blocking errors
    std::shared_ptr<int> blockingErrorResponse = std::make_shared<int>();

    // Process query status
    QSqlError queryError;

    // Caches
    QSet<FP::AddApp> addAppsCache;
    QHash<QUuid, FP::Playlist> playlistsCache;
    QHash<QUuid, LB::PlaylistGame::EntryDetails> playlistGameDetailsCache;

    // Initial query buffers
    QList<FP::Install::DBQueryBuffer> gameQueries;
    FP::Install::DBQueryBuffer addAppQuery;
    FP::Install::DBQueryBuffer playlistQueries;
    QList<FP::Install::DBQueryBuffer> playlistGameQueries;

    // Make initial game query
    queryError = mFlashpointInstall->initialGameQuery(gameQueries, mImportSelections.platforms);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Make initial add apps query
    queryError = mFlashpointInstall->initialAddAppQuery(addAppQuery);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Make initial playlists query
    queryError = mFlashpointInstall->initialPlaylistQuery(playlistQueries, mImportSelections.playlists);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Process Playlists, add to cache and create ID list
    QList<QUuid> targetPlaylistIDs;
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

        // Build playlist
        FP::Playlist playlist = fpPb.build();

        // Add to cache
        playlistsCache[playlist.getID()] = playlist;

        // Add to ID list
        targetPlaylistIDs.append(playlist.getID());
    }

    // Make initial playlist games query
    queryError = mFlashpointInstall->initialPlaylistGameQuery(playlistGameQueries, targetPlaylistIDs);
    if(queryError.isValid())
    {
       errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
       return Failed;
    }

    // Determine workload
    int currentProgressValue = 0;
    int maximumProgressValue = addAppQuery.size; // Additional App pre-load
    for(const FP::Install::DBQueryBuffer& query : gameQueries) // All games
        maximumProgressValue += query.size;
    for(const FP::Install::DBQueryBuffer& query : playlistGameQueries) // All playlist games
        maximumProgressValue += query.size;
    maximumProgressValue += addAppQuery.size * gameQueries.size(); // All checks of Additional Apps

    // Re-prep progress dialog
    emit progressMaximumChanged(maximumProgressValue);
    emit progressStepChanged(STEP_ADD_APP_PRELOAD);

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
        fpAab.wLaunchCommand(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_LAUNCH_COMMAND).toString());
        fpAab.wName(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString());
        fpAab.wWaitExit(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_WAIT_EXIT).toString());
        fpAab.wParentID(addAppQuery.result.value(FP::Install::DBTable_Add_App::COL_PARENT_ID).toString());

        // Build additional app
        FP::AddApp additionalApp = fpAab.build();

        // Add to cache
        addAppsCache.insert(additionalApp);

        // Update progress dialog value
        if(mCanceled)
        {
           errorReport = Qx::GenericError();
           return Canceled;
        }
        else
            emit progressValueChanged(++currentProgressValue);
    }

    // Process games and additional apps by platform
    for(int i = 0; i < gameQueries.size(); i++)
    {
        // Get current result
        FP::Install::DBQueryBuffer currentPlatformGameResult = gameQueries[i];

        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLATFORM_GAMES.arg(currentPlatformGameResult.source));

        // Open LB platform doc
        LB::Install::XMLHandle docRequest = {LB::Install::Platform, currentPlatformGameResult.source};
        std::unique_ptr<LB::Install::XMLDocLegacy> currentPlatformXML;
        Qx::XmlStreamReaderError platformReadError = mLaunchBoxInstall->openXMLDocument(currentPlatformXML, docRequest, mOptionSet.updateOptions);

        // Stop import if error occured
        if(platformReadError.isValid())
        {
            // Emit import failure
            errorReport = Qx::GenericError(Qx::GenericError::Critical, LB::Install::populateErrorWithTarget(MSG_LB_XML_UNEXPECTED_ERROR, docRequest),
                                           platformReadError.getText());
            return Failed;
        }

        // Setup for ensuring image sub-directories exist
        QString imageDirError; // Error return reference
        *blockingErrorResponse = QMessageBox::No; // Default to choice "No" incase the signal is not correctly connected using Qt::BlockingQueuedConnection

        // Check image sub-directories
        while(!mLaunchBoxInstall->ensureImageDirectories(imageDirError, currentPlatformGameResult.source))
        {
            // Notify GUI Thread of error
            emit blockingErrorOccured(blockingErrorResponse, Qx::GenericError(Qx::GenericError::Error, imageDirError, "Retry?", QString(), CAPTION_IMAGE_ERR),
                                      QMessageBox::Yes | QMessageBox::No);

            // Check response
            if(*blockingErrorResponse == QMessageBox::No)
               break;
        }

        // Add/Update games
        for(int j = 0; j < currentPlatformGameResult.size; j++)
        {
            // Advance to next record
            currentPlatformGameResult.result.next();

            // Check if game is extreme and only include if user ticked the option
            if(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_EXTREME).toString() == "0" || mOptionSet.generalOptions.includeExtreme)
            {
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
                fpGb.wLibrary(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_LIBRARY).toString());

                // Convert and convert FP game to LB game and add to document
                LB::Game builtGame = LB::Game(fpGb.build(), mFlashpointInstall->getCLIFpPath());
                currentPlatformXML->addGame(builtGame);

                // Setup for ensuring image sub-directories exist
                QString imageTransferError; // Error return reference
                *blockingErrorResponse = QMessageBox::NoToAll; // Default to choice "NoToAll" incase the signal is not correctly connected using Qt::BlockingQueuedConnection
                bool skipAllImages = false; // NoToAll response tracker

                // Transfer game images
                while(!skipAllImages && !mLaunchBoxInstall->transferLogo(imageTransferError, mOptionSet.imageMode, mFlashpointInstall->getLogosDirectory(), builtGame))
                {
                    // Notify GUI Thread of error
                    emit blockingErrorOccured(blockingErrorResponse, Qx::GenericError(Qx::GenericError::Error, imageTransferError, "Retry?", QString(), CAPTION_IMAGE_ERR),
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);

                    // Check response
                    if(*blockingErrorResponse == QMessageBox::No)
                       break;
                    else if(*blockingErrorResponse == QMessageBox::NoToAll)
                       skipAllImages = true;
                }

                while(!skipAllImages && !mLaunchBoxInstall->transferScreenshot(imageTransferError, mOptionSet.imageMode, mFlashpointInstall->getScrenshootsDirectory(), builtGame))
                {
                    // Notify GUI Thread of error
                    emit blockingErrorOccured(blockingErrorResponse, Qx::GenericError(Qx::GenericError::Error, imageTransferError, "Retry?", QString(), CAPTION_IMAGE_ERR),
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);

                    // Check response
                    if(*blockingErrorResponse == QMessageBox::No)
                       break;
                    else if(*blockingErrorResponse == QMessageBox::NoToAll)
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
                emit progressValueChanged(++currentProgressValue);
        }

        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLATFORM_ADD_APPS.arg(currentPlatformGameResult.source));

        // Add applicable additional apps
        for (QSet<FP::AddApp>::iterator j = addAppsCache.begin(); j != addAppsCache.end();)
        {
            // If the current platform doc contains the game this add app belongs to, convert and add it, then remove it from cache
            if (currentPlatformXML->containsGame((*j).getParentID()))
            {
               currentPlatformXML->addAddApp(LB::AddApp(*j, mFlashpointInstall->getCLIFpPath()));
               j = addAppsCache.erase(j);

               // Reduce progress dialog maximum by total iterations cut from future platforms
               maximumProgressValue = maximumProgressValue - gameQueries.size() + i + 1;
               emit progressMaximumChanged(maximumProgressValue);
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
                emit progressValueChanged(++currentProgressValue);
        }

        // Finalize document
        currentPlatformXML->finalize();

        // Add final game details to Playlist Game lookup cache
        for (QHash<QUuid, LB::Game>::const_iterator i = currentPlatformXML->getFinalGames().constBegin();
             i != currentPlatformXML->getFinalGames().constEnd(); ++i)
           playlistGameDetailsCache[i.key()] = {i.value().getTitle(), QFileInfo(i.value().getAppPath()).fileName(), i.value().getPlatform()};

        // Forefit doucment lease and save it
        QString saveError;
        if(!mLaunchBoxInstall->saveXMLDocument(saveError, std::move(currentPlatformXML)))
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                           LB::Install::populateErrorWithTarget(LB::Install::XMLWriterLegacy::ERR_WRITE_FAILED, docRequest), saveError);
            return Failed;
        }

    }

    // Process playlists
    for(FP::Install::DBQueryBuffer& currentPlaylistGameResult : playlistGameQueries)
    {
        // Get corresponding playlist from cache
        FP::Playlist currentPlaylist = playlistsCache.value(QUuid(currentPlaylistGameResult.source));

        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLAYLIST_GAMES.arg(currentPlaylist.getTitle()));

        // Open LB playlist doc
        LB::Install::XMLHandle docRequest = {LB::Install::Playlist, currentPlaylist.getTitle()};
        std::unique_ptr<LB::Install::XMLDocLegacy> currentPlaylistXML;
        Qx::XmlStreamReaderError playlistReadError = mLaunchBoxInstall->openXMLDocument(currentPlaylistXML, docRequest, mOptionSet.updateOptions);

        // Stop import if error occured
        if(playlistReadError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                           LB::Install::populateErrorWithTarget(MSG_LB_XML_UNEXPECTED_ERROR, docRequest), playlistReadError.getText());
            return Failed;
        }

        // Convert and set playlist header
        currentPlaylistXML->setPlaylistHeader(LB::PlaylistHeader(currentPlaylist));

        // Add/Update playlist games
        for(int i = 0; i < currentPlaylistGameResult.size; i++)
        {
            // Advance to next record
            currentPlaylistGameResult.result.next();

            // Only process the playlist game if it was included in import (TODO: Possibly implement checking all other Platform xmls for presence of this game)
            if(playlistGameDetailsCache.contains(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_GAME_ID).toString()))
            {
                // Form game from record
                FP::PlaylistGameBuilder fpPgb;
                fpPgb.wID(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_ID).toString());
                fpPgb.wPlaylistID(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_PLAYLIST_ID).toString());
                fpPgb.wOrder(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_ORDER).toString());
                fpPgb.wGameID(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_GAME_ID).toString());

                // Build FP playlist game, convert to LB and add
                currentPlaylistXML->addPlaylistGame(LB::PlaylistGame(fpPgb.build(), playlistGameDetailsCache));
            }

            // Update progress dialog value
            if(mCanceled)
            {
                errorReport = Qx::GenericError();
                return Canceled;
            }
            else
                emit progressValueChanged(++currentProgressValue);
        }

        // Finalize document
        currentPlaylistXML->finalize();

        // Forefit doucment lease and save it
        QString saveError;
        if(!mLaunchBoxInstall->saveXMLDocument(saveError, std::move(currentPlaylistXML)))
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                           LB::Install::populateErrorWithTarget(LB::Install::XMLWriterLegacy::ERR_WRITE_FAILED, docRequest), saveError);
            return Failed;
        }
    }

    // Reset install
    mLaunchBoxInstall->softReset();

    // Emit successful import completion
    errorReport = Qx::GenericError();
    return Successful;
}

void ImportWorker::notifyCanceled() { mCanceled = true; }
