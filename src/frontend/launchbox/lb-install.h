#ifndef LAUNCHBOX_INSTALL_H
#define LAUNCHBOX_INSTALL_H

// Qt Includes
#include <QString>
#include <QDir>
#include <QSet>
#include <QIcon>
#include <QtXml>

// Project Includes
#include "../fe-install.h"
#include "lb-items.h"
#include "lb-data.h"

namespace Lb {

class Install : public Fe::Install
{
    friend class PlatformDoc; // TODO: See about removing the need for these (CLIfp path would need public accessor here)
    friend class PlaylistDoc;
//-Class Variables--------------------------------------------------------------------------------------------------
public:
    // Identity
    static inline const QString NAME = "LaunchBox";
    static inline const QString ICON_PATH = ":/frontend/LaunchBox/icon.svg";
    static inline const QUrl HELP_URL = QUrl("https://forums.launchbox-app.com/files/file/2652-obbys-flashpoint-importer-for-launchbox");

    // Paths
    static inline const QString PLATFORMS_PATH = "Data/Platforms";
    static inline const QString PLAYLISTS_PATH = "Data/Playlists";
    static inline const QString DATA_PATH = "Data";
    static inline const QString CORE_PATH = "Core";
    static inline const QString MAIN_EXE_PATH = "LaunchBox.exe";
    static inline const QString PLATFORM_IMAGES_PATH = "Images";
    static inline const QString LOGO_PATH = "Box - Front";
    static inline const QString SCREENSHOT_PATH = "Screenshot - Gameplay";

    // Files
    static inline const QString XML_EXT = "xml";

    // Support
    static inline const QList<Fe::ImageMode> IMAGE_MODE_ORDER {
        Fe::ImageMode::Link,
        Fe::ImageMode::Copy,
        Fe::ImageMode::Reference
    };

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mDataDirectory;
    QDir mPlatformsDirectory;
    QDir mPlaylistsDirectory;
    QDir mPlatformImagesDirectory;
    QDir mCoreDirectory;

    // Image transfers for import worker
    QList<ImageMap> mWorkerImageJobs;

    // Other trackers
    Qx::FreeIndexTracker<int> mLbDatabaseIdTracker = Qx::FreeIndexTracker<int>(0, -1, {});
    QHash<QUuid, PlaylistGame::EntryDetails> mPlaylistGameDetailsCache;
    // TODO: Even though the playlist game IDs don't seem to matter, at some point for for completeness scan all playlists when hooking an install to get the
    // full list of in use IDs

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Install management
    void nullify() override;
    Qx::GenericError populateExistingDocs() override;
    QString translateDocName(const QString& originalName, Fe::DataDoc::Type type) const override;

    // Image Processing
    QString imageDestinationPath(Fp::ImageType imageType, const Fe::Game* game) const;
    Qx::GenericError editBulkImageReferences(const Fe::ImageSources& imageSources);

    // Doc handling
    QString dataDocPath(Fe::DataDoc::Identifier identifier) const;
    std::shared_ptr<Fe::PlatformDoc::Reader> preparePlatformDocCheckout(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& translatedName) override;
    std::shared_ptr<Fe::PlaylistDoc::Reader> preparePlaylistDocCheckout(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& translatedName) override;
    std::shared_ptr<Fe::PlatformDoc::Writer> preparePlatformDocCommit(const std::unique_ptr<Fe::PlatformDoc>& platformDoc) override;
    std::shared_ptr<Fe::PlaylistDoc::Writer> preparePlaylistDocCommit(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc) override;

    Qx::GenericError checkoutPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc>& returnBuffer);
    Qx::GenericError commitPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc> document);

public:
    // Install management
    void softReset() override;

    // Info
    QString name() const override;
    QString executablePath() const override;
    QList<Fe::ImageMode> preferredImageModeOrder() const override;
    QString versionString() const override;

    // Import stage notifier hooks
    Qx::GenericError preImageProcessing(QList<ImageMap>& workerTransfers, Fe::ImageSources bulkSources) override;

    // Image handling
    void processDirectGameImages(const Fe::Game* game, const Fe::ImageSources& imageSources) override;
};
REGISTER_FRONTEND(Install::NAME, Install, &Install::ICON_PATH, &Install::HELP_URL);

}


#endif // LAUNCHBOX_INSTALL_H
