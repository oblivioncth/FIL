#ifndef COREIMPORTWORKER_H
#define COREIMPORTWORKER_H

#include <QObject>
#include <QMessageBox>
#include "flashpoint-install.h"
#include "launchbox-install.h"

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
        LB::UpdateOptions updateOptions;
        LB::Install::ImageMode imageMode;
        FP::Install::InclusionOptions inclusionOptions;
    };

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Import Steps
    static inline const QString STEP_ADD_APP_PRELOAD = "Pre-loading Additional Apps...";
    static inline const QString STEP_IMPORTING_PLATFORM_GAMES = "Importing games for platform %1...";
    static inline const QString STEP_IMPORTING_PLATFORM_ADD_APPS = "Importing additional apps for platform %1...";
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

//-Instance Functions---------------------------------------------------------------------------------------------------------
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
    void importCompleted(ImportResult importResult, Qx::GenericError errorReport);
};

//-Metatype declarations-------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(ImportWorker::ImportResult);

#endif // COREIMPORTWORKER_H
