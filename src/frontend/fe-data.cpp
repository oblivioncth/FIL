#include "fe-data.h"
#include "fe-install.h"

namespace Fe
{

//===============================================================================================================
// DataDoc::Identifier
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const DataDoc::Identifier& lhs, const DataDoc::Identifier& rhs) noexcept
{
    return lhs.mDocType == rhs.mDocType && lhs.mDocName == rhs.mDocName;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const DataDoc::Identifier& key, uint seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.mDocType);
    seed = hash(seed, key.mDocName);

    return seed;
}

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
DataDoc::Type DataDoc::Identifier::docType() const { return mDocType; }
QString DataDoc::Identifier::docTypeString() const { return TYPE_STRINGS.value(mDocType); }
QString DataDoc::Identifier::docName() const { return mDocName; }

//===============================================================================================================
// DataDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
DataDoc::DataDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName)
    : mParent(parent),
      mDocumentFile(std::move(docFile)),
      mName(docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
QString DataDoc::filePath() const { return mDocumentFile->fileName(); }

//Public:
Install* DataDoc::parent() const { return mParent; }
DataDoc::Identifier DataDoc::identifier() const { return Identifier(type(), mName); }
Qx::GenericError DataDoc::standardError(StandardError error, Qx::GenericError::ErrorLevel errorLevel) const
{
    QString formattedError = STD_ERRORS[error];
    formattedError.replace(M_DOC_TYPE, identifier().docTypeString());
    formattedError.replace(M_DOC_NAME, identifier().docName());
    formattedError.replace(M_DOC_PARENT, parent()->name());

    return Qx::GenericError(errorLevel, formattedError);
}

void DataDoc::clearFile() { mDocumentFile->resize(0); }

//===============================================================================================================
// DataDocReader
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Public:
DataDocReader::DataDocReader(DataDoc* targetDoc) : mTargetDocument(targetDoc) {}

//===============================================================================================================
// DataDocWriter
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Public:
DataDocWriter::DataDocWriter(DataDoc* sourceDoc) : mSourceDocument(sourceDoc) {}

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::PlatformDoc(Install* parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions, const Key&)
    : DataDoc(parent, std::move(docFile), docName), mUpdateOptions(updateOptions) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlatformDoc::type() const { return Type::Platform; }

//Public:
const QHash<QUuid, std::shared_ptr<Game>>& PlatformDoc::getFinalGames() const { return mGamesFinal; }
const QHash<QUuid, std::shared_ptr<AddApp>>& PlatformDoc::getFinalAddApps() const { return mAddAppsFinal; }

bool PlatformDoc::containsGame(QUuid gameID) const { return mGamesFinal.contains(gameID) || mGamesExisting.contains(gameID); }
bool PlatformDoc::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

void PlatformDoc::addGame(FP::Game game)
{
    std::shared_ptr<Game> feGame = prepareGame(game);
    QUuid key = feGame->getID();

    // Check if game exists
    if(mGamesExisting.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
        {
            feGame->transferOtherFields(mGamesExisting[key]->getOtherFields());
            mGamesFinal[key] = feGame;
            mGamesExisting.remove(key);
        }
        else
        {
            mGamesFinal[key] = std::move(mGamesExisting[key]);
            mGamesExisting.remove(key);
        }

    }
    else
        mGamesFinal[key] = feGame;
}

void PlatformDoc::addAddApp(FP::AddApp app)
{
    std::shared_ptr<AddApp> feApp = prepareAddApp(app);
    QUuid key = feApp->getID();

    // Check if additional app exists
    if(mAddAppsExisting.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
        {
            feApp->transferOtherFields(mAddAppsExisting[key]->getOtherFields());
            mAddAppsFinal[key] = feApp;
            mAddAppsExisting.remove(key);
        }
        else
        {
            mAddAppsFinal[key] = std::move(mAddAppsExisting[key]);
            mAddAppsExisting.remove(key);
        }

    }
    else
        mAddAppsFinal[key] = feApp;
}

void PlatformDoc::finalize()
{
    // Copy items to final list if obsolete entries are to be kept
    if(!mUpdateOptions.removeObsolete)
    {
        mGamesFinal.insert(mGamesExisting);
        mAddAppsFinal.insert(mAddAppsExisting);
    }

    // Clear existing lists
    mGamesExisting.clear();
    mAddAppsExisting.clear();

    // Perform frontend specific finalization
    cleanup();
}

//===============================================================================================================
// PlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDocReader::PlatformDocReader(PlatformDoc* targetDoc)
    : DataDocReader(targetDoc) {}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDocWriter::PlatformDocWriter(PlatformDoc* sourceDoc)
    : DataDocWriter(sourceDoc) {}


//===============================================================================================================
// PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDoc::PlaylistDoc(Install* parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions, const Key&)
    : DataDoc(parent, std::move(docFile), docName), mUpdateOptions(updateOptions) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlaylistDoc::type() const { return Type::Platform; }

//Public:
const std::shared_ptr<PlaylistHeader>& PlaylistDoc::getPlaylistHeader() const { return mPlaylistHeader; }
const QHash<QUuid, std::shared_ptr<PlaylistGame>>& PlaylistDoc::getFinalPlaylistGames() const { return mPlaylistGamesFinal; }

bool PlaylistDoc::containsPlaylistGame(QUuid gameID) const { return mPlaylistGamesFinal.contains(gameID) || mPlaylistGamesExisting.contains(gameID); }


void PlaylistDoc::setPlaylistHeader(FP::Playlist playlist)
{
    std::shared_ptr<PlaylistHeader> fePlaylistHeader = preparePlaylistHeader(playlist);

    fePlaylistHeader->transferOtherFields(mPlaylistHeader->getOtherFields());
    mPlaylistHeader = fePlaylistHeader;
}

void PlaylistDoc::addPlaylistGame(FP::PlaylistGame playlistGame)
{
    std::shared_ptr<PlaylistGame> fePlaylistGame = preparePlaylistGame(playlistGame);
    QUuid key = fePlaylistGame->getGameID();

    // Check if playlist game exists
    if(mPlaylistGamesExisting.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
        {
            fePlaylistGame->transferOtherFields(mPlaylistGamesExisting[key]->getOtherFields());
            //fePlaylistGame.setLBDatabaseID(mPlaylistGamesExisting[key].getLBDatabaseID()); // TODO: For LB specific child add this in
            mPlaylistGamesFinal[key] = fePlaylistGame;
            mPlaylistGamesExisting.remove(key);
        }
        else
        {
            mPlaylistGamesFinal[key] = std::move(mPlaylistGamesExisting[key]);
            mPlaylistGamesExisting.remove(key);
        }

    }
    else
    {
        //fePlaylistGame.setLBDatabaseID(mPlaylistGameFreeLBDBIDTracker->reserveFirstFree()); // TODO: For LB specific child add this in
        mPlaylistGamesFinal[key] = fePlaylistGame;
    }
}

void PlaylistDoc::finalize()
{
    // Copy items to final list if obsolete entries are to be kept
    if(!mUpdateOptions.removeObsolete)
        mPlaylistGamesFinal.insert(mPlaylistGamesExisting);

    // Clear existing lists
    mPlaylistGamesExisting.clear();

    // Perform frontend specific finalization
    cleanup();
}

//===============================================================================================================
// PlaylistDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDocReader::PlaylistDocReader(PlaylistDoc* targetDoc)
    : DataDocReader(targetDoc) {}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDocWriter::PlaylistDocWriter(PlaylistDoc* sourceDoc)
    : DataDocWriter(sourceDoc) {}

}
