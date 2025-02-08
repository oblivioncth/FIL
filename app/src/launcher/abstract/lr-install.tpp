#ifndef LR_INSTALL_TPP
#define LR_INSTALL_TPP

#include "lr-install.h" // Can ignore recursion warning

// Qx Includes
#include <qx/xml/qx-xmlstreamreadererror.h>
#include <qx/xml/qx-common-xml.h>

#ifndef LR_INSTALL_H
#error __FILE__ should only be included from lr-install.h.
#endif // LR_INSTALL_H

namespace Lr
{

//===============================================================================================================
// Install
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
template<LauncherId Id>
Install<Id>::Install(const QString& installPath) :
    IInstall(installPath)
{}

//-Instance Functions--------------------------------------------------------------------------------------------
//Protected:
template<LauncherId Id>
void Install<Id>::preparePlatformDocCommit(const PlatformT& document)
{
    Q_UNUSED(document);
}

template<LauncherId Id>
void Install<Id>::preparePlaylistDocCommit(const PlaylistT& document)
{
    Q_UNUSED(document);
}

//Public:
template<LauncherId Id>
QString Install<Id>::name() const { return StaticRegistry<Id>::name().toString(); }

template<LauncherId Id>
DocHandlingError Install<Id>::checkoutPlatformDoc(std::unique_ptr<IPlatformDoc>& returnBuffer, const QString& name)
{
    // Translate to launcher doc name
    QString translatedName = translateDocName(name, IDataDoc::Type::Platform);

    // Get initialized blank doc and create reader if present
    std::unique_ptr<PlatformT> platformDoc = preparePlatformDocCheckout(translatedName);
    std::shared_ptr<IPlatformDoc::Reader> docReader;
    if constexpr(HasPlatformReader<Id>)
        docReader = std::make_shared<PlatformReaderT>(platformDoc.get());

    // Open document
    DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Fill return buffer on success
    if(!readErrorStatus.isValid())
        returnBuffer = std::move(platformDoc);

    // Return status
    return readErrorStatus;
}

template<LauncherId Id>
DocHandlingError Install<Id>::checkoutPlaylistDoc(std::unique_ptr<IPlaylistDoc>& returnBuffer, const QString& name)
{
    // Translate to launcher doc name
    QString translatedName = translateDocName(name, IDataDoc::Type::Playlist);

    // Get initialized blank doc and create reader if present
    std::unique_ptr<PlaylistT> playlistDoc = preparePlaylistDocCheckout(translatedName);
    std::shared_ptr<IPlatformDoc::Reader> docReader;
    if constexpr(HasPlaylistReader<Id>)
        docReader = std::make_shared<PlaylistReaderT>(playlistDoc.get());

    // Open document
    DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Fill return buffer on success
    if(!readErrorStatus.isValid())
        returnBuffer = std::move(playlistDoc);

    // Return status
    return readErrorStatus;
}

template<LauncherId Id>
DocHandlingError Install<Id>::commitPlatformDoc(std::unique_ptr<IPlatformDoc> document)
{
    // Doc should belong to this install
    Q_ASSERT(document->install() == this);

    // Work with native type
    auto nativeDoc = static_cast<PlatformT*>(document.get());

    // Handle any preparations
    preparePlatformDocCommit(*nativeDoc);

    // Write
    std::shared_ptr<IPlatformDoc::Writer> docWriter = std::make_shared<PlatformWriterT>(nativeDoc);
    DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

template<LauncherId Id>
DocHandlingError Install<Id>::commitPlaylistDoc(std::unique_ptr<IPlaylistDoc> document)
{
    // Doc should belong to this install
    Q_ASSERT(document->install() == this);

    // Work with native type
    auto nativeDoc = static_cast<PlaylistT*>(document.get());

    // Handle any preparations
    preparePlaylistDocCommit(*nativeDoc);

    // Write
    std::shared_ptr<IPlaylistDoc::Writer> docWriter = std::make_shared<PlaylistWriterT>(nativeDoc);
    DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

}

#endif // LR_INSTALL_TPP
