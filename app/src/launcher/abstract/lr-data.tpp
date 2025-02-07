#ifndef LR_DATA_TPP
#define LR_DATA_TPP

#include "lr-data.h" // Ignore recursive error, doesn't actually cause problem

// Qx Includes
#include <qx/xml/qx-xmlstreamreadererror.h>
#include <qx/xml/qx-common-xml.h>

// Project Includes
#include "import/details.h"
#include "launcher/abstract/lr-install.h"

namespace Lr
{

//===============================================================================================================
// DataDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<LauncherId Id>
DataDoc<Id>::DataDoc(InstallT* install, const QString& docPath, QString docName) :
    IDataDoc(install, docPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<LauncherId Id>
Id::InstallT* DataDoc<Id>::install() const { return static_cast<InstallT*>(IDataDoc::install()); }

//===============================================================================================================
// DataDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
DataDocReader<DocT>::DataDocReader(DocT* targetDoc) :
    IDataDoc::Reader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
DocT* DataDocReader<DocT>::target() const { return static_cast<DocT*>(IDataDoc::Reader::target()); }

//===============================================================================================================
// DataDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
DataDocWriter<DocT>::DataDocWriter(DocT* sourceDoc) :
    IDataDoc::Writer(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
DocT* DataDocWriter<DocT>::source() const { return static_cast<DocT*>(IDataDoc::Writer::source()); }

//===============================================================================================================
// UpdateableDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<LauncherId Id>
UpdateableDoc<Id>::UpdateableDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    IUpdateableDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<LauncherId Id>
Id::InstallT* UpdateableDoc<Id>::install() const { return static_cast<InstallT*>(IDataDoc::install()); }

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<LauncherId Id>
PlatformDoc<Id>::PlatformDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    IPlatformDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<LauncherId Id>
Id::InstallT* PlatformDoc<Id>::install() const { return static_cast<InstallT*>(IDataDoc::install()); }

template<LauncherId Id>
void PlatformDoc<Id>::addSet(const Fp::Set& set, ImagePaths& images)
{
    if(!mError.isValid())
    {
        // Process set (technically this can fail and return nullptr)
        std::shared_ptr<GameT> game = processSet(set);

        /* Process single image if applicable.
         *
         * The derived install type will not be defined at this point so we must access install() via
         * the abstract base type.
         */
        auto install = static_cast<Install<Id>*>(IPlatformDoc::install());
        if(game && Import::Details::current().imageMode != Import::ImageMode::Reference)
            install->convertToDestinationImages(*game, images);
    }
}

//===============================================================================================================
// PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<LauncherId Id>
PlaylistDoc<Id>::PlaylistDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    IPlaylistDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<LauncherId Id>
Id::InstallT* PlaylistDoc<Id>::install() const { return static_cast<InstallT*>(IDataDoc::install()); }

//===============================================================================================================
// BasicPlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<LauncherId Id>
BasicPlatformDoc<Id>::BasicPlatformDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    PlatformDoc<Id>(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<LauncherId Id>
Id::InstallT* BasicPlatformDoc<Id>::install() const { return static_cast<InstallT*>(IDataDoc::install()); }

template<LauncherId Id>
const QHash<QUuid, std::shared_ptr<typename Id::GameT>>& BasicPlatformDoc<Id>::finalGames() const { return mGamesFinal; }

template<LauncherId Id>
const QHash<QUuid, std::shared_ptr<typename Id::AddAppT>>& BasicPlatformDoc<Id>::finalAddApps() const { return mAddAppsFinal; }

template<LauncherId Id>
bool BasicPlatformDoc<Id>::containsGame(QUuid gameId) const { return mGamesFinal.contains(gameId) || mGamesExisting.contains(gameId); }

template<LauncherId Id>
bool BasicPlatformDoc<Id>::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

template<LauncherId Id>
std::shared_ptr<typename Id::GameT> BasicPlatformDoc<Id>::processSet(const Fp::Set& set)
{
    std::shared_ptr<GameT> game;
    if(!mError.isValid())
    {
        // Prepare game
        game = prepareGame(set.game());

        // Add game
        addUpdateableItem(mGamesExisting, mGamesFinal, game);

        // Handle additional apps
        for(const Fp::AddApp& addApp : set.addApps())
        {
            // Prepare
            std::shared_ptr<AddAppT> lrAddApp = prepareAddApp(addApp);

            // Add
            addUpdateableItem(mAddAppsExisting, mAddAppsFinal, lrAddApp);
        }
    }
    return game;
}

template<LauncherId Id>
void BasicPlatformDoc<Id>::finalize()
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
        IUpdateableDoc::finalize();
    }
}

template<LauncherId Id>
bool BasicPlatformDoc<Id>::isEmpty() const
{
    return mGamesFinal.isEmpty() && mGamesExisting.isEmpty() && mAddAppsFinal.isEmpty() && mAddAppsExisting.isEmpty();
}

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
template<LauncherId Id>
BasicPlaylistDoc<Id>::BasicPlaylistDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    PlaylistDoc<Id>(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<LauncherId Id>
Id::InstallT* BasicPlaylistDoc<Id>::install() const { return static_cast<InstallT*>(IDataDoc::install()); }

template<LauncherId Id>
const std::shared_ptr<typename Id::PlaylistHeaderT>& BasicPlaylistDoc<Id>::playlistHeader() const { return mPlaylistHeader; }

template<LauncherId Id>
const QHash<QUuid, std::shared_ptr<typename Id::PlaylistGameT>>& BasicPlaylistDoc<Id>::finalPlaylistGames() const { return mPlaylistGamesFinal; }

template<LauncherId Id>
bool BasicPlaylistDoc<Id>::containsPlaylistGame(QUuid gameId) const { return mPlaylistGamesFinal.contains(gameId) || mPlaylistGamesExisting.contains(gameId); }

template<LauncherId Id>
void BasicPlaylistDoc<Id>::setPlaylistData(const Fp::Playlist& playlist)
{
    if(!mError.isValid())
    {
        std::shared_ptr<PlaylistHeaderT> lrPlaylistHeader = preparePlaylistHeader(playlist);

        // Ensure doc already existed before transferring (null check)
        if(mPlaylistHeader)
            lrPlaylistHeader->transferOtherFields(mPlaylistHeader->otherFields());

        // Set instance header to new one
        mPlaylistHeader = lrPlaylistHeader;

        for(const auto& plg : playlist.playlistGames())
        {
            // Prepare playlist game
            std::shared_ptr<PlaylistGameT> lrPlaylistGame = preparePlaylistGame(plg);

            // Add playlist game
            addUpdateableItem(mPlaylistGamesExisting, mPlaylistGamesFinal, lrPlaylistGame);
        }
    }
}

template<LauncherId Id>
void BasicPlaylistDoc<Id>::finalize()
{
    if(!mError.isValid())
    {
        // Finalize item stores
        finalizeUpdateableItems(mPlaylistGamesExisting, mPlaylistGamesFinal);

        // Perform base finalization
        IUpdateableDoc::finalize();
    }
}

template<LauncherId Id>
bool BasicPlaylistDoc<Id>::isEmpty() const
{
    // The playlist header doesn't matter if there are no games
    return mPlaylistGamesFinal.isEmpty() && mPlaylistGamesExisting.isEmpty();
}

//===============================================================================================================
// XmlDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
template<class DocT>
XmlDocReader<DocT>::XmlDocReader(DocT* targetDoc, const QString& root) :
    DataDocReader<DocT>(targetDoc),
    mXmlFile(targetDoc->path()),
    mStreamReader(&mXmlFile),
    mRootElement(root)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
DocHandlingError XmlDocReader<DocT>::streamStatus() const
{
    if(mStreamReader.hasError())
    {
        Qx::XmlStreamReaderError xmlError(mStreamReader);
        return DocHandlingError(*target(), DocHandlingError::DocReadFailed, xmlError.text());
    }

    return DocHandlingError();
}

//Public:
template<class DocT>
DocHandlingError XmlDocReader<DocT>::readInto()
{
    // Open File
    if(!mXmlFile.open(QFile::ReadOnly))
        return DocHandlingError(*target(), DocHandlingError::DocCantOpen, mXmlFile.errorString());

    if(!mStreamReader.readNextStartElement())
    {
        Qx::XmlStreamReaderError xmlError(mStreamReader);
        return DocHandlingError(*target(), DocHandlingError::DocReadFailed, xmlError.text());
    }

    if(mStreamReader.name() != mRootElement)
        return DocHandlingError(*target(), DocHandlingError::NotParentDoc);

    return readTargetDoc();

    // File is automatically closed when reader is destroyed...
}

//===============================================================================================================
// XmlDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
template<class DocT>
XmlDocWriter<DocT>::XmlDocWriter(DocT* sourceDoc, const QString& root) :
    DataDocWriter<DocT>(sourceDoc),
    mXmlFile(sourceDoc->path()),
    mStreamWriter(&mXmlFile),
    mRootElement(root)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
void XmlDocWriter<DocT>::writeCleanTextElement(const QString& qualifiedName, const QString& text)
{
    if(text.isEmpty())
        mStreamWriter.writeEmptyElement(qualifiedName);
    else
        mStreamWriter.writeTextElement(qualifiedName, Qx::xmlSanitized(text));
}

template<class DocT>
void XmlDocWriter<DocT>::writeOtherFields(const QHash<QString, QString>& otherFields)
{
    for(QHash<QString, QString>::const_iterator i = otherFields.constBegin(); i != otherFields.constEnd(); ++i)
        writeCleanTextElement(i.key(), i.value());
}

template<class DocT>
DocHandlingError XmlDocWriter<DocT>::streamStatus() const
{
    return mStreamWriter.hasError() ? DocHandlingError(*source(), DocHandlingError::DocWriteFailed, mStreamWriter.device()->errorString()) :
               DocHandlingError();
}

//Public:
template<class DocT>
DocHandlingError XmlDocWriter<DocT>::writeOutOf()
{
    // Open File
    if(!mXmlFile.open(QFile::WriteOnly | QFile::Truncate)) // Discard previous contents
        return DocHandlingError(*source(), DocHandlingError::DocCantSave, mXmlFile.errorString());

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

#endif // LR_DATA_TPP
