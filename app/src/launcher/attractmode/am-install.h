#ifndef ATTRACTMODE_INSTALL_H
#define ATTRACTMODE_INSTALL_H

// Qx Includes
#include <qx/core/qx-versionnumber.h>

// Project Includes
#include "launcher/lr-install.h"
#include "launcher/attractmode/am-data.h"
#include "launcher/attractmode/am-settings-data.h"

namespace Am
{

class Install : public Lr::Install
{
    friend class PlatformInterface;
    friend class PlaylistInterface;

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    // Identity
    static inline const QString NAME = u"AttractMode"_s;
    static inline const QString ICON_PATH = u":/launcher/AttractMode/icon.png"_s;
    static inline const QUrl HELP_URL = QUrl(u""_s);

    // Naming
    static inline const QString PLATFORM_TAG_PREFIX = u"[Platform] "_s;
    static inline const QString PLAYLIST_TAG_PREFIX = u"[Playlist] "_s;

    // Paths
    static inline const QString EMULATORS_PATH = u"emulators"_s;
    static inline const QString ROMLISTS_PATH = u"romlists"_s;
    static inline const QString SCRAPER_PATH = u"scraper"_s;
    static inline const QString MAIN_CFG_PATH = u"attract.cfg"_s;
#ifdef _WIN32
    static inline const QString MAIN_EXE_PATH = u"attract.exe"_s;
#else
    static inline const QString MAIN_EXE_PATH = u"attract"_s;
#endif
    static inline const QString CONSOLE_EXE_PATH = u"attract-console.exe"_s; // Removed in newer versions

    // Sub paths
    static inline const QString LOGO_FOLDER_NAME = u"flyer"_s;
    static inline const QString SCREENSHOT_FOLDER_NAME = u"snap"_s;
    static inline const QString OVERVIEW_FOLDER_NAME = u"overview"_s;
    static inline const QString MARQUEE_FOLDER_NAME = u"marquee"_s;

    // Files
    static inline const QString TXT_EXT = u"txt"_s;
    static inline const QString TAG_EXT = u"tag"_s;
    static inline const QString CFG_EXT = u"cfg"_s;

    // Support
    static inline const QList<Import::ImageMode> IMAGE_MODE_ORDER {
        Import::ImageMode::Link,
        Import::ImageMode::Copy
    };
    /*
     * NOTE: In order to support reference, thousands of folders would have to be added to the image search list which is likely impractical.
     * It could be attempted I suppose.
     */

    // Extra
    static inline const QString MARQUEE_PATH = u":/launcher/AttractMode/marquee.png"_s;

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
    QFile mConsoleExe; // Removed in newer versions
    QFile mFpRomlist;
    QFile mEmulatorConfigFile;

    // Image transfers for import worker
    QList<ImageMap> mWorkerImageJobs;

    // Main romlist
    std::unique_ptr<Romlist> mRomlist;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(const QString& installPath);

//-Instance Functions-----------------------------------------------------------------------------------------------
private:
    // Install management
    void nullify() override;
    Qx::Error populateExistingDocs() override;
    QString versionFromExecutable() const;

    // Image Processing
    QString imageDestinationPath(Fp::ImageType imageType, const Lr::Game* game) const;

    // Doc handling
    std::shared_ptr<Lr::PlatformDoc::Reader> preparePlatformDocCheckout(std::unique_ptr<Lr::PlatformDoc>& platformDoc, const QString& translatedName) override;
    std::shared_ptr<Lr::PlaylistDoc::Reader> preparePlaylistDocCheckout(std::unique_ptr<Lr::PlaylistDoc>& playlistDoc, const QString& translatedName) override;
    std::shared_ptr<Lr::PlatformDoc::Writer> preparePlatformDocCommit(const std::unique_ptr<Lr::PlatformDoc>& platformDoc) override;
    std::shared_ptr<Lr::PlaylistDoc::Writer> preparePlaylistDocCommit(const std::unique_ptr<Lr::PlaylistDoc>& playlistDoc) override;

    Lr::DocHandlingError checkoutMainConfig(std::unique_ptr<CrudeSettings>& returnBuffer);
    Lr::DocHandlingError checkoutFlashpointRomlist(std::unique_ptr<Romlist>& returnBuffer);
    Lr::DocHandlingError checkoutClifpEmulatorConfig(std::unique_ptr<Emulator>& returnBuffer);
    Lr::DocHandlingError commitMainConfig(std::unique_ptr<CrudeSettings> document);
    Lr::DocHandlingError commitFlashpointRomlist(std::unique_ptr<Romlist> document);
    Lr::DocHandlingError commitClifpEmulatorConfig(std::unique_ptr<Emulator> document);

public:
    // Install management
    void softReset() override;

    // Info
    QString name() const override;
    QList<Import::ImageMode> preferredImageModeOrder() const override;
    bool isRunning() const override;
    QString versionString() const override;
    QString translateDocName(const QString& originalName, Lr::DataDoc::Type type) const override;

    // Import stage notifier hooks
    Qx::Error preImport(const ImportDetails& details) override;
    Qx::Error prePlatformsImport() override;
    Qx::Error postPlatformsImport() override;
    Qx::Error preImageProcessing(QList<ImageMap>& workerTransfers, const Lr::ImageSources& bulkSources) override;
    Qx::Error postImport() override;

    // Image handling
    void processDirectGameImages(const Lr::Game* game, const Lr::ImageSources& imageSources) override;
};
REGISTER_LAUNCHER(Install::NAME, Install, &Install::ICON_PATH, &Install::HELP_URL);

}
#endif // ATTRACTMODE_INSTALL_H
