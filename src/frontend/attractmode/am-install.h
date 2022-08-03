#ifndef ATTRACTMODE_INSTALL_H
#define ATTRACTMODE_INSTALL_H

// Qx Includes
#include <qx/core/qx-versionnumber.h>

// Project Includes
#include "../fe-install.h"
#include "am-data.h"
#include "am-settings-data.h"

namespace Am
{

class Install : public Fe::Install
{
    friend class PlatformInterface;
    friend class PlaylistInterface;

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    // Identity
    static inline const QString NAME = "AttractMode";
    static inline const QString ICON_PATH = ":/frontend/AttractMode/icon.png";
    static inline const QUrl HELP_URL = QUrl("");

    // Naming
    static inline const QString PLATFORM_TAG_PREFIX = "[Platform] ";
    static inline const QString PLAYLIST_TAG_PREFIX = "[Playlist] ";

    // Paths
    static inline const QString EMULATORS_PATH = "emulators";
    static inline const QString ROMLISTS_PATH = "romlists";
    static inline const QString SCRAPER_PATH = "scraper";
    static inline const QString MAIN_CFG_PATH = "attract.cfg";
    static inline const QString MAIN_EXE_PATH = "attract.exe";
    static inline const QString CONSOLE_EXE_PATH = "attract-console.exe";

    // Sub paths
    static inline const QString LOGO_FOLDER_NAME = "flyer";
    static inline const QString SCREENSHOT_FOLDER_NAME = "snap";
    static inline const QString OVERVIEW_FOLDER_NAME = "overview";

    // Files
    static inline const QString TXT_EXT = "txt";
    static inline const QString TAG_EXT = "tag";
    static inline const QString CFG_EXT = "cfg";

    // Support
    static inline const QList<Fe::ImageMode> IMAGE_MODE_ORDER {
        Fe::ImageMode::Link,
        Fe::ImageMode::Copy
    };
    /*
     * NOTE: In order to support reference, thousands of folders would have to be added to the image search list which is likely impractical.
     * It could be attempted I suppose.
     */

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories - Required
    QDir mEmulatorsDirectory;
    QDir mRomlistsDirectory;
    QFile mMainConfigFile;

    // Files and directories - Optional
    QDir mFpTagDirectory;
    QDir mFpScraperDirectory;
    QFile mMainExe;
    QFile mConsoleExe;
    QFile mFpRomlist;
    QFile mEmulatorConfigFile;

    // Image transfers for import worker
    QList<ImageMap> mWorkerImageJobs;

    // Main romlist
    std::unique_ptr<Romlist> mRomlist;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Instance Functions-----------------------------------------------------------------------------------------------
private:
    // Install management
    void nullify() override;
    Qx::GenericError populateExistingDocs() override;
    QString translateDocName(const QString& originalName, Fe::DataDoc::Type type) const override;

    // Image Processing
    QString imageDestinationPath(Fp::ImageType imageType, const Fe::Game* game) const;

    // Doc handling
    std::shared_ptr<Fe::PlatformDoc::Reader> preparePlatformDocCheckout(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& translatedName) override;
    std::shared_ptr<Fe::PlaylistDoc::Reader> preparePlaylistDocCheckout(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& translatedName) override;
    std::shared_ptr<Fe::PlatformDoc::Writer> preparePlatformDocCommit(const std::unique_ptr<Fe::PlatformDoc>& platformDoc) override;
    std::shared_ptr<Fe::PlaylistDoc::Writer> preparePlaylistDocCommit(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc) override;

    Qx::GenericError checkoutMainConfig(std::unique_ptr<CrudeSettings>& returnBuffer);
    Qx::GenericError checkoutFlashpointRomlist(std::unique_ptr<Romlist>& returnBuffer);
    Qx::GenericError checkoutClifpEmulatorConfig(std::unique_ptr<Emulator>& returnBuffer);
    Qx::GenericError commitMainConfig(std::unique_ptr<CrudeSettings> document);
    Qx::GenericError commitFlashpointRomlist(std::unique_ptr<Romlist> document);
    Qx::GenericError commitClifpEmulatorConfig(std::unique_ptr<Emulator> document);

public:
    // Install management
    void softReset() override;

    // Info
    QString name() const override;
    QString executablePath() const override;
    QList<Fe::ImageMode> preferredImageModeOrder() const override;
    QString versionString() const override;

    // Import stage notifier hooks
    Qx::GenericError preImport(const ImportDetails& details) override;
    Qx::GenericError prePlatformsImport() override;
    Qx::GenericError postPlatformsImport() override;
    Qx::GenericError preImageProcessing(QList<ImageMap>& workerTransfers, Fe::ImageSources bulkSources) override;
    Qx::GenericError postImport() override;

    // Image handling
    void processDirectGameImages(const Fe::Game* game, const Fe::ImageSources& imageSources) override;
};
REGISTER_FRONTEND(Install::NAME, Install, &Install::ICON_PATH, &Install::HELP_URL);

}
#endif // ATTRACTMODE_INSTALL_H
