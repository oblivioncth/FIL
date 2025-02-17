#ifndef LR_INSTALL_H
#define LR_INSTALL_H

// Qt Includes
#include <QDir>

// Project Includes
#include "launcher/interface/lr-install-interface.h"
#include "launcher/abstract/lr-registration.h"

namespace Lr
{

template<LauncherId Id>
class Install : public IInstall
{
//-Aliases-------------------------------------------------------------------------------------------------------------
protected:
    using PlatformT = Id::PlatformT;
    using PlatformReaderT = Id::PlatformReaderT;
    using PlatformWriterT = Id::PlatformWriterT;
    using PlaylistT = Id::PlaylistT;
    using PlaylistReaderT = Id::PlaylistReaderT;
    using PlaylistWriterT = Id::PlaylistWriterT;
    using GameT = Id::GameT;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    Install(const QString& installPath);

//-Instance Functions---------------------------------------------------------------------------------------------------
protected:
    // RE-IMPLEMENT
    using IInstall::nullify;

    // OPTIONALLY RE-IMPLEMENT
    virtual void preparePlatformDocCommit(const PlatformT& document); // Does nothing by default
    virtual void preparePlaylistDocCommit(const PlaylistT& document); // Does nothing by default

    // IMPLEMENT
    using IInstall::populateExistingDocs;
    virtual std::unique_ptr<PlatformT> preparePlatformDocCheckout(const QString& translatedName) = 0;
    virtual std::unique_ptr<PlaylistT> preparePlaylistDocCheckout(const QString& translatedName) = 0;

public:
    QString name() const override;

    DocHandlingError checkoutPlatformDoc(std::unique_ptr<IPlatformDoc>& returnBuffer, const QString& name) override;
    DocHandlingError checkoutPlaylistDoc(std::unique_ptr<IPlaylistDoc>& returnBuffer, const QString& name) override;
    DocHandlingError commitPlatformDoc(std::unique_ptr<IPlatformDoc> platformDoc) override;
    DocHandlingError commitPlaylistDoc(std::unique_ptr<IPlaylistDoc> playlistDoc) override;

    // RE-IMPLEMENT
    using IInstall::softReset;

    // IMPLEMENT
    using IInstall::preferredImageModeOrder;
    using IInstall::isRunning;
    using IInstall::processBulkImageSources; // Just do nothing if Reference mode isn't supported
    virtual void convertToDestinationImages(const GameT& game, Import::ImagePaths& images) = 0; // NOTE: One or both of the image paths provided here can be null (i.e. images unavailable).

    // OPTIONALLY RE-IMPLEMENT
    using IInstall::preImport;
    using IInstall::postImport;
    using IInstall::prePlatformsImport;
    using IInstall::postPlatformsImport;
    using IInstall::preImageProcessing;
    using IInstall::postImageProcessing;
    using IInstall::prePlaylistsImport;
    using IInstall::postPlaylistsImport;
    using IInstall::translateDocName;
    using IInstall::platformCategoryIconPath;
    using IInstall::platformIconsDirectory;
    using IInstall::playlistIconsDirectory;
};

}

#include "lr-install.tpp"
#endif // LR_INSTALL_H
