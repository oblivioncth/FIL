#ifndef COREIMPORTWORKER_H
#define COREIMPORTWORKER_H

#include <QObject>
#include <QMessageBox>
#include "flashpointinstall.h"
#include "launchboxinstall.h"

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
        QSet<QString> platforms;
        QSet<QString> playlists;
    };

    struct OptionSet
    {
        LB::Install::UpdateOptions updateOptions;
        LB::Install::ImageMode imageMode;
        LB::Install::GeneralOptions generalOptions;
    };

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Import Steps
    // ProgressDialog - Import Operation
    static inline const QString STEP_FP_DB_INITIAL_QUERY = "Making initial Flashpoint database queries...";
    static inline const QString STEP_ADD_APP_PRELOAD = "Pre-loading Additional Apps...";
    static inline const QString STEP_IMPORTING_PLATFORM_GAMES = "Importing games for platform %1...";
    static inline const QString STEP_IMPORTING_PLATFORM_ADD_APPS = "Importing additional apps for platform %1...";
    static inline const QString STEP_IMPORTING_PLAYLIST_GAMES = "Importing playlist %1...";

    // Import Errors
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occured while reading the Flashpoint database:";

    // Error Captions
    static inline const QString CAPTION_IMAGE_ERR = "Error importing game image(s)";

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    std::shared_ptr<FP::Install> mFlashpointInstall;
    std::shared_ptr<LB::Install> mLaunchBoxInstall;
    ImportSelections mImportSelections;
    OptionSet mOptionSet;
    bool mCanceled = false;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    ImportWorker(std::shared_ptr<FP::Install> fpInstallForWork,
                 std::shared_ptr<LB::Install> lbInstallForWork,
                 ImportSelections importSelections,
                 OptionSet optionSet);

//-Slots---------------------------------------------------------------------------------------------------------
public slots:
    void doImport();
    void notifyCancel();

//-Signals---------------------------------------------------------------------------------------------------------
signals:
    // Progress
    void progressValueChanged(int currentValue);
    void progressMaximumChanged(int maximumValue);
    void progressStepChanged(QString currentStep);

    // Error handling
    void blockingErrorOccured(int* response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices);

    // Finished
    void importCompleted(ImportResult importResult, Qx::GenericError errorReport);
};

#endif // COREIMPORTWORKER_H
