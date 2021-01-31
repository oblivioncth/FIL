#include "import-worker.h"

//===============================================================================================================
// IMPORT WORKER
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

//-Instance Functions--------------------------------------------------------------------------------------------
//Private
const QList<QUuid> ImportWorker::preloadPlaylists(FP::Install::DBQueryBuffer& playlistQuery)
{
    QList<QUuid> targetPlaylistIDs;

    for(int i = 0; i < playlistQuery.size; i++)
    {
        // Advance to next record
        playlistQuery.result.next();

        // Form playlist from record
        FP::PlaylistBuilder fpPb;
        fpPb.wID(playlistQuery.result.value(FP::Install::DBTable_Playlist::COL_ID).toString());
        fpPb.wTitle(playlistQuery.result.value(FP::Install::DBTable_Playlist::COL_TITLE).toString());
        fpPb.wDescription(playlistQuery.result.value(FP::Install::DBTable_Playlist::COL_DESCRIPTION).toString());
        fpPb.wAuthor(playlistQuery.result.value(FP::Install::DBTable_Playlist::COL_AUTHOR).toString());

        // Build playlist
        FP::Playlist playlist = fpPb.build();

        // Add to cache
        mPlaylistsCache[playlist.getID()] = playlist;

        // Add to ID list
        targetPlaylistIDs.append(playlist.getID());
    }

    return targetPlaylistIDs;
}

const QList<QUuid> ImportWorker::getPlaylistSpecificGameIDs(FP::Install::DBQueryBuffer& playlistGameIDQuery)
{
    QList<QUuid> playlistSpecGameIDs;

    for(int i = 0; i < playlistGameIDQuery.size; i++)
    {
        // Advance to next record
        playlistGameIDQuery.result.next();

        // Add ID to list
        playlistSpecGameIDs.append(QUuid(playlistGameIDQuery.result.value(FP::Install::DBTable_Playlist_Game::COL_GAME_ID).toString()));
    }

    return playlistSpecGameIDs;
}

ImportWorker::ImportResult ImportWorker::preloadAddApps(Qx::GenericError& errorReport, FP::Install::DBQueryBuffer& addAppQuery)
{
    mAddAppsCache.reserve(addAppQuery.size);
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

ImportWorker::ImportResult ImportWorker::processGames(Qx::GenericError& errorReport, QList<FP::Install::DBQueryBuffer>& gameQueries, bool playlistSpecific)
{
    for(int i = 0; i < gameQueries.size(); i++)
    {
        // Get current result
        FP::Install::DBQueryBuffer& currentPlatformGameResult = gameQueries[i];

        // Update progress dialog label
        emit progressStepChanged((playlistSpecific ? STEP_IMPORTING_PLAYLIST_SPEC_GAMES : STEP_IMPORTING_PLATFORM_GAMES).arg(currentPlatformGameResult.source));

        // Open LB platform doc
        LB::Xml::DataDocHandle docRequest = {LB::Xml::PlatformDoc::TYPE_NAME, currentPlatformGameResult.source};
        std::unique_ptr<LB::Xml::PlatformDoc> currentPlatformXML;
        Qx::XmlStreamReaderError platformReadError = mLaunchBoxInstall->openPlatformDoc(currentPlatformXML, docRequest.docName, mOptionSet.updateOptions);

        // Stop import if error occured
        if(platformReadError.isValid())
        {
            // Emit import failure
            errorReport = Qx::GenericError(Qx::GenericError::Critical, LB::Xml::formatDataDocError(MSG_LB_XML_UNEXPECTED_ERROR, docRequest),
                                           platformReadError.getText());
            return Failed;
        }

        // Setup for ensuring image sub-directories exist
        QString imageDirError; // Error return reference
        *mBlockingErrorResponse = QMessageBox::No; // Default to choice "No" incase the signal is not correctly connected using Qt::BlockingQueuedConnection

        // Check image sub-directories
        while(!mLaunchBoxInstall->ensureImageDirectories(imageDirError, currentPlatformGameResult.source))
        {
            // Notify GUI Thread of error
            emit blockingErrorOccured(mBlockingErrorResponse, Qx::GenericError(Qx::GenericError::Error, imageDirError, "Retry?", QString(), CAPTION_IMAGE_ERR),
                                      QMessageBox::Yes | QMessageBox::No);

            // Check response
            if(*mBlockingErrorResponse == QMessageBox::No)
               break;
        }

        // Add/Update games
        for(int j = 0; j < currentPlatformGameResult.size; j++)
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
            fpGb.wLibrary(currentPlatformGameResult.result.value(FP::Install::DBTable_Game::COL_LIBRARY).toString());

            // Convert and convert FP game to LB game and add to document
            LB::Game builtGame = LB::Game(fpGb.build(), mFlashpointInstall->getCLIFpPath());
            currentPlatformXML->addGame(builtGame);

            // Setup for ensuring image sub-directories exist
            QString imageTransferError; // Error return reference
            *mBlockingErrorResponse = QMessageBox::NoToAll; // Default to choice "NoToAll" incase the signal is not correctly connected using Qt::BlockingQueuedConnection
            bool skipAllImages = false; // NoToAll response tracker

            // Transfer game images if applicable
            if(mOptionSet.imageMode != LB::Install::Reference)
            {
                while(!skipAllImages && !mLaunchBoxInstall->transferLogo(imageTransferError, mOptionSet.imageMode, mFlashpointInstall->getLogosDirectory(), builtGame))
                {
                    // Notify GUI Thread of error
                    emit blockingErrorOccured(mBlockingErrorResponse, Qx::GenericError(Qx::GenericError::Error, imageTransferError, "Retry?", QString(), CAPTION_IMAGE_ERR),
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);

                    // Check response
                    if(*mBlockingErrorResponse == QMessageBox::No)
                       break;
                    else if(*mBlockingErrorResponse == QMessageBox::NoToAll)
                       skipAllImages = true;
                }

                while(!skipAllImages && !mLaunchBoxInstall->transferScreenshot(imageTransferError, mOptionSet.imageMode, mFlashpointInstall->getScrenshootsDirectory(), builtGame))
                {
                    // Notify GUI Thread of error
                    emit blockingErrorOccured(mBlockingErrorResponse, Qx::GenericError(Qx::GenericError::Error, imageTransferError, "Retry?", QString(), CAPTION_IMAGE_ERR),
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);

                    // Check response
                    if(*mBlockingErrorResponse == QMessageBox::No)
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
            if (currentPlatformXML->containsGame((*j).getParentID()))
            {
               currentPlatformXML->addAddApp(LB::AddApp(*j, mFlashpointInstall->getCLIFpPath()));
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
        currentPlatformXML->finalize();

        // Add final game details to Playlist Game lookup cache
        for (QHash<QUuid, LB::Game>::const_iterator i = currentPlatformXML->getFinalGames().constBegin();
             i != currentPlatformXML->getFinalGames().constEnd(); ++i)
           mPlaylistGameDetailsCache[i.key()] = {i.value().getTitle(), QFileInfo(i.value().getAppPath()).fileName(), i.value().getPlatform()};

        // Forefit doucment lease and save it
        QString saveError;
        if(!mLaunchBoxInstall->savePlatformDoc(saveError, std::move(currentPlatformXML)))
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                           LB::Xml::formatDataDocError(LB::Xml::ERR_WRITE_FAILED, docRequest), saveError);
            return Failed;
        }

    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::setImageReferences(Qx::GenericError& errorReport, QSet<QString> platforms)
{
    // Open platforms document
    std::unique_ptr<LB::Xml::PlatformsDoc> platformConfigXML;
    LB::Xml::DataDocHandle requestHandle = {LB::Xml::PlatformsDoc::TYPE_NAME, LB::Xml::PlatformsDoc::STD_NAME}; // For errors only
    Qx::XmlStreamReaderError platformConfigReadError = mLaunchBoxInstall->openPlatformsDoc(platformConfigXML);

    // Stop import if error occured
    if(platformConfigReadError.isValid())
    {
        // Emit import failure
        errorReport = Qx::GenericError(Qx::GenericError::Critical, LB::Xml::formatDataDocError(MSG_LB_XML_UNEXPECTED_ERROR, requestHandle),
                                       platformConfigReadError.getText());
        return Failed;
    }

    // Set media folder paths and ensure document contains platform or else image paths will be ignored
    for(const QString& platform : platforms)
    {
        platformConfigXML->setMediaFolder(platform, LB::Install::LOGO_PATH, QDir::toNativeSeparators(mFlashpointInstall->getLogosDirectory().absolutePath()));
        platformConfigXML->setMediaFolder(platform, LB::Install::SCREENSHOT_PATH,  QDir::toNativeSeparators(mFlashpointInstall->getScrenshootsDirectory().absolutePath()));

        if(!platformConfigXML->containsPlatform(platform))
        {
            LB::PlatformBuilder pb;
            pb.wName(platform);
            platformConfigXML->addPlatform(pb.build());
        }
    }

    // Save platforms document
    QString saveError;
    if(!mLaunchBoxInstall->savePlatformsDoc(saveError, std::move(platformConfigXML)))
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                       LB::Xml::formatDataDocError(LB::Xml::ERR_WRITE_FAILED, requestHandle), saveError);
        return Failed;
    }

    // Report successful step completion
    errorReport = Qx::GenericError();
    return Successful;
}

ImportWorker::ImportResult ImportWorker::processPlaylists(Qx::GenericError& errorReport, QList<FP::Install::DBQueryBuffer>& playlistGameQueries)
{
    for(FP::Install::DBQueryBuffer& currentPlaylistGameResult : playlistGameQueries)
    {
        // Get corresponding playlist from cache
        FP::Playlist currentPlaylist = mPlaylistsCache.value(QUuid(currentPlaylistGameResult.source));

        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLAYLIST_GAMES.arg(currentPlaylist.getTitle()));

        // Open LB playlist doc
        LB::Xml::DataDocHandle docRequest = {LB::Xml::PlaylistDoc::TYPE_NAME, currentPlaylist.getTitle()};
        std::unique_ptr<LB::Xml::PlaylistDoc> currentPlaylistXML;
        Qx::XmlStreamReaderError playlistReadError = mLaunchBoxInstall->openPlaylistDoc(currentPlaylistXML, docRequest.docName, mOptionSet.updateOptions);

        // Stop import if error occured
        if(playlistReadError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                           LB::Xml::formatDataDocError(MSG_LB_XML_UNEXPECTED_ERROR, docRequest), playlistReadError.getText());
            return Failed;
        }

        // Convert and set playlist header
        currentPlaylistXML->setPlaylistHeader(LB::PlaylistHeader(currentPlaylist));

        // Add/Update playlist games
        for(int i = 0; i < currentPlaylistGameResult.size; i++)
        {
            // Advance to next record
            currentPlaylistGameResult.result.next();

            // Only process the playlist game if it was included in import
            if(mPlaylistGameDetailsCache.contains(QUuid(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_GAME_ID).toString())))
            {
                // Form game from record
                FP::PlaylistGameBuilder fpPgb;
                fpPgb.wID(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_ID).toString());
                fpPgb.wPlaylistID(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_PLAYLIST_ID).toString());
                fpPgb.wOrder(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_ORDER).toString());
                fpPgb.wGameID(currentPlaylistGameResult.result.value(FP::Install::DBTable_Playlist_Game::COL_GAME_ID).toString());

                // Build FP playlist game, convert to LB and add
                currentPlaylistXML->addPlaylistGame(LB::PlaylistGame(fpPgb.build(), mPlaylistGameDetailsCache));
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
        currentPlaylistXML->finalize();

        // Forefit doucment lease and save it
        QString saveError;
        if(!mLaunchBoxInstall->savePlaylistDoc(saveError, std::move(currentPlaylistXML)))
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical,
                                           LB::Xml::formatDataDocError(LB::Xml::ERR_WRITE_FAILED, docRequest), saveError);
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
    QList<FP::Install::DBQueryBuffer> gameQueries;
    QList<FP::Install::DBQueryBuffer> playlistSpecGameQueries;
    FP::Install::DBQueryBuffer addAppQuery;
    FP::Install::DBQueryBuffer playlistQueries;
    QList<FP::Install::DBQueryBuffer> playlistGameQueries;

    // Make initial playlists query
    queryError = mFlashpointInstall->queryPlaylistsByName(playlistQueries, mImportSelections.playlists);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Pre-load Playlists, add to cache and create ID list
    const QList<QUuid> targetPlaylistIDs = preloadPlaylists(playlistQueries);

    // Make initial game query
    queryError = mFlashpointInstall->queryGamesByPlatform(gameQueries, mImportSelections.platforms, mOptionSet.inclusionOptions);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Make initial playlist specific game query if applicable
    if(mOptionSet.playlistMode == LB::Install::PlaylistGameMode::ForceAll)
    {
        FP::Install::DBQueryBuffer pgIDQuery;

        // Make playlist game ID query
        queryError = mFlashpointInstall->queryPlaylistGameIDs(pgIDQuery, targetPlaylistIDs);
        if(queryError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
            return Failed;
        }

        // Get playlist game ID list
        const QList<QUuid> targetPlaylistGameIDs = getPlaylistSpecificGameIDs(pgIDQuery);

        // Make unselected platforms list
        QStringList availablePlatforms = mFlashpointInstall->getPlatformList();
        QSet<QString> unselectedPlatforms = QSet<QString>(availablePlatforms.begin(), availablePlatforms.end()).subtract(mImportSelections.platforms);

        // Make game query
        queryError = mFlashpointInstall->queryGamesByPlatform(playlistSpecGameQueries, unselectedPlatforms, mOptionSet.inclusionOptions, targetPlaylistGameIDs);
        if(queryError.isValid())
        {
            errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
            return Failed;
        }
    }

    // Make initial add apps query
    queryError = mFlashpointInstall->queryAllAddApps(addAppQuery);
    if(queryError.isValid())
    {
        errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
        return Failed;
    }

    // Make initial playlist games query
    queryError = mFlashpointInstall->queryPlaylistGamesByPlaylist(playlistGameQueries, targetPlaylistIDs);
    if(queryError.isValid())
    {
       errorReport = Qx::GenericError(Qx::GenericError::Critical, MSG_FP_DB_UNEXPECTED_ERROR, queryError.text());
       return Failed;
    }

    // Determine workload
    mCurrentProgressValue = 0;
    mMaximumProgressValue = addAppQuery.size; // Additional App pre-load
    for(const FP::Install::DBQueryBuffer& query : gameQueries) // All games
        mMaximumProgressValue += query.size;
    for(const FP::Install::DBQueryBuffer& query : playlistSpecGameQueries) // All playlist specific games
        mMaximumProgressValue += query.size;
    for(const FP::Install::DBQueryBuffer& query : playlistGameQueries) // All playlist games
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
    if(mOptionSet.imageMode == LB::Install::Reference)
    {
        // Update progress dialog label
        emit progressStepChanged(STEP_SETTING_IMAGE_REFERENCES);

        // Create playlist pecific platforms set
        QSet<QString> playlistSpecPlatforms;
        for(const FP::Install::DBQueryBuffer& query : playlistSpecGameQueries)
            playlistSpecPlatforms.insert(query.source);

        if((importStepStatus = setImageReferences(errorReport, mImportSelections.platforms + playlistSpecPlatforms)) != Successful)
            return importStepStatus;
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
    mLaunchBoxInstall->softReset();

    // Report successful import completion
    errorReport = Qx::GenericError();
    return Successful;
}

//-Slots---------------------------------------------------------------------------------------------------------
//Public Slots:
void ImportWorker::notifyCanceled() { mCanceled = true; }
