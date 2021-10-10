#ifndef COREIMPORTWORKER_H
#define COREIMPORTWORKER_H

#include <QObject>
#include <QMessageBox>
#include "flashpoint/fp-install.h"
#include "launchbox/lb-install.h"

class ImportWorker : public QObject
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImportResult {Failed, Canceled, Successful};

//-Class Structs-------------------------------------------------------------------------------------------------
public:
    struct ImportSelections
    {
        QStringList platforms;
        QStringList playlists;
    };

    struct OptionSet
    {
        LB::UpdateOptions updateOptions;
        LB::Install::ImageMode imageMode;
        LB::Install::PlaylistGameMode playlistMode;
        FP::DB::InclusionOptions inclusionOptions;
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

    // Import Errors
    static inline const QString MSG_FP_DB_CANT_CONNECT = "Failed to establish a handle to the Flashpoint database:";
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occured while reading the Flashpoint database:";
    static inline const QString MSG_LB_XML_UNEXPECTED_ERROR = "An unexpected error occured while reading Launchbox XML (%1 | %2):";

    // Error Captions
    static inline const QString CAPTION_IMAGE_ERR = "Error importing game image(s)";

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Install links
    std::shared_ptr<FP::Install> mFlashpointInstall;
    std::shared_ptr<LB::Install> mLaunchBoxInstall;

    // Job details
    ImportSelections mImportSelections;
    OptionSet mOptionSet;

    // Job Caches
    QSet<FP::AddApp> mAddAppsCache;
    QHash<QUuid, FP::Playlist> mPlaylistsCache;
    QHash<QUuid, LB::PlaylistGame::EntryDetails> mPlaylistGameDetailsCache;

    // Progress Tracking
    int mCurrentProgressValue;
    int mMaximumProgressValue;

    // Cancel Status
    bool mCanceled = false;

    // Error Tracking
    std::shared_ptr<int> mBlockingErrorResponse = std::make_shared<int>();

//-Constructor---------------------------------------------------------------------------------------------------
public:
    ImportWorker(std::shared_ptr<FP::Install> fpInstallForWork,
                 std::shared_ptr<LB::Install> lbInstallForWork,
                 ImportSelections importSelections,
                 OptionSet optionSet);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    const QList<QUuid> preloadPlaylists(FP::DB::QueryBuffer& playlistQuery);
    //const QMultiHash<QUuid, int> generateGameTagMap(FP::DB::QueryBuffer& gameTagsQuery);
    const QList<QUuid> getPlaylistSpecificGameIDs(FP::DB::QueryBuffer& playlistGameIDQuery);
    ImportResult preloadAddApps(Qx::GenericError& errorReport, FP::DB::QueryBuffer& addAppQuery);
    ImportResult processGames(Qx::GenericError& errorReport, QList<FP::DB::QueryBuffer>& gameQueries, bool playlistSpecific);
    ImportResult setImageReferences(Qx::GenericError& errorReport, QStringList platforms);
    ImportResult processPlaylists(Qx::GenericError& errorReport, QList<FP::DB::QueryBuffer>& playlistGameQueries);

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

    // Finished
    void importCompleted(ImportWorker::ImportResult importResult, Qx::GenericError errorReport);
};

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(ImportWorker::ImportResult);

#endif // COREIMPORTWORKER_H
