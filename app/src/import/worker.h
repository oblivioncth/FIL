#ifndef IMPORT_WORKER_H
#define IMPORT_WORKER_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-groupedprogressmanager.h>

// libfp Includes
#include <fp/fp-install.h>
/* TODO: Figure out why removing this include from here causes the compilation of
 * the flashpoint install property in properties.h to fail due to the type being
 * incomplete
 */

// Project Includes
#include "launcher/interface/lr-install-interface.h"
#include "import/image.h"

namespace Import
{

class Worker : public QObject
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum Result {Failed, Canceled, Taskless, Successful};

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

//-Class Variables-----------------------------------------------------------------------------------------------
private:
    // Import Steps
    static inline const QString STEP_ADD_APP_PRELOAD = u"Pre-loading Additional Apps..."_s;
    static inline const QString STEP_IMPORTING_PLATFORM_SETS = u"Importing games and additional apps for platform %1..."_s;
    static inline const QString STEP_IMPORTING_PLAYLIST_SPEC_SETS = u"Importing playlist specific and additional apps for platform %1..."_s;
    static inline const QString STEP_IMPORTING_PLAYLISTS = u"Importing playlist %1..."_s;
    static inline const QString STEP_FINALIZING = u"Finalizing..."_s;

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Install links
    Fp::Install* mFlashpointInstall;
    Lr::IInstall* mLauncherInstall;

    // Image Manager
    ImageManager mImageManager;

    // Job details
    Selections mImportSelections;
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
    Worker(Fp::Install* flashpoint, Lr::IInstall* launcher, Selections importSelections, OptionSet optionSet);

//-Destructor---------------------------------------------------------------------------------------------------
public:
    ~Worker();

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    Qx::ProgressGroup* initializeProgressGroup(const QString& groupName, quint64 weight);
    Qx::Error preloadPlaylists(QList<Fp::Playlist>& targetPlaylists);
    QList<QUuid> getPlaylistSpecificGameIds(const QList<Fp::Playlist>& playlists);
    Result processPlatformGames(Qx::Error& errorReport, std::unique_ptr<Lr::IPlatformDoc>& platformDoc, Fp::Db::QueryBuffer& gameQueryResult);
    void cullUnimportedPlaylistGames(QList<Fp::Playlist>& playlists);

    Result preloadAddApps(Qx::Error& errorReport, Fp::Db::QueryBuffer& addAppQuery);
    Result processGames(Qx::Error& errorReport, QList<Fp::Db::QueryBuffer>& primary, QList<Fp::Db::QueryBuffer>& playlistSpecific);
    Result processPlaylists(Qx::Error& errorReport, const QList<Fp::Playlist>& playlists);
    Result processImages(Qx::Error& errorReport);
    Result processIcons(Qx::Error& errorReport, const QStringList& platforms, const QList<Fp::Playlist>& playlists);

public:
    Result doImport(Qx::Error& errorReport);

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
    void importCompleted(Worker::Result importResult, const Qx::Error& errorReport);
};

}

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(Import::Worker::Result);

#endif // IMPORT_WORKER_H
