#ifndef COREIMPORTWORKER_H
#define COREIMPORTWORKER_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/network/qx-downloadmanager.h>
#include <qx/core/qx-groupedprogressmanager.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "frontend/fe-install.h"

class QX_ERROR_TYPE(ImageTransferError, "ImageTransferError", 1350)
{
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        ImageSourceUnavailable = 1,
        ImageWontBackup = 2,
        ImageWontCopy = 3,
        ImageWontLink = 4,
        CantCreateDirectory = 5
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {ImageSourceUnavailable, u"An Expected source image does not exist."_s},
        {ImageWontBackup, u"Cannot rename an existing image for backup."_s},
        {ImageWontCopy, u"Cannot copy an image to its destination."_s},
        {ImageWontLink, u"Cannot create a symbolic link for an image."_s},
        {CantCreateDirectory, u"Could not create a directory for an image destination."_s}
    };

    static inline const QString CAPTION_IMAGE_ERR = u"Error importing game image(s)"_s;
    static inline const QString IMAGE_RETRY_PROMPT = u"Retry?"_s;
    static inline const QString SRC_PATH_TEMPLATE = u"Source: %1"_s;
    static inline const QString DEST_PATH_TEMPLATE = u"Destination: %1"_s;

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSourcePath;
    QString mDestinationPath;

//-Constructor-------------------------------------------------------------
public:
    ImageTransferError(Type t = NoError, const QString& src = {}, const QString& dest = {});

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
    QString deriveDetails() const override;
    QString deriveCaption() const override;
};

class ImportWorker : public QObject
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImportResult {Failed, Canceled, Taskless, Successful};
    enum PlaylistGameMode {SelectedPlatform, ForceAll};

//-Inner Classes------------------------------------------------------------------------------------------------
private:
    class Pg
    {
    public:
        static inline const QString AddAppPreload = u"AddAppPreload"_s;
        static inline const QString ImageDownload = u"ImageDownload"_s;
        static inline const QString ImageTransfer = u"ImageTransfer"_s;
        static inline const QString IconTransfer = u"IconTransfer"_s;
        static inline const QString GameImport = u"GameImport"_s;
        static inline const QString PlaylistImport = u"PlaylistImport"_s;
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
    static inline const QString STEP_ADD_APP_PRELOAD = u"Pre-loading Additional Apps..."_s;
    static inline const QString STEP_IMPORTING_PLATFORM_SETS = u"Importing games and additional apps for platform %1..."_s;
    static inline const QString STEP_IMPORTING_PLAYLIST_SPEC_SETS = u"Importing playlist specific and additional apps for platform %1..."_s;
    static inline const QString STEP_IMPORTING_PLAYLISTS = u"Importing playlist %1..."_s;
    static inline const QString STEP_DOWNLOADING_IMAGES = u"Downloading images..."_s;
    static inline const QString STEP_IMPORTING_IMAGES = u"Importing images..."_s;
    static inline const QString STEP_FINALIZING = u"Finalizing..."_s;

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
    Qx::Error preloadPlaylists(QList<Fp::Playlist>& targetPlaylists);
    QList<QUuid> getPlaylistSpecificGameIds(const QList<Fp::Playlist>& playlists);
    ImageTransferError transferImage(bool symlink, QString sourcePath, QString destPath);
    bool performImageJobs(const QList<Fe::Install::ImageMap>& jobs, bool symlink, Qx::ProgressGroup* pg = nullptr);
    ImportResult processPlatformGames(Qx::Error& errorReport, std::unique_ptr<Fe::PlatformDoc>& platformDoc, Fp::Db::QueryBuffer& gameQueryResult);
    void cullUnimportedPlaylistGames(QList<Fp::Playlist>& playlists);

    ImportResult preloadAddApps(Qx::Error& errorReport, Fp::Db::QueryBuffer& addAppQuery);
    ImportResult processGames(Qx::Error& errorReport, QList<Fp::Db::QueryBuffer>& primary, QList<Fp::Db::QueryBuffer>& playlistSpecific);
    ImportResult processPlaylists(Qx::Error& errorReport, const QList<Fp::Playlist>& playlists);
    ImportResult processImages(Qx::Error& errorReport);
    ImportResult processIcons(const QStringList& platforms);

public:
    ImportResult doImport(Qx::Error& errorReport);

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
    void progressStepChanged(const QString& currentStep);

    // Error handling
    void blockingErrorOccured(std::shared_ptr<int> response, const Qx::Error& blockingError, QMessageBox::StandardButtons choices);
    void authenticationRequired(const QString& prompt, QAuthenticator* authenticator);

    // Finished
    void importCompleted(ImportWorker::ImportResult importResult, const Qx::Error& errorReport);
};

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(ImportWorker::ImportResult);

#endif // COREIMPORTWORKER_H
