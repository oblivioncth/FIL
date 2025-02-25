#ifndef LAUNCHBOX_INSTALL_H
#define LAUNCHBOX_INSTALL_H

// Qt Includes
#include <QString>
#include <QDir>
#include <QSet>
#include <QIcon>
#include <QtXml>

// Project Includes
#include "launcher/abstract/lr-install.h"
#include "launcher/implementation/launchbox/lb-data.h"

namespace Lb {

class Install : public Lr::Install<LauncherId>
{
    friend class PlatformDoc; // TODO: See about removing the need for these (CLIfp path would need public accessor here)
    friend class PlaylistDoc;
//-Class Variables--------------------------------------------------------------------------------------------------
private:
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
    static inline const QList<Import::ImageMode> IMAGE_MODE_ORDER {
        Import::ImageMode::Link,
        Import::ImageMode::Copy,
        Import::ImageMode::Reference
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
    Qx::Error populateExistingDocs() override;

    // Image Processing
    QString imageDestinationPath(Fp::ImageType imageType, const Lr::Game& game) const;
    void editBulkImageReferences(const Import::ImagePaths& imageSources);

    // Doc handling
    QString dataDocPath(Lr::IDataDoc::Identifier identifier) const;
    std::unique_ptr<PlatformDoc> preparePlatformDocCheckout(const QString& translatedName) override;
    std::unique_ptr<PlaylistDoc> preparePlaylistDocCheckout(const QString& translatedName) override;

    Lr::DocHandlingError checkoutPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc>& returnBuffer);
    Lr::DocHandlingError commitPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc> document);
    Lr::DocHandlingError checkoutParentsDoc(std::unique_ptr<ParentsDoc>& returnBuffer);
    Lr::DocHandlingError commitParentsDoc(std::unique_ptr<ParentsDoc> document);

public:
    // Install management
    void softReset() override;

    // Info
    QList<Import::ImageMode> preferredImageModeOrder() const override;
    bool isRunning() const override;
    QString versionString() const override;
    QString translateDocName(const QString& originalName, Lr::IDataDoc::Type type) const override;

    // Import stage notifier hooks
    Qx::Error prePlatformsImport() override;
    Qx::Error postPlatformsImport() override;
    Qx::Error preImageProcessing() override;
    Qx::Error postImageProcessing() override;
    Qx::Error postPlaylistsImport() override;

    // Image handling
    void processBulkImageSources(const Import::ImagePaths& bulkSources) override;
    void convertToDestinationImages(const Game& game, Import::ImagePaths& images) override;
    QString platformCategoryIconPath() const override;
    std::optional<QDir> platformIconsDirectory() const override;
    std::optional<QDir> playlistIconsDirectory() const override;
};


}


#endif // LAUNCHBOX_INSTALL_H
