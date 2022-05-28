#ifndef LAUNCHBOX_INSTALL_H
#define LAUNCHBOX_INSTALL_H

// Qt Includes
#include <QString>
#include <QDir>
#include <QSet>
#include <QtXml>

// Project Includes
#include "../fe-install.h"
#include "lb-items.h"
#include "lb-data.h"

namespace Lb {

class Install : public Fe::Install
{
    friend class PlatformDoc;
    friend class PlaylistDoc;
//-Class Variables--------------------------------------------------------------------------------------------------
public:
    // Identity
    static inline const QString NAME = "LaunchBox";

    // Paths
    static inline const QString PLATFORMS_PATH = "Data/Platforms";
    static inline const QString PLAYLISTS_PATH = "Data/Playlists";
    static inline const QString DATA_PATH = "Data";
    static inline const QString MAIN_EXE_PATH = "LaunchBox.exe";
    static inline const QString PLATFORM_IMAGES_PATH = "Images";
    static inline const QString LOGO_PATH = "Box - Front";
    static inline const QString SCREENSHOT_PATH = "Screenshot - Gameplay";

    // Files
    static inline const QString XML_EXT = "xml";

    // Support
    static inline const QSet<Fe::Install::ImageMode> IMAGE_MODES {
        Fe::Install::ImageMode::Reference,
        Fe::Install::ImageMode::Copy,
        Fe::Install::ImageMode::Link
    };

//-Instance Variables-----------------------------------------------------------------------------------------------
private:

    // Files and directories
    QDir mDataDirectory;
    QDir mPlatformsDirectory;
    QDir mPlaylistsDirectory;
    QDir mPlatformImagesDirectory;

    // Other trackers
    Qx::FreeIndexTracker<int> mLbDatabaseIdTracker = Qx::FreeIndexTracker<int>(0, -1, {});
    QHash<QUuid, PlaylistGame::EntryDetails> mPlaylistGameDetailsCache;
    // TODO: Even though the playlist game IDs don't seem to matter, at some point for for completeness scan all playlists when hooking an install to get the
    // full list of in use IDs

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static QString makeFileNameLBKosher(QString fileName);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void nullifyDerived() override;
    void softResetDerived() override;

    QString dataDocPath(Fe::DataDoc::Identifier identifier) const override;

    std::shared_ptr<Fe::PlatformDocReader> prepareOpenPlatformDoc(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& name, const Fe::UpdateOptions& updateOptions) override;
    std::shared_ptr<Fe::PlaylistDocReader> prepareOpenPlaylistDoc(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& name, const Fe::UpdateOptions& updateOptions) override;
    std::shared_ptr<Fe::PlatformDocWriter> prepareSavePlatformDoc(const std::unique_ptr<Fe::PlatformDoc>& platformDoc) override;
    std::shared_ptr<Fe::PlaylistDocWriter> prepareSavePlaylistDoc(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc) override;

    Qx::GenericError openPlatformsDoc(std::unique_ptr<PlatformsDoc>& returnBuffer);
    Qx::GenericError savePlatformsDoc(std::unique_ptr<PlatformsDoc> document);

public:
    QString name() const override;
    QString executablePath() const override;
    ImageRefType imageRefType() const override;
    bool supportsImageMode(ImageMode imageMode) const override;

    Qx::GenericError populateExistingDocs(QStringList targetPlatforms, QStringList targetPlaylists) override;

    QString imageDestinationPath(Fp::ImageType imageType, const Fe::Game& game) const override;
    Qx::GenericError bulkReferenceImages(QString logoRootPath, QString screenshotRootPath, QStringList platforms) override;
};
REGISTER_FRONTEND(Install::NAME, Install);

}


#endif // LAUNCHBOX_INSTALL_H
