// Unit Include
#include "lr-data.h"

// Qx Includes
#include <qx/xml/qx-xmlstreamreadererror.h>
#include <qx/xml/qx-common-xml.h>

// Project Includes
#include "launcher/lr-install.h"

namespace Lr
{
//===============================================================================================================
// DocHandlingError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
DocHandlingError::DocHandlingError() :
    mType(NoError)
{}


DocHandlingError::DocHandlingError(const DataDoc& doc, Type t, const QString& s) :
    mType(t),
    mErrorStr(generatePrimaryString(doc, t)),
    mSpecific(s)
{}

//-Class Functions-------------------------------------------------------------
//Private:
QString DocHandlingError::generatePrimaryString(const DataDoc& doc, Type t)
{
    // TODO: Use Qx for this
    QString formattedError = ERR_STRINGS[t];
    formattedError.replace(M_DOC_TYPE, doc.identifier().docTypeString());
    formattedError.replace(M_DOC_NAME, doc.identifier().docName());
    formattedError.replace(M_DOC_PARENT, doc.parent()->name());

    return formattedError;
}

//-Instance Functions-------------------------------------------------------------
//Public:
bool DocHandlingError::isValid() const { return mType != NoError; }
QString DocHandlingError::errorString() const { return mErrorStr; }
QString DocHandlingError::specific() const { return mSpecific; }
DocHandlingError::Type DocHandlingError::type() const { return mType; }

//Private:
Qx::Severity DocHandlingError::deriveSeverity() const { return Qx::Critical; }
quint32 DocHandlingError::deriveValue() const { return mType; }
QString DocHandlingError::derivePrimary() const { return mErrorStr; }
QString DocHandlingError::deriveSecondary() const { return mSpecific; }

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
QString ImageSources::screenshotPath() const { return mScreenshotPath; }
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
    mTargetDocument(targetDoc)
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
    mSourceDocument(sourceDoc)
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
Qx::Error Errorable::error() const { return mError; }

//===============================================================================================================
// UpdateableDoc
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
UpdateableDoc::UpdateableDoc(Install* const parent, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
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
PlatformDoc::PlatformDoc(Install* const parent, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
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
BasicPlatformDoc::BasicPlatformDoc(Install* const parent, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
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
        std::shared_ptr<Game> lrGame = prepareGame(set.game(), images);

        // Add game
        addUpdateableItem(mGamesExisting, mGamesFinal, lrGame);

        // Handle additional apps
        for(const Fp::AddApp& addApp : set.addApps())
        {
            // Prepare
            std::shared_ptr<AddApp> lrAddApp = prepareAddApp(addApp);

            // Add
            addUpdateableItem(mAddAppsExisting, mAddAppsFinal, lrAddApp);
        }

        // Allow install to handle images if needed
        parent()->processDirectGameImages(lrGame.get(), images);

    }
}

void BasicPlatformDoc::finalize()
{
    if(!mError.isValid())
    {
        /* TODO: Have this (and all other implementations of finalize() do something like return
         * the IDs of titles that were removed, or otherwise populate an internal variable so that afterwards
         * the list can be used to purge all images or other title related files (like overviews with AM).
         * Right now only the data portion of old games is removed)
         */

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
PlaylistDoc::PlaylistDoc(Install* const parent, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    UpdateableDoc(parent, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
DataDoc::Type PlaylistDoc::type() const { return Type::Playlist; }

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
BasicPlaylistDoc::BasicPlaylistDoc(Install* const parent, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
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


void BasicPlaylistDoc::setPlaylistData(const Fp::Playlist& playlist)
{
    if(!mError.isValid())
    {
        std::shared_ptr<PlaylistHeader> lrPlaylistHeader = preparePlaylistHeader(playlist);

        // Ensure doc already existed before transferring (null check)
        if(mPlaylistHeader)
            lrPlaylistHeader->transferOtherFields(mPlaylistHeader->otherFields());

        // Set instance header to new one
        mPlaylistHeader = lrPlaylistHeader;

        for(const auto& plg : playlist.playlistGames())
        {
            // Prepare playlist game
            std::shared_ptr<PlaylistGame> lrPlaylistGame = preparePlaylistGame(plg);

            // Add playlist game
            addUpdateableItem(mPlaylistGamesExisting, mPlaylistGamesFinal, lrPlaylistGame);
        }
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

//===============================================================================================================
// XmlDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
XmlDocReader::XmlDocReader(DataDoc* targetDoc, const QString& root) :
    DataDoc::Reader(targetDoc),
    mXmlFile(targetDoc->path()),
    mStreamReader(&mXmlFile),
    mRootElement(root)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
DocHandlingError XmlDocReader::streamStatus() const
{
    if(mStreamReader.hasError())
    {
        Qx::XmlStreamReaderError xmlError(mStreamReader);
        return DocHandlingError(*mTargetDocument, DocHandlingError::DocReadFailed, xmlError.text());
    }

    return DocHandlingError();
}

//Public:
DocHandlingError XmlDocReader::readInto()
{
    // Open File
    if(!mXmlFile.open(QFile::ReadOnly))
        return DocHandlingError(*mTargetDocument, DocHandlingError::DocCantOpen, mXmlFile.errorString());

    if(!mStreamReader.readNextStartElement())
    {
        Qx::XmlStreamReaderError xmlError(mStreamReader);
        return DocHandlingError(*mTargetDocument, DocHandlingError::DocReadFailed, xmlError.text());
    }

    if(mStreamReader.name() != mRootElement)
        return DocHandlingError(*mTargetDocument, DocHandlingError::NotParentDoc);

    return readTargetDoc();

    // File is automatically closed when reader is destroyed...
}

//===============================================================================================================
// XmlDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
XmlDocWriter::XmlDocWriter(DataDoc* sourceDoc, const QString& root) :
    DataDoc::Writer(sourceDoc),
    mXmlFile(sourceDoc->path()),
    mStreamWriter(&mXmlFile),
    mRootElement(root)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
void XmlDocWriter::writeCleanTextElement(const QString& qualifiedName, const QString& text)
{
    if(text.isEmpty())
        mStreamWriter.writeEmptyElement(qualifiedName);
    else
        mStreamWriter.writeTextElement(qualifiedName, Qx::xmlSanitized(text));
}

void XmlDocWriter::writeOtherFields(const QHash<QString, QString>& otherFields)
{
    for(QHash<QString, QString>::const_iterator i = otherFields.constBegin(); i != otherFields.constEnd(); ++i)
        writeCleanTextElement(i.key(), i.value());
}

DocHandlingError XmlDocWriter::streamStatus() const
{
    return mStreamWriter.hasError() ? DocHandlingError(*mSourceDocument, DocHandlingError::DocWriteFailed, mStreamWriter.device()->errorString()) :
               DocHandlingError();
}

//Public:
DocHandlingError XmlDocWriter::writeOutOf()
{
    // Open File
    if(!mXmlFile.open(QFile::WriteOnly | QFile::Truncate)) // Discard previous contents
        return DocHandlingError(*mSourceDocument, DocHandlingError::DocCantSave, mXmlFile.errorString());

    // Enable auto formatting
    mStreamWriter.setAutoFormatting(true);
    mStreamWriter.setAutoFormattingIndent(2);

    // Write standard XML header
    mStreamWriter.writeStartDocument(u"1.0"_s, true);

    // Write main LaunchBox tag
    mStreamWriter.writeStartElement(mRootElement);

    // Write main body
    if(!writeSourceDoc())
        return streamStatus();

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return streamStatus();

    // File is automatically closed when writer is destroyed...
}

}

