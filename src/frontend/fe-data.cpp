// Unit Include
#include "fe-data.h"

// Project Includes
#include "fe-install.h"

namespace Fe
{

//===============================================================================================================
// DataDoc::Identifier
//===============================================================================================================

//-Operators----------------------------------------------------------------------------------------------------
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

bool DataDoc::clearFile(){ return mDocumentFile->resize(0); } // resize() automatically seeks to new end

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
//Private:
void UpdateableDoc::finalizeDerived() {} // Empty default implementation so it does nothing, but is opt in for derived classes

//Public:
void UpdateableDoc::finalize() { finalizeDerived(); }

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlatformDoc::PlatformDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions) :
    UpdateableDoc(parent, std::move(docFile), docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlatformDoc::type() const { return Type::Platform; }

//Public:
void PlatformDoc::setGameImageReference(Fp::ImageType, QUuid, QString)
{
    throw new std::exception("UNSUPPORTED");
}

//===============================================================================================================
// BasicPlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlatformDoc::BasicPlatformDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions) :
    PlatformDoc(parent, std::move(docFile), docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const QHash<QUuid, std::shared_ptr<Game>>& BasicPlatformDoc::getFinalGames() const { return mGamesFinal; }
const QHash<QUuid, std::shared_ptr<AddApp>>& BasicPlatformDoc::getFinalAddApps() const { return mAddAppsFinal; }

bool BasicPlatformDoc::containsGame(QUuid gameId) const { return mGamesFinal.contains(gameId) || mGamesExisting.contains(gameId); }
bool BasicPlatformDoc::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

const Game* BasicPlatformDoc::addGame(Fp::Game game)
{
    // Prepare game
    std::shared_ptr<Game> feGame = prepareGame(game);

    // Add game
    addUpdateableItem(mGamesExisting, mGamesFinal, feGame->getId(), feGame);

    // Return pointer to converted and added game
    return feGame.get();
}

void BasicPlatformDoc::addAddApp(Fp::AddApp app)
{
    // Prepare game
    std::shared_ptr<AddApp> feAddApp = prepareAddApp(app);

    // Add game
    addUpdateableItem(mAddAppsExisting, mAddAppsFinal, feAddApp->getId(), feAddApp);
}

void BasicPlatformDoc::finalize()
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

//===============================================================================================================
// BasicPlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlatformDocReader::BasicPlatformDocReader(DataDoc* targetDoc) :
    PlatformDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
// TODO: Consider removing the following and similar, and just making public getters for existing items.
//       Right now this is considered to break encapsulation too much, but it might be alright
QHash<QUuid, std::shared_ptr<Game>>& BasicPlatformDocReader::targetDocExistingGames()
{
    return static_cast<BasicPlatformDoc*>(mTargetDocument)->mGamesExisting;
}

QHash<QUuid, std::shared_ptr<AddApp>>& BasicPlatformDocReader::targetDocExistingAddApps()
{
    return static_cast<BasicPlatformDoc*>(mTargetDocument)->mAddAppsExisting;
}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

PlatformDocWriter::PlatformDocWriter(DataDoc* sourceDoc) :
    DataDocWriter(sourceDoc)
{}

//===============================================================================================================
// BasicPlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlatformDocWriter::BasicPlatformDocWriter(DataDoc* sourceDoc) :
    PlatformDocWriter(sourceDoc)
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

//===============================================================================================================
// BasicPlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
/* NOTE: Right now mPlaylistHeader is left uninitialized (unless done so explicitly by a derivative). This is fine,
 * as currently 'void PlaylistDoc::setPlaylistHeader(Fp::Playlist playlist)' checks to see if an existing header
 * is present before performing a field transfer (i.e. in case the playlist doc didn't already exist); however,
 * if more parts of the process end up needing to interact with a doc that has a potentially null playlist header,
 * it may be better to require a value for it in this base class' constructor so that all derivatives must provide
 * a default (likely null/empty) playlist header.
 */
BasicPlaylistDoc::BasicPlaylistDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions) :
    PlaylistDoc(parent, std::move(docFile), docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const std::shared_ptr<PlaylistHeader>& BasicPlaylistDoc::getPlaylistHeader() const { return mPlaylistHeader; }
const QHash<QUuid, std::shared_ptr<PlaylistGame>>& BasicPlaylistDoc::getFinalPlaylistGames() const { return mPlaylistGamesFinal; }

bool BasicPlaylistDoc::containsPlaylistGame(QUuid gameId) const { return mPlaylistGamesFinal.contains(gameId) || mPlaylistGamesExisting.contains(gameId); }


void BasicPlaylistDoc::setPlaylistHeader(Fp::Playlist playlist)
{
    std::shared_ptr<PlaylistHeader> fePlaylistHeader = preparePlaylistHeader(playlist);

    // Ensure doc already existed before transferring (null check)
    if(mPlaylistHeader)
        fePlaylistHeader->transferOtherFields(mPlaylistHeader->getOtherFields());

    // Set instance header to new one
    mPlaylistHeader = fePlaylistHeader;
}

void BasicPlaylistDoc::addPlaylistGame(Fp::PlaylistGame playlistGame)
{
    // Prepare playlist game
    std::shared_ptr<PlaylistGame> fePlaylistGame = preparePlaylistGame(playlistGame);

    // Add playlist game
    addUpdateableItem(mPlaylistGamesExisting, mPlaylistGamesFinal, fePlaylistGame->getId(), fePlaylistGame);
}

void BasicPlaylistDoc::finalize()
{
    // Finalize item stores
    finalizeUpdateableItems(mPlaylistGamesExisting, mPlaylistGamesFinal);

    // Perform base finalization
    UpdateableDoc::finalize();
}

//===============================================================================================================
// PlaylistDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlaylistDocReader::PlaylistDocReader(DataDoc* targetDoc) :
    DataDocReader(targetDoc)
{}

//===============================================================================================================
// BasicPlaylistDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlaylistDocReader::BasicPlaylistDocReader(DataDoc* targetDoc) :
    PlaylistDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
QHash<QUuid, std::shared_ptr<PlaylistGame>>& BasicPlaylistDocReader::targetDocExistingPlaylistGames()
{
    return static_cast<BasicPlaylistDoc*>(mTargetDocument)->mPlaylistGamesExisting;
}

std::shared_ptr<PlaylistHeader>& BasicPlaylistDocReader::targetDocPlaylistHeader()
{
    return static_cast<BasicPlaylistDoc*>(mTargetDocument)->mPlaylistHeader;
}

//===============================================================================================================
// PlaylistDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlaylistDocWriter::PlaylistDocWriter(DataDoc* sourceDoc) :
    DataDocWriter(sourceDoc)
{}

//===============================================================================================================
// BasicPlaylistDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlaylistDocWriter::BasicPlaylistDocWriter(DataDoc* sourceDoc) :
    PlaylistDocWriter(sourceDoc)
{}

}

