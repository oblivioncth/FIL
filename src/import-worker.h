#ifndef COREIMPORTWORKER_H
#define COREIMPORTWORKER_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/network/qx-syncdownloadmanager.h>

// Project Includes
#include "flashpoint/fp-install.h"
#include "frontend/fe-install.h"

class ImportWorker : public QObject
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImportResult {Failed, Canceled, Successful};
    enum PlaylistGameMode {SelectedPlatform, ForceAll};

//-Class Structs-------------------------------------------------------------------------------------------------
public:
    struct ImportSelections
    {
        QStringList platforms;
        QStringList playlists;
    };

    struct OptionSet
    {
        Fe::UpdateOptions updateOptions;
        Fe::Install::ImageMode imageMode;
        bool downloadImages;
        PlaylistGameMode playlistMode;
        Fp::Db::InclusionOptions inclusionOptions;
    };

    struct ImageTransferJob
    {
        QString sourcePath;
        QString destPath;
    };

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Import Steps
    static inline const QString STEP_ADD_APP_PRELOAD = "Pre-loading Additional Apps...";
    static inline const QString STEP_IMPORTING_PLATFORM_GAMES = "Importing games for platform %1...";
    static inline const QString STEP_IMPORTING_PLAYLIST_SPEC_GAMES = "Importing playlist specific games for platform %1...";
    static inline const QString STEP_IMPORTING_PLATFORM_ADD_APPS = "Importing additional apps for platform %1...";
    static inline const QString STEP_IMPORTING_PLAYLIST_SPEC_ADD_APPS = "Importing playlist specific additional apps for platform %1...";
    static inline const QString STEP_IMPORTING_PLAYLIST_GAMES = "Importing playlist %1...";
    static inline const QString STEP_SETTING_IMAGE_REFERENCES = "Setting image references...";
    static inline const QString STEP_DOWNLOADING_IMAGES = "Downloading images...";
    static inline const QString STEP_IMPORTING_IMAGES = "Importing images...";

    // Import Errors
    static inline const QString MSG_FP_DB_CANT_CONNECT = "Failed to establish a handle to the Flashpoint database:";
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occured while reading the Flashpoint database:";

    // Files
    static inline const QString IMAGE_EXT = ".png";

    // Images Errors
    static inline const QString ERR_IMAGE_WONT_BACKUP = R"(Cannot rename the an existing image for backup:)";
    static inline const QString ERR_IMAGE_WONT_COPY = R"(Cannot copy the image "%1" to its destination:)";
    static inline const QString ERR_IMAGE_WONT_LINK = R"(Cannot create a symbolic link for "%1" at:)";
    static inline const QString ERR_CANT_MAKE_DIR = R"(Could not create the following image directory. Make sure you have write permissions at that location.)";
    static inline const QString CAPTION_IMAGE_ERR = "Error importing game image(s)";


//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Install links
    std::shared_ptr<Fp::Install> mFlashpointInstall;
    std::shared_ptr<Fe::Install> mFrontendInstall;

    // Image processing
    Qx::SyncDownloadManager mImageDownloadManager;

    // Job details
    ImportSelections mImportSelections;
    OptionSet mOptionSet;

    // Job Caches
    QSet<Fp::AddApp> mAddAppsCache;
    QHash<QUuid, Fp::Playlist> mPlaylistsCache;
    QSet<QUuid> mImportedGameIdsCache;
    QList<ImageTransferJob> mImageTransferJobs;

    // Progress Tracking
    int mCurrentProgressValue;
    int mMaximumProgressValue;

    // Cancel Status
    bool mCanceled = false;

    // Error Tracking
    std::shared_ptr<int> mBlockingErrorResponse = std::make_shared<int>();

//-Constructor---------------------------------------------------------------------------------------------------
public:
    ImportWorker(std::shared_ptr<Fp::Install> fpInstallForWork,
                 std::shared_ptr<Fe::Install> feInstallForWork,
                 ImportSelections importSelections,
                 OptionSet optionSet);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    const QList<QUuid> preloadPlaylists(Fp::Db::QueryBuffer& playlistQuery);
    const QList<QUuid> getPlaylistSpecificGameIds(Fp::Db::QueryBuffer& playlistGameIdQuery);
    Qx::GenericError transferImage(bool symlink, QString sourcePath, QString destPath);

    ImportResult preloadAddApps(Qx::GenericError& errorReport, Fp::Db::QueryBuffer& addAppQuery);
    ImportResult processGames(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& gameQueries, bool playlistSpecific);
    ImportResult processPlaylists(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& playlistGameQueries);
    ImportResult processImages(Qx::GenericError& errorReport, const QList<Fp::Db::QueryBuffer>& playlistSpecGameQueries);


public:
    ImportResult doImport(Qx::GenericError& errorReport);

//-Slots----------------------------------------------------------------------------------------------------------
public slots:
    void notifyCanceled();

//-Signals---------------------------------------------------------------------------------------------------------
signals:
    // Progress
    void progressValueChanged(int currentValue);
    void progressMaximumChanged(int maximumValue);
    void progressStepChanged(QString currentStep);

    // Error handling
    void blockingErrorOccured(std::shared_ptr<int> response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices);
    void authenticationRequired(QString prompt, QString* username, QString* password, bool* abort);

    // Finished
    void importCompleted(ImportWorker::ImportResult importResult, Qx::GenericError errorReport);
};

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(ImportWorker::ImportResult);

#endif // COREIMPORTWORKER_H
