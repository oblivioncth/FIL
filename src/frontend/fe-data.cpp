// Unit Include
#include "fe-data.h"

// Project Includes
#include "fe-install.h"

namespace Fe
{
//-Doc Handling Error--------------------------------------------------------------------------------------------
namespace Dhe
{
    // Message Macros
    const QString M_DOC_TYPE = "<docType>";
    const QString M_DOC_NAME = "<docName>";
    const QString M_DOC_PARENT = "<docParent>";

    // Standard Errors
    static inline const QHash<DocHandlingError, QString> STD_ERRORS = {
        {DocHandlingError::DocAlreadyOpen, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is already open."},
        {DocHandlingError::DocCantOpen, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") cannot be opened."},
        {DocHandlingError::DocCantSave, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") cannot be saved."},
        {DocHandlingError::NotParentDoc, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is not a" + M_DOC_PARENT + "document."},
        {DocHandlingError::CantRemoveBackup, "The existing backup of the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") could not be removed."},
        {DocHandlingError::CantCreateBackup, "Could not create a backup of the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ")."},
        {DocHandlingError::DocInvalidType, "The document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is invalid or of the wrong type."},
        {DocHandlingError::DocReadFailed, "Reading the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") failed."},
        {DocHandlingError::DocWriteFailed, "Writing to the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") failed."}
    };
}

QString docHandlingErrorString(const DataDoc* doc, DocHandlingError handlingError)
{
    QString formattedError = Dhe::STD_ERRORS[handlingError];
    formattedError.replace(Dhe::M_DOC_TYPE, doc->identifier().docTypeString());
    formattedError.replace(Dhe::M_DOC_NAME, doc->identifier().docName());
    formattedError.replace(Dhe::M_DOC_PARENT, doc->parent()->name());

    return formattedError;
}

//===============================================================================================================
// ImageSources
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ImageSources::ImageSources() {}
ImageSources::ImageSources(const QString& logoPath, const QString& screenshotPath) :
    mLogoPath(logoPath),
    mScreenshotPath(screenshotPath)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool ImageSources::isNull() const { return mLogoPath.isEmpty() && mScreenshotPath.isEmpty(); }
QString ImageSources::logoPath() const { return mLogoPath; }
QString ImageSources::screenshotPath() const { return mScreenshotPath; };
void ImageSources::setLogoPath(const QString& path) { mLogoPath = path; }
void ImageSources::setScreenshotPath(const QString& path) { mScreenshotPath = path; }

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
DataDoc::DataDoc(Install* const parent, const QString& docPath, QString docName) :
      mParent(parent),
      mDocumentPath(docPath),
      mName(docName)
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
DataDoc::~DataDoc() {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Install* DataDoc::parent() const { return mParent; }
QString DataDoc::path() const { return mDocumentPath; }
DataDoc::Identifier DataDoc::identifier() const { return Identifier(type(), mName); }

//===============================================================================================================
// DataDoc::Reader
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
DataDoc::Reader::Reader(DataDoc* targetDoc) :
    mTargetDocument(targetDoc),
    mStdReadErrorStr(docHandlingErrorString(targetDoc, DocHandlingError::DocReadFailed))
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
DataDoc::Reader::~Reader() {}

//===============================================================================================================
// DataDoc::Writer
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
DataDoc::Writer::Writer(DataDoc* sourceDoc) :
    mSourceDocument(sourceDoc),
    mStdWriteErrorStr(docHandlingErrorString(sourceDoc, DocHandlingError::DocWriteFailed))
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
DataDoc::Writer::~Writer() {}

//===============================================================================================================
// Errorable
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
Errorable::Errorable() {}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
Errorable::~Errorable() {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
bool Errorable::hasError() const { return mError.isValid(); }
Qx::GenericError Errorable::error() const { return mError; }

//===============================================================================================================
// UpdateableDoc
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
UpdateableDoc::UpdateableDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions) :
    DataDoc(parent, docPath, docName),
    mUpdateOptions(updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
void UpdateableDoc::finalize() {} // Does nothing for base class

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlatformDoc::PlatformDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions) :
    UpdateableDoc(parent, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlatformDoc::type() const { return Type::Platform; }

//===============================================================================================================
// PlatformDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlatformDoc::Reader::Reader(DataDoc* targetDoc) :
    DataDoc::Reader(targetDoc)
{}

//===============================================================================================================
// PlatformDoc::Writer
//===============================================================================================================

PlatformDoc::Writer::Writer(DataDoc* sourceDoc) :
    DataDoc::Writer(sourceDoc)
{}

//===============================================================================================================
// BasicPlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlatformDoc::BasicPlatformDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions) :
    PlatformDoc(parent, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool BasicPlatformDoc::isEmpty() const
{
    return mGamesFinal.isEmpty() && mGamesExisting.isEmpty() && mAddAppsFinal.isEmpty() && mAddAppsExisting.isEmpty();
}

const QHash<QUuid, std::shared_ptr<Game>>& BasicPlatformDoc::finalGames() const { return mGamesFinal; }
const QHash<QUuid, std::shared_ptr<AddApp>>& BasicPlatformDoc::finalAddApps() const { return mAddAppsFinal; }

bool BasicPlatformDoc::containsGame(QUuid gameId) const { return mGamesFinal.contains(gameId) || mGamesExisting.contains(gameId); }
bool BasicPlatformDoc::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

void BasicPlatformDoc::addSet(const Fp::Set& set, const ImageSources& images)
{
    if(!mError.isValid())
    {
        // Prepare game
        std::shared_ptr<Game> feGame = prepareGame(set.game(), images);

        // Add game
        addUpdateableItem(mGamesExisting, mGamesFinal, feGame);

        // Handle additional apps
        for(const Fp::AddApp& addApp : set.addApps())
        {
            // Prepare
            std::shared_ptr<AddApp> feAddApp = prepareAddApp(addApp);

            // Add
            addUpdateableItem(mAddAppsExisting, mAddAppsFinal, feAddApp);
        }

        // Allow install to handle images if needed
        parent()->processDirectGameImages(feGame.get(), images);

    }
}

void BasicPlatformDoc::finalize()
{
    if(!mError.isValid())
    {
        // Finalize item stores
        finalizeUpdateableItems(mGamesExisting, mGamesFinal);
        finalizeUpdateableItems(mAddAppsExisting, mAddAppsFinal);

        // Perform base finalization
        UpdateableDoc::finalize();
    }
}

//===============================================================================================================
// BasicPlatformDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlatformDoc::Reader::Reader(DataDoc* targetDoc) :
    PlatformDoc::Reader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
/* TODO: Consider removing the following and similar, and just making public getters for existing items.
 *       Right now this is considered to break encapsulation too much, but it might not be that big of a deal
 *       and would be cleaner from a usability standpoint that doing this
 */
QHash<QUuid, std::shared_ptr<Game>>& BasicPlatformDoc::Reader::targetDocExistingGames()
{
    return static_cast<BasicPlatformDoc*>(mTargetDocument)->mGamesExisting;
}

QHash<QUuid, std::shared_ptr<AddApp>>& BasicPlatformDoc::Reader::targetDocExistingAddApps()
{
    return static_cast<BasicPlatformDoc*>(mTargetDocument)->mAddAppsExisting;
}

//===============================================================================================================
// BasicPlatformDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlatformDoc::Writer::Writer(DataDoc* sourceDoc) :
    PlatformDoc::Writer(sourceDoc)
{}

//===============================================================================================================
// PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDoc::PlaylistDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions) :
    UpdateableDoc(parent, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlaylistDoc::type() const { return Type::Platform; }

//===============================================================================================================
// PlaylistDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlaylistDoc::Reader::Reader(DataDoc* targetDoc) :
    DataDoc::Reader(targetDoc)
{}

//===============================================================================================================
// PlaylistDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
PlaylistDoc::Writer::Writer(DataDoc* sourceDoc) :
    DataDoc::Writer(sourceDoc)
{}

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
BasicPlaylistDoc::BasicPlaylistDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions) :
    PlaylistDoc(parent, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool BasicPlaylistDoc::isEmpty() const
{
    // The playlist header doesn't matter if there are no games
    return mPlaylistGamesFinal.isEmpty() && mPlaylistGamesExisting.isEmpty();
}

const std::shared_ptr<PlaylistHeader>& BasicPlaylistDoc::playlistHeader() const { return mPlaylistHeader; }
const QHash<QUuid, std::shared_ptr<PlaylistGame>>& BasicPlaylistDoc::finalPlaylistGames() const { return mPlaylistGamesFinal; }

bool BasicPlaylistDoc::containsPlaylistGame(QUuid gameId) const { return mPlaylistGamesFinal.contains(gameId) || mPlaylistGamesExisting.contains(gameId); }


void BasicPlaylistDoc::setPlaylistHeader(const Fp::Playlist& playlist)
{
    if(!mError.isValid())
    {
        std::shared_ptr<PlaylistHeader> fePlaylistHeader = preparePlaylistHeader(playlist);

        // Ensure doc already existed before transferring (null check)
        if(mPlaylistHeader)
            fePlaylistHeader->transferOtherFields(mPlaylistHeader->otherFields());

        // Set instance header to new one
        mPlaylistHeader = fePlaylistHeader;
    }
}

void BasicPlaylistDoc::addPlaylistGame(const Fp::PlaylistGame& playlistGame)
{
    if(!mError.isValid())
    {
        // Prepare playlist game
        std::shared_ptr<PlaylistGame> fePlaylistGame = preparePlaylistGame(playlistGame);

        // Add playlist game
        addUpdateableItem(mPlaylistGamesExisting, mPlaylistGamesFinal, fePlaylistGame);
    }
}

void BasicPlaylistDoc::finalize()
{
    if(!mError.isValid())
    {
        // Finalize item stores
        finalizeUpdateableItems(mPlaylistGamesExisting, mPlaylistGamesFinal);

        // Perform base finalization
        UpdateableDoc::finalize();
    }
}

//===============================================================================================================
// BasicPlaylistDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlaylistDoc::Reader::Reader(DataDoc* targetDoc) :
    PlaylistDoc::Reader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
QHash<QUuid, std::shared_ptr<PlaylistGame>>& BasicPlaylistDoc::Reader::targetDocExistingPlaylistGames()
{
    return static_cast<BasicPlaylistDoc*>(mTargetDocument)->mPlaylistGamesExisting;
}

std::shared_ptr<PlaylistHeader>& BasicPlaylistDoc::Reader::targetDocPlaylistHeader()
{
    return static_cast<BasicPlaylistDoc*>(mTargetDocument)->mPlaylistHeader;
}

//===============================================================================================================
// BasicPlaylistDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
BasicPlaylistDoc::Writer::Writer(DataDoc* sourceDoc) :
    PlaylistDoc::Writer(sourceDoc)
{}

}

