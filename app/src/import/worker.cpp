// Unit Include
#include "worker.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>

// Project Includes
#include "kernel/clifp.h"
#include "import/details.h"
#include "import/backup.h"

namespace Import
{

//===============================================================================================================
// Worker
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
Worker::Worker(Fp::Install* flashpoint, Lr::IInstall* launcher, Selections importSelections, OptionSet optionSet) :
    mFlashpointInstall(flashpoint),
    mLauncherInstall(launcher),
    mImageManager(flashpoint, launcher, mCanceled),
    mImportSelections(importSelections),
    mOptionSet(optionSet),
    mCurrentProgress(0),
    mCanceled(false)
{
    mImageManager.setDownload(optionSet.downloadImages);
    mImageManager.setMode(optionSet.imageMode);
    connect(&mImageManager, &ImageManager::progressStepChanged, this, &Worker::progressStepChanged);
    connect(&mImageManager, &ImageManager::blockingErrorOccured, this, &Worker::blockingErrorOccured);
    connect(&mImageManager, &ImageManager::authenticationRequired, this, &Worker::authenticationRequired);
}

//-Destructor---------------------------------------------------------------------------------------------------
//Public:
Worker::~Worker() { Details::clearCurrent(); }

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
Qx::ProgressGroup* Worker::initializeProgressGroup(const QString& groupName, quint64 weight)
{
   Qx::ProgressGroup* pg = mProgressManager.addGroup(groupName);
   pg->setWeight(weight);
   pg->setValue(0);
   pg->setMaximum(0);
   return pg;
}

Qx::Error Worker::preloadPlaylists(QList<Fp::Playlist>& targetPlaylists)
{
    // Reset playlists
    targetPlaylists.clear();

    // Create copy of playlists list
    Fp::PlaylistManager* plm = mFlashpointInstall->playlistManager();
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

QList<QUuid> Worker::getPlaylistSpecificGameIds(const QList<Fp::Playlist>& playlists)
{
    QList<QUuid> playlistSpecGameIds;

    for(const Fp::Playlist& pl : playlists)
        for(const Fp::PlaylistGame& plg : pl.playlistGames())
            playlistSpecGameIds.append(plg.gameId());

    return playlistSpecGameIds;
}

Worker::Result Worker::processPlatformGames(Qx::Error& errorReport, std::unique_ptr<Lr::IPlatformDoc>& platformDoc, Fp::Db::QueryBuffer& gameQueryResult)
{
    Fp::Db* db = mFlashpointInstall->database();

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

        // Get tags
        Fp::GameTags gameTags;
        if(auto dbErr = db->getGameTags(gameTags, builtGame.id()); dbErr.isValid())
        {
            errorReport = Qx::Error();
            return Failed;
        }

        // Construct full game set
        Fp::Set::Builder sb;
        sb.wGame(builtGame); // From above
        sb.wTags(gameTags); // From above
        if(!mOptionSet.excludeAddApps)
        {
            // Add playable add apps
            auto addApps = mAddAppsCache.values(builtGame.id());
            addApps.removeIf([](const Fp::AddApp& aa){ return !aa.isPlayable(); });
            sb.wAddApps(addApps);
            mAddAppsCache.remove(builtGame.id());
        }

        // Add set to doc
        const Lr::Game* addedGame = platformDoc->addSet(sb.build());
        Q_ASSERT(addedGame);

        // Add ID to imported game cache
        mImportedGameIdsCache.insert(addedGame->id());

        // Handle images
        mImageManager.prepareGameImages(*addedGame);

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

void Worker::cullUnimportedPlaylistGames(QList<Fp::Playlist>& playlists)
{
    const auto& idCache = mImportedGameIdsCache;
    for(auto& pl : playlists)
    {
        pl.playlistGames().removeIf([idCache](const Fp::PlaylistGame& plg){
            return !idCache.contains(plg.gameId());
        });
    }
}

Worker::Result Worker::preloadAddApps(Qx::Error& errorReport, Fp::Db::QueryBuffer& addAppQuery)
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

Worker::Result Worker::processGames(Qx::Error& errorReport, QList<Fp::Db::QueryBuffer>& primary, QList<Fp::Db::QueryBuffer>& playlistSpecific)
{    
    // Status tracking
    Result platformImportStatus;

    // Track total platforms that have been handled
    qsizetype remainingPlatforms = primary.size() + playlistSpecific.size();

    // Use lambda to handle both lists due to major overlap
    auto platformsHandler = [&remainingPlatforms, &errorReport, this](QList<Fp::Db::QueryBuffer>& platformQueryResults, QString label) -> Result {
        Result result;

        for(int i = 0; i < platformQueryResults.size(); i++)
        {
            Fp::Db::QueryBuffer& currentQueryResult = platformQueryResults[i];

            // Open launcher platform doc
            std::unique_ptr<Lr::IPlatformDoc> currentPlatformDoc;
            Lr::DocHandlingError platformReadError = mLauncherInstall->checkoutPlatformDoc(currentPlatformDoc, currentQueryResult.source);

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

            //---Close out document----------------------------------

            // Forfeit document lease and save it
            Lr::DocHandlingError saveError;
            if((saveError = mLauncherInstall->commitPlatformDoc(std::move(currentPlatformDoc))).isValid())
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

Worker::Result Worker::processPlaylists(Qx::Error& errorReport, const QList<Fp::Playlist>& playlists)
{
    for(const auto& currentPlaylist : playlists)
    {
        // Update progress dialog label
        emit progressStepChanged(STEP_IMPORTING_PLAYLISTS.arg(currentPlaylist.title()));

        // Open launcher playlist doc
        std::unique_ptr<Lr::IPlaylistDoc> currentPlaylistDoc;
        Lr::DocHandlingError playlistReadError = mLauncherInstall->checkoutPlaylistDoc(currentPlaylistDoc, currentPlaylist.title());

        // Stop import if error occurred
        if(playlistReadError.isValid())
        {
            errorReport = playlistReadError;
            return Failed;
        }

        // Convert and set playlist header
        currentPlaylistDoc->setPlaylistData(currentPlaylist);

        // Forfeit document lease and save it
        Lr::DocHandlingError saveError;
        if((saveError = mLauncherInstall->commitPlaylistDoc(std::move(currentPlaylistDoc))).isValid())
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

Worker::Result Worker::processImages(Qx::Error& errorReport)
{    
    // Download
    if(Qx::DownloadManagerReport rep = mImageManager.downloadImages(); !rep.wasSuccessful())
    {
        // Notify GUI Thread of error
        emit blockingErrorOccured(mBlockingErrorResponse, rep, QMessageBox::Abort | QMessageBox::Ignore);

        // Check response
        if(*mBlockingErrorResponse == QMessageBox::Abort)
        {
            errorReport = Qx::Error();
            return Canceled;
        }
    }

    // Import
    if(Qx::Error err = mLauncherInstall->preImageProcessing(); err.isValid())
    {
        errorReport = err;
        return Failed;
    }

    if(!mImageManager.importImages())
    {
        errorReport = Qx::Error();
        return Canceled;
    }

    if(Qx::Error err = mLauncherInstall->postImageProcessing(); err.isValid())
    {
        errorReport = err;
        return Failed;
    }

    // Report successful step completion
    errorReport = Qx::Error();
    return Successful;
}

Worker::Result Worker::processIcons(Qx::Error& errorReport, const QStringList& platforms, const QList<Fp::Playlist>& playlists)
{
    bool canceled;
    errorReport = mImageManager.importIcons(canceled, platforms, playlists);
    return canceled ? Canceled : errorReport.isValid() ? Failed : Successful;
}

//Public
Worker::Result Worker::doImport(Qx::Error& errorReport)
{
    //-Setup----------------------------------------------------------------

    // Import step status
    Result importStepStatus;

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
        for(const QString& selPlatform : std::as_const(mImportSelections.platforms))
            unselectedPlatforms.removeAll(selPlatform);

        // Make game query
        queryError = fpDatabase->queryGamesByPlatform(playlistSpecGameQueries, unselectedPlatforms, mOptionSet.inclusionOptions, &targetPlaylistGameIds);
        if(queryError.isValid())
        {
            errorReport = queryError;
            return Failed;
        }
    }

    // Bail if there's no work to be done
    if(gameQueries.isEmpty() && playlistSpecGameQueries.isEmpty() && targetPlaylists.isEmpty())
        return Taskless;

    // Make initial add apps query
    if(!mOptionSet.excludeAddApps)
    {
        queryError = fpDatabase->queryAllAddApps(addAppQuery);
        if(queryError.isValid())
        {
            errorReport = queryError;
            return Failed;
        }
    }

    //-Determine Workload-------------------------------------------------
    quint64 totalGameCount = 0;

    QStringList playlistSpecPlatforms;
    for(const Fp::Db::QueryBuffer& query : std::as_const(playlistSpecGameQueries))
        playlistSpecPlatforms.append(query.source);
    QStringList involvedPlatforms = mImportSelections.platforms + playlistSpecPlatforms;

    // Additional App pre-load
    Qx::ProgressGroup* pgAddAppPreload = initializeProgressGroup(Pg::AddAppPreload, 2);
    pgAddAppPreload->setMaximum(addAppQuery.size);

    // Initialize game query progress group since there will always be at least one game to import
    Qx::ProgressGroup* pgGameImport = initializeProgressGroup(Pg::GameImport, 2);

    // All games
    for(const Fp::Db::QueryBuffer& query : std::as_const(gameQueries))
    {
        pgGameImport->increaseMaximum(query.size);
        totalGameCount += query.size;
    }

    // All playlist specific games
    for(const Fp::Db::QueryBuffer& query : std::as_const(playlistSpecGameQueries))
    {
        pgGameImport->increaseMaximum(query.size);
        totalGameCount += query.size;
    }

    // TODO: Maybe move initial progress setup of image related tasks to ImageManager
    Qx::ProgressGroup* pgImageDownload = nullptr;
    Qx::ProgressGroup* pgImageTransfer = nullptr;
    Qx::ProgressGroup* pgIconTransfer = nullptr;

    // Screenshot and Logo downloads
    if(mOptionSet.downloadImages)
    {
        pgImageDownload = initializeProgressGroup(Pg::ImageDownload, 3);
        pgImageDownload->setMaximum(totalGameCount * 2);
    }

    // Screenshot and Logo transfer
    if(mOptionSet.imageMode != ImageMode::Reference)
    {
        pgImageTransfer = initializeProgressGroup(Pg::ImageTransfer, 3);
        pgImageTransfer->setMaximum(totalGameCount * 2);
    }

    // Icon transfers
    // TODO: Somewhat wasteful because these create a temporary copy, but not a big deal for now
    quint64 iconCount = 0;
    if(!mLauncherInstall->platformCategoryIconPath().isEmpty())
        iconCount++;
    if(mLauncherInstall->platformIconsDirectory())
        iconCount += involvedPlatforms.size();
    if(mLauncherInstall->playlistIconsDirectory())
        iconCount += targetPlaylists.size();

    if(iconCount > 0)
    {
        pgIconTransfer = initializeProgressGroup(Pg::IconTransfer, 3);
        pgIconTransfer->increaseMaximum(iconCount);
    }

    // All playlists
    if(!targetPlaylists.isEmpty())
    {
        Qx::ProgressGroup* pgPlaylistImport = initializeProgressGroup(Pg::PlaylistImport, 4);
        pgPlaylistImport->increaseMaximum(targetPlaylists.size());
    }

    // Install image progress groups
    mImageManager.setProgressGroups(pgImageDownload, pgImageTransfer, pgIconTransfer);

    // Connect progress manager signal
    connect(&mProgressManager, &Qx::GroupedProgressManager::progressUpdated, this, &Worker::pmProgressUpdated);

    //-Handle Launcher Specific Import Setup------------------------------
    Details details{
        .updateOptions = mOptionSet.updateOptions,
        .imageMode = mOptionSet.imageMode,
        .clifpPath = CLIFp::standardCLIFpPath(*mFlashpointInstall),
        .involvedPlatforms = involvedPlatforms,
        .involvedPlaylists = mImportSelections.playlists
    };
    Details::setCurrent(details);

    errorReport = mLauncherInstall->preImport();
    if(errorReport.isValid())
        return Failed;

    //-Set Progress Indicator To First Step----------------------------------
    emit progressMaximumChanged(mProgressManager.maximum());
    emit progressStepChanged(STEP_ADD_APP_PRELOAD);

    //-Primary Import Stages-------------------------------------------------

    // Pre-load additional apps
    if(!mOptionSet.excludeAddApps)
    {
        if((importStepStatus = preloadAddApps(errorReport, addAppQuery)) != Successful)
            return importStepStatus;
    }

    // Handle Launcher specific pre-platform tasks
    errorReport = mLauncherInstall->prePlatformsImport();
    if(errorReport.isValid())
        return Failed;

    // Process games and additional apps by platform (primary and playlist specific)
    if((importStepStatus = processGames(errorReport, gameQueries, playlistSpecGameQueries)) != Successful)
        return importStepStatus;

    // Handle Launcher specific post-platform tasks
    errorReport = mLauncherInstall->postPlatformsImport();
    if(errorReport.isValid())
        return Failed;

    // Process images
    if((importStepStatus = processImages(errorReport)) != Successful)
        return importStepStatus;

    // Process Icons
    if((importStepStatus = processIcons(errorReport, involvedPlatforms, targetPlaylists)) != Successful)
        return importStepStatus;

    // Process playlists
    if(!targetPlaylists.isEmpty())
    {
        // Remove un-imported games from playlists
        cullUnimportedPlaylistGames(targetPlaylists);

        // Handle Launcher specific pre-playlist tasks
        errorReport = mLauncherInstall->prePlaylistsImport();
        if(errorReport.isValid())
            return Failed;

        if((importStepStatus = processPlaylists(errorReport, targetPlaylists)) != Successful)
            return importStepStatus;

        // Handle Launcher specific pre-playlist tasks
        errorReport = mLauncherInstall->postPlaylistsImport();
        if(errorReport.isValid())
            return Failed;
    }

    // Handle Launcher specific cleanup
    emit progressStepChanged(STEP_FINALIZING);
    errorReport = mLauncherInstall->postImport();
    if(errorReport.isValid())
        return Failed;

    // Check for final cancellation
    if(mCanceled)
    {
        errorReport = Qx::Error();
        return Canceled;
    }

    // Reset install
    mLauncherInstall->softReset();

    // Purge purgeables
    Import::BackupManager::instance()->purge();

    // Report successful import completion
    errorReport = Qx::Error();
    return Successful;
}

//-Slots---------------------------------------------------------------------------------------------------------
//Private Slots:
void Worker::pmProgressUpdated(quint64 currentProgress)
{
    /* NOTE: This is required because if the value isn't actually different than the current when
     * the connected QProgressDialog::setValue() is triggered then processEvents() won't be called.
     * This is a problem because the fixed range of Qx::GroupedProgressManager of 0-100 means that groups
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
void Worker::notifyCanceled() { mCanceled = true; }

}
