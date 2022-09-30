#ifndef COREIMPORTWORKER_H
#define COREIMPORTWORKER_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/network/qx-downloadmanager.h>
#include <qx/core/qx-groupedprogressmanager.h>

// libfp Includes
#include <fp/flashpoint/fp-install.h>

// Project Includes
#include "frontend/fe-install.h"

class ImportWorker : public QObject
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImportResult {Failed, Canceled, Successful};
    enum PlaylistGameMode {SelectedPlatform, ForceAll};

//-Inner Classes------------------------------------------------------------------------------------------------
private:
    class Pg
    {
    public:
        static inline const QString AddAppPreload = "AddAppPreload";
        static inline const QString ImageDownload = "ImageDownload";
        static inline const QString ImageTransfer = "ImageTransfer";
        static inline const QString GameImport = "GameImport";
        static inline const QString PlaylistGameMatchImport = "PlaylistGameMatchImport";
    };

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
        Fe::ImageMode imageMode;
        bool downloadImages;
        PlaylistGameMode playlistMode;
        Fp::Db::InclusionOptions inclusionOptions;
    };

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Import Steps
    static inline const QString STEP_ADD_APP_PRELOAD = "Pre-loading Additional Apps...";
    static inline const QString STEP_IMPORTING_PLATFORM_SETS = "Importing games and additional apps for platform %1...";
    static inline const QString STEP_IMPORTING_PLAYLIST_SPEC_SETS = "Importing playlist specific and additional apps for platform %1...";
    static inline const QString STEP_IMPORTING_PLAYLIST_GAMES = "Importing playlist %1...";
    static inline const QString STEP_DOWNLOADING_IMAGES = "Downloading images...";
    static inline const QString STEP_IMPORTING_IMAGES = "Importing images...";
    static inline const QString STEP_FINALIZING = "Finalizing...";

    // Import Errors
    static inline const QString MSG_FP_DB_CANT_CONNECT = "Failed to establish a handle to the Flashpoint database:";
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occurred while reading the Flashpoint database:";

    // Files
    static inline const QString IMAGE_EXT = ".png";

    // Images Errors
    static inline const QString ERR_IMAGE_SRC_UNAVAILABLE = "An Expected source image does not exist.\n%1";
    static inline const QString ERR_IMAGE_WONT_BACKUP = "Cannot rename an existing image for backup.\n%1";
    static inline const QString ERR_IMAGE_WONT_COPY = "Cannot copy the image\n\n%1\n\nto\n\n%2";
    static inline const QString ERR_IMAGE_WONT_LINK = "Cannot create a symbolic link from\n\n%1\n\n%2";
    static inline const QString ERR_CANT_MAKE_DIR = "Could not create the following image directory. Make sure you have write permissions at that location.\n%1";
    static inline const QString CAPTION_IMAGE_ERR = "Error importing game image(s)";
    static inline const QString IMAGE_RETRY_PROMPT = "Retry?";

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
    QMultiHash<QUuid, Fp::AddApp> mAddAppsCache; // Stores in groups based on parent ID
    QHash<QUuid, Fp::Playlist> mPlaylistsCache;
    QSet<QUuid> mImportedGameIdsCache;

    // Progress Tracking
    Qx::GroupedProgressManager mProgressManager;
    quint64 mCurrentProgress;

    // Cancel Status
    bool mCanceled;

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
    Qx::ProgressGroup* initializeProgressGroup(const QString& groupName, quint64 weight);
    const QList<QUuid> preloadPlaylists(Fp::Db::QueryBuffer& playlistQuery);
    const QList<QUuid> getPlaylistSpecificGameIds(Fp::Db::QueryBuffer& playlistGameIdQuery);
    Qx::GenericError transferImage(bool symlink, QString sourcePath, QString destPath);
    ImportResult processPlatformGames(Qx::GenericError& errorReport, std::unique_ptr<Fe::PlatformDoc>& platformDoc, Fp::Db::QueryBuffer& gameQueryResult);
    ImportResult processPlatformAddApps(Qx::GenericError& errorReport, std::unique_ptr<Fe::PlatformDoc>& platformDoc);

    ImportResult preloadAddApps(Qx::GenericError& errorReport, Fp::Db::QueryBuffer& addAppQuery);
    ImportResult processGames(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& primary, QList<Fp::Db::QueryBuffer>& playlistSpecific);
    ImportResult processPlaylists(Qx::GenericError& errorReport, QList<Fp::Db::QueryBuffer>& playlistGameQueries);
    ImportResult processImages(Qx::GenericError& errorReport, const QList<Fp::Db::QueryBuffer>& playlistSpecGameQueries);

public:
    ImportResult doImport(Qx::GenericError& errorReport);

//-Slots----------------------------------------------------------------------------------------------------------
private slots:
    void pmProgressUpdated(quint64 currentProgress);

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
    void authenticationRequired(QString prompt, QAuthenticator* authenticator);

    // Finished
    void importCompleted(ImportWorker::ImportResult importResult, Qx::GenericError errorReport);
};

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(ImportWorker::ImportResult);

#endif // COREIMPORTWORKER_H
