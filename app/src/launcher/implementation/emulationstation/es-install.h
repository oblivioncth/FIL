#ifndef EMULATIONSTATION_INSTALL_H
#define EMULATIONSTATION_INSTALL_H

// Qt Includes
#include <QRegularExpression>

// Project Includes
#include "launcher/abstract/lr-install.h"
#include "launcher/implementation/emulationstation/es-data.h"

namespace Es
{

class Install : public Lr::Install<LauncherId>
{
    friend class Gamelist;
    friend class Collection;
//-Class Variables--------------------------------------------------------------------------------------------------
private:
    // Paths
#ifdef _WIN32
    static inline const QString EXE_NAME = u"ES-DE.exe"_s;
#else
    static inline const QString EXE_NAME = u"es-de"_s;
#endif
    static inline const QString DATA_ROOT_PATH = u"ES-DE"_s;
    static inline const QString PORTABLE_TXT_PATH = u"portable.txt"_s;
    static inline const QString GAMELISTS_PATH = u"gamelists"_s;
    static inline const QString COLLECTIONS_PATH = u"collections"_s;
    static inline const QString CUSTOM_SYSTEMS_PATH = u"custom_systems"_s;
    static inline const QString CUSTOM_SYSTEMS_FILE_PATH = u"es_systems.xml"_s;
    static inline const QString DOWNLOADED_MEDIA_PATH = u"downloaded_media"_s;
    static inline const QString THEMES_PATH = u"themes"_s;
    static inline const QString SETTINGS_PATH = u"settings"_s;
    static inline const QString SETTINGS_FILE_PATH = u"es_settings.xml"_s;
    static inline const QString LOG_PATH = u"logs/es_log.txt"_s;
    static inline const QString LOGO_PATH = u"covers"_s;
    static inline const QString SCREENSHOT_PATH = u"screenshots"_s;
    static inline const QString FAKE_THEME_NAME = u"_flashpoint"_s;
    static inline const QString FAKE_THEME_ICON_NAME = u"flashpoint.png"_s;
    static inline const QString PLATFORM_ICONS_FOLDER_NAME = u"systems"_s;
    static inline const QString PLAYLIST_ICONS_FOLDER_NAME = u"collections"_s;

    // Files
    static inline const QString CFG_EXT = u"cfg"_s;
    static inline const QString CUSTOM_PREFIX = u"custom-"_s;
    static inline const QString GAMELIST_FILENAME = u"gamelist.xml"_s;

    // Support
    static inline const QList<Import::ImageMode> IMAGE_MODE_ORDER {
        Import::ImageMode::Link,
        Import::ImageMode::Copy
    };
    static inline const QRegularExpression LOG_VERSION_REGEX = QRegularExpression(uR"(.* Info:\s+ES-DE (?<ver>[0-9]\.[0-9]\.[0-9] ))"_s);

public:
    static inline const QString ROMPATH_PLACEHOLDER = u"%ROMPATH%"_s;
    static inline const QString BASENAME_PLACEHOLDER = u"%BASENAME%"_s;
    static inline const QString DUMMY_EXT = u"dummy"_s;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mDataRoot;
    QDir mGameListsDir;
    QDir mCollectionsDir;
    QDir mCustomSystemsDir;
    QDir mDownloadedMediaDir;
    QDir mThemesDir;
    QDir mSettingsDir;
    QDir mRomsDir; // Determined before import
    QFile mPortableFile;
    QFile mExeFile;
    QFile mLogFile;
    QFile mCustomSystemsFile;
    QFile mSettingsFile;

    QDir mFakeThemeDir;
    QDir mPlatformIconsDir;
    QDir mPlaylistIconsDir;

    // Persistent config handles
    std::unique_ptr<Systemlist> mCustomSystemsDoc;
    std::unique_ptr<Settings> mSettingsDoc;

    // Other trackers
    QHash<QUuid, QString> mPlaylistGameSystemNameCache;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(const QString& installPath);

//-Class Functions--------------------------------------------------------------------------------------------------------
private:
    QDir determineDataRoot(const QDir& providedDir);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Install management
    Qx::Error populateExistingDocs(QSet<Lr::IDataDoc::Identifier>& existingDocs) override;
    QString versionFromExecutable() const;

    // Doc handling
    QString dataDocPath(Lr::IDataDoc::Identifier identifier) const;
    std::unique_ptr<Gamelist> preparePlatformDocCheckout(const QString& translatedName) override;
    void preparePlatformDocCommit(const Gamelist& document) override;
    std::unique_ptr<Collection> preparePlaylistDocCheckout(const QString& translatedName) override;
    void preparePlaylistDocCommit(const Collection& collection) override;

    Lr::DocHandlingError checkoutCustomSystemsDoc();
    Lr::DocHandlingError commitCustomSystemsDoc();
    Lr::DocHandlingError checkoutSettingsDoc();
    Lr::DocHandlingError commitSettingsDoc();
    //Lr::DocHandlingError commitSettingsDoc(); No writes needed

public:
    // Install management
    void softReset() override;

    // Info
    QList<Import::ImageMode> preferredImageModeOrder() const override;
    bool isRunning() const override;
    QString versionString() const override;
    QString translateDocName(const QString& originalName, Lr::IDataDoc::Type type) const override;
    QDir romsDirectory() const;

    // Import stage notifier hooks
    Qx::Error preImport() override;
    Qx::Error prePlatformsImport() override;
    Qx::Error postPlatformsImport() override;
    Qx::Error postImport() override;

    // Image handling
    QString generateImagePath(const Game& game, Fp::ImageType type) override;
    void processBulkImageSources(const Import::ImagePaths& bulkSources) override;
    QString platformCategoryIconPath() const override;
    std::optional<QDir> platformIconsDirectory() const override;
    std::optional<QDir> playlistIconsDirectory() const override;
};

/* TODO: This is really annoying but it seems that ES REQUIRES rom files to be present in order for the game
 * to show up so we have to make dummy files; this is not true when the 'gamelist.xml only' option is enabled,
 * but that affects every system that's part of the install (we only need it to be enabled for FP systems) and
 * users may not like that. It would be more ideal to patch ES-DE to add an option to es_systems.xml to allow
 * for a particular system to be treated like that option is on, even if it isn't. Just a simple bool. I posed
 * this to the maintainer but they said they generally don't accept PRs :/. At some point, just try anyway. It
 * seems like if it can be made very minimal they might accept it.
 */

}

#endif // EMULATIONSTATION_INSTALL_H
