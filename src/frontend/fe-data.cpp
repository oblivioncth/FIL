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
    seed = qHash(key.mDocType, seed);
    seed = qHash(key.mDocName, seed);

    return seed;
}

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
DataDoc::Identifier::Identifier(Type docType, QString docName) :
    mDocType(docType),
    mDocName(docName)
{}

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
DataDoc::DataDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName) :
      mParent(parent),
      mDocumentFile(std::move(docFile)),
      mName(docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
QString DataDoc::filePath() const { return mDocumentFile->fileName(); }

//Public:
Install* DataDoc::parent() const { return mParent; }
DataDoc::Identifier DataDoc::identifier() const { return Identifier(type(), mName); }
QString DataDoc::errorString(StandardError error) const
{
    QString formattedError = STD_ERRORS[error];
    formattedError.replace(M_DOC_TYPE, identifier().docTypeString());
    formattedError.replace(M_DOC_NAME, identifier().docName());
    formattedError.replace(M_DOC_PARENT, parent()->name());

    return formattedError;
}

void DataDoc::clearFile() { mDocumentFile->resize(0); }

//===============================================================================================================
// DataDocReader
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
DataDocReader::DataDocReader(DataDoc* targetDoc) :
    mTargetDocument(targetDoc),
    mPrimaryError(targetDoc->errorString(DataDoc::StandardError::DocReadFailed))
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
std::unique_ptr<QFile>& DataDocReader::targetDocFile() { return mTargetDocument->mDocumentFile; }
//===============================================================================================================
// DataDocWriter
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
DataDocWriter::DataDocWriter(DataDoc* sourceDoc) :
    mSourceDocument(sourceDoc),
    mPrimaryError(sourceDoc->errorString(DataDoc::StandardError::DocWriteFailed))
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
std::unique_ptr<QFile>& DataDocWriter::sourceDocFile() { return mSourceDocument->mDocumentFile; }

//===============================================================================================================
// UpdateableDoc
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
UpdateableDoc::UpdateableDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions) :
    DataDoc(parent, std::move(docFile), docName),
    mUpdateOptions(updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
void UpdateableDoc::finalize() { finalizeDerived(); }

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::PlatformDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions) :
    UpdateableDoc(parent, std::move(docFile), docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlatformDoc::type() const { return Type::Platform; }

//Public:
void PlatformDoc::setGameImageReference(FP::ImageType, QUuid, QString)
{
    throw new std::exception("UNSUPPORTED");
}

const QHash<QUuid, std::shared_ptr<Game>>& PlatformDoc::getFinalGames() const { return mGamesFinal; }
const QHash<QUuid, std::shared_ptr<AddApp>>& PlatformDoc::getFinalAddApps() const { return mAddAppsFinal; }

bool PlatformDoc::containsGame(QUuid gameID) const { return mGamesFinal.contains(gameID) || mGamesExisting.contains(gameID); }
bool PlatformDoc::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

const Game* PlatformDoc::addGame(FP::Game game)
{
    // Prepare game
    std::shared_ptr<Game> feGame = prepareGame(game);

    // Add game
    addUpdateableItem(mGamesExisting, mGamesFinal, feGame->getId(), feGame);

    // Return pointer to converted and added game
    return feGame.get();
}

const AddApp* PlatformDoc::addAddApp(FP::AddApp app)
{
    // Prepare game
    std::shared_ptr<AddApp> feAddApp = prepareAddApp(app);

    // Add game
    addUpdateableItem(mAddAppsExisting, mAddAppsFinal, feAddApp->getId(), feAddApp);

    // Return pointer to converted and added add app
    return feAddApp.get();
}

void PlatformDoc::finalize()
{
    // Finalize item stores
    finalizeUpdateableItems(mGamesExisting, mGamesFinal);
    finalizeUpdateableItems(mAddAppsExisting, mAddAppsFinal);

    // Perform base finalization
    UpdateableDoc::finalize();
}

//===============================================================================================================
// PlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlatformDocReader::PlatformDocReader(DataDoc* targetDoc) :
    DataDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
// TODO: Consider removing the following and similar, and just making public getters for existing items.
//       Right now this is considered to break encapsulation too much, but it might be alright
QHash<QUuid, std::shared_ptr<Game>>& PlatformDocReader::targetDocExistingGames()
{
    return static_cast<PlatformDoc*>(mTargetDocument)->mGamesExisting;
}

QHash<QUuid, std::shared_ptr<AddApp>>& PlatformDocReader::targetDocExistingAddApps()
{
    return static_cast<PlatformDoc*>(mTargetDocument)->mAddAppsExisting;
}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlatformDocWriter::PlatformDocWriter(DataDoc* sourceDoc) :
    DataDocWriter(sourceDoc)
{}

//===============================================================================================================
// PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDoc::PlaylistDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions) :
    UpdateableDoc(parent, std::move(docFile), docName, updateOptions)
{}

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
    // Prepare playlist game
    std::shared_ptr<PlaylistGame> fePlaylistGame = preparePlaylistGame(playlistGame);

    // Add playlist game
    addUpdateableItem(mPlaylistGamesExisting, mPlaylistGamesFinal, fePlaylistGame->getId(), fePlaylistGame);
}

void PlaylistDoc::finalize()
{
    // Finalize item stores
    finalizeUpdateableItems(mPlaylistGamesExisting, mPlaylistGamesFinal);

    // Perform base finalization
    UpdateableDoc::finalize();
}

//===============================================================================================================
// PlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlaylistDocReader::PlaylistDocReader(DataDoc* targetDoc) :
    DataDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:

QHash<QUuid, std::shared_ptr<PlaylistGame>>& PlaylistDocReader::targetDocExistingPlaylistGames()
{
    return static_cast<PlaylistDoc*>(mTargetDocument)->mPlaylistGamesExisting;
}

std::shared_ptr<PlaylistHeader>& PlaylistDocReader::targetDocPlaylistHeader()
{
    return static_cast<PlaylistDoc*>(mTargetDocument)->mPlaylistHeader;
}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlaylistDocWriter::PlaylistDocWriter(DataDoc* sourceDoc) :
    DataDocWriter(sourceDoc)
{}

}

