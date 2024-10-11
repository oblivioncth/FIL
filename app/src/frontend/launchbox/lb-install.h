#ifndef LAUNCHBOX_INSTALL_H
#define LAUNCHBOX_INSTALL_H

// Qt Includes
#include <QString>
#include <QDir>
#include <QSet>
#include <QIcon>
#include <QtXml>

// Project Includes
#include "frontend/fe-install.h"
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
    static inline const QString NAME = u"LaunchBox"_s;
    static inline const QString ICON_PATH = u":/frontend/LaunchBox/icon.svg"_s;
    static inline const QUrl HELP_URL = QUrl(u"https://forums.launchbox-app.com/files/file/2652-obbys-flashpoint-importer-for-launchbox"_s);

    // Paths
    static inline const QString PLATFORMS_PATH = u"Data/Platforms"_s;
    static inline const QString PLAYLISTS_PATH = u"Data/Playlists"_s;
    static inline const QString DATA_PATH = u"Data"_s;
    static inline const QString CORE_PATH = u"Core"_s;
    static inline const QString MAIN_EXE_PATH = u"Core/LaunchBox.exe"_s;
    static inline const QString PLATFORM_IMAGES_PATH = u"Images"_s;
    static inline const QString PLATFORM_ICONS_PATH = u"Images/Platform Icons/Platforms"_s;
    static inline const QString PLAYLIST_ICONS_PATH = u"Images/Platform Icons/Playlists"_s;
    static inline const QString PLATFORM_CATEGORY_ICONS_PATH = u"Images/Platform Icons/Platform Categories"_s;
    static inline const QString LOGO_PATH = u"Box - Front"_s;
    static inline const QString SCREENSHOT_PATH = u"Screenshot - Gameplay"_s;

    // Files
    static inline const QString XML_EXT = u"xml"_s;

    // Parents.xml
    static inline const QString MAIN_PLATFORM_CATEGORY = u"Flashpoint"_s;
    static inline const QString PLATFORMS_PLATFORM_CATEGORY = u"Flashpoint Platforms"_s;
    static inline const QString PLATFORMS_PLATFORM_CATEGORY_NESTED = u"Platforms"_s;
    static inline const QString PLAYLISTS_PLATFORM_CATEGORY = u"Flashpoint Playlists"_s;
    static inline const QString PLAYLISTS_PLATFORM_CATEGORY_NESTED = u"Playlists"_s;

    // Other
    static const quint64 LB_DB_ID_TRACKER_MAX = 100000;

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
    QDir mPlatformIconsDirectory;
    QDir mPlatformCategoryIconsDirectory;
    QDir mPlaylistIconsDirectory;
    QDir mCoreDirectory;
    QFileInfo mExeFile;

    // Image transfers for import worker
    QList<ImageMap> mWorkerImageJobs;

    // Persistent config handles
    std::unique_ptr<PlatformsConfigDoc> mPlatformsConfig;
    std::unique_ptr<ParentsDoc> mParents;

    // Other trackers
    Qx::FreeIndexTracker mLbDatabaseIdTracker = Qx::FreeIndexTracker(0, LB_DB_ID_TRACKER_MAX, {});
    QHash<QUuid, PlaylistGame::EntryDetails> mPlaylistGameDetailsCache;
    QSet<QUuid> mModifiedPlaylistIds;
    // TODO: Even though the playlist game IDs don't seem to matter, at some point for for completeness scan all playlists when hooking an install to get the
    // full list of in use IDs

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(const QString& installPath);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Install management
    void nullify() override;
    Qx::Error populateExistingDocs() override;

    // Image Processing
    QString imageDestinationPath(Fp::ImageType imageType, const Fe::Game* game) const;
    void editBulkImageReferences(const Fe::ImageSources& imageSources);

    // Doc handling
    QString dataDocPath(Fe::DataDoc::Identifier identifier) const;
    std::shared_ptr<Fe::PlatformDoc::Reader> preparePlatformDocCheckout(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& translatedName) override;
    std::shared_ptr<Fe::PlaylistDoc::Reader> preparePlaylistDocCheckout(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& translatedName) override;
    std::shared_ptr<Fe::PlatformDoc::Writer> preparePlatformDocCommit(const std::unique_ptr<Fe::PlatformDoc>& platformDoc) override;
    std::shared_ptr<Fe::PlaylistDoc::Writer> preparePlaylistDocCommit(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc) override;

    Fe::DocHandlingError checkoutPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc>& returnBuffer);
    Fe::DocHandlingError commitPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc> document);
    Fe::DocHandlingError checkoutParentsDoc(std::unique_ptr<ParentsDoc>& returnBuffer);
    Fe::DocHandlingError commitParentsDoc(std::unique_ptr<ParentsDoc> document);

public:
    // Install management
    void softReset() override;

    // Info
    QString name() const override;
    QList<Fe::ImageMode> preferredImageModeOrder() const override;
    bool isRunning() const override;
    QString versionString() const override;
    QString translateDocName(const QString& originalName, Fe::DataDoc::Type type) const override;

    // Import stage notifier hooks
    Qx::Error prePlatformsImport() override;
    Qx::Error postPlatformsImport() override;
    Qx::Error preImageProcessing(QList<ImageMap>& workerTransfers, const Fe::ImageSources& bulkSources) override;
    Qx::Error postImageProcessing() override;
    Qx::Error postPlaylistsImport() override;

    // Image handling
    void processDirectGameImages(const Fe::Game* game, const Fe::ImageSources& imageSources) override;
    QString platformCategoryIconPath() const override;
    std::optional<QDir> platformIconsDirectory() const override;
    std::optional<QDir> playlistIconsDirectory() const override;

};
REGISTER_FRONTEND(Install::NAME, Install, &Install::ICON_PATH, &Install::HELP_URL);

}


#endif // LAUNCHBOX_INSTALL_H
