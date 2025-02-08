#ifndef CONTROLLER_H
#define CONTROLLER_H

// Qt Includes
#include <QObject>

// Project Includes
#include "import/properties.h"
#include "import/worker.h"
#include "kernel/clifp.h"
#include "ui/mainwindow.h"
#include "ui/progresspresenter.h"

class Controller : public QObject
{
//-Class Variables---------------------------------------------------------------
private:
    // Messages - General
    static inline const QString MSG_FATAL_NO_INTERNAL_CLIFP_VER = u"Failed to get version information from the internal copy of CLIFp.exe!\n"
                                                                  "\n"
                                                                  "Execution cannot continue."_s;

    // Messages - FP General
    static inline const QString MSG_FP_CLOSE_PROMPT = u"It is strongly recommended to close Flashpoint before proceeding as it can severely slow or interfere with the import process"_s;

    // Messages - Input
    static inline const QString MSG_LR_INSTALL_INVALID = u"The specified directory either doesn't contain a valid launcher install, or it contains a version that is incompatible with this tool."_s;
    static inline const QString MSG_FP_INSTALL_INVALID = u"The specified directory either doesn't contain a valid Flashpoint install, or it contains a version that is incompatible with this tool."_s;
    static inline const QString MSG_FP_VER_NOT_TARGET = u"The selected Flashpoint install contains a version of Flashpoint that is different from the target version series (" PROJECT_TARGET_FP_VER_PFX_STR "), but appears to have a compatible structure. "
                                                        "You may proceed at your own risk as the tool is not guaranteed to work correctly in this circumstance. Please use a newer version of " PROJECT_SHORT_NAME " if available."_s;

    static inline const QString MSG_INSTALL_CONTENTS_CHANGED = u"The contents of your installs have been changed since the initial scan and therefore must be re-evaluated. You will need to make your selections again."_s;

    // Messages - General import procedure
    static inline const QString MSG_PRE_EXISTING_IMPORT = u"One or more existing Platforms/Playlists may be affected by this import. These will be altered even if they did not originate from this program (i.e. if you "
                                                          "already happened to have a Platform/Playlist with the same name as one present in Flashpoint).\n"
                                                          "\n"
                                                          "Are you sure you want to proceed?"_s;
    static inline const QString MSG_LAUNCHER_CLOSE_PROMPT = u"The importer has detected that the selected launcher is running. It must be closed in order to continue. If recently closed, wait a few moments before trying to proceed again as it performs significant cleanup in the background."_s;

    // Initial import status
    static inline const QString STEP_FP_DB_INITIAL_QUERY = u"Making initial Flashpoint database queries..."_s;

    // Messages - Import Result
    static inline const QString MSG_POST_IMPORT = u"The Flashpoint import has completed successfully. Next time you start the launcher it may take longer than usual as it may have to fill in some default fields for the imported Platforms/Playlists.\n"
                                                  "\n"
                                                  "If you wish to import further selections or update to a newer version of Flashpoint, simply re-run this procedure after pointing it to the desired Flashpoint installation."_s;
    static inline const QString MSG_NO_WORK = u"The provided import selections/options resulted in no tasks to perform. Double-check your settings."_s;
    static inline const QString MSG_USER_CANCELED = u"Import canceled by user, all changes that occurred during import will now be reverted (other than existing images that were replaced with newer versions)."_s;
    static inline const QString MSG_HAVE_TO_REVERT = u"Due to previous unrecoverable errors, all changes that occurred during import will now be reverted (other than existing images that were replaced with newer versions).\n"
                                                     "\n"
                                                     "Afterwards, check to see if there is a newer version of " PROJECT_SHORT_NAME " and try again using that version. If not ask for help on the relevant forums where this tool was released (see Help).\n"
                                                     "\n"
                                                     "If you believe this to be due to a bug with this software, please submit an issue to its GitHub page (listed under help)"_s;

    // Messages - FP CLIFp
    static inline const QString MSG_FP_CLFIP_WILL_DOWNGRADE = u"The existing version of "_s + CLIFp::EXE_NAME +  u" in your Flashpoint install is newer than the version package with this tool.\n"
                                                                                                                "\n"
                                                                                                                "Replacing it with the packaged Version (downgrade) will likely cause compatibility issues unless you are specifically re-importing after downgrading your Flashpoint install to a previous version.\n"
                                                                                                                "\n"
                                                                                                                "Do you wish to downgrade "_s + CLIFp::EXE_NAME + u"?"_s;

    static inline const QString MSG_FP_CANT_DEPLOY_CLIFP = u"Failed to deploy "_s + CLIFp::EXE_NAME + u" to the selected Flashpoint install.\n"
                                                                                                      "\n"
                                                                                                      "%1\n"
                                                                                                      "\n"
                                                                                                      "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually."_s;
    // Dialog captions
    static inline const QString CAPTION_GENERAL_FATAL_ERROR = u"Fatal Error!"_s;
    static inline const QString CAPTION_TASKLESS_IMPORT = u"Nothing to do"_s;
    static inline const QString CAPTION_IMPORTING = u"FP Import"_s;
    static inline const QString CAPTION_REVERT = u"Reverting changes..."_s;
    static inline const QString CAPTION_FLASHPOINT_BROWSE = u"Select the root directory of your Flashpoint install..."_s;
    static inline const QString CAPTION_CLIFP_DOWNGRADE = u"Downgrade CLIFp?"_s;
    static inline const QString CAPTION_CLIFP_ERR = u"Error deploying CLIFp"_s;

//-Instance Variables-------------------------------------------------------------
private:
    Import::Properties mImportProperties;
    MainWindow mMainWindow;
    ProgressPresenter mProgressPresenter;

//-Constructor-------------------------------------------------------------
public:
    Controller();

//-Instance Functions-------------------------------------------------------------
private:
    void processImportResult(Import::Worker::Result importResult, const Qx::Error& errorReport);
    void revertAllLauncherChanges();
    void deployCLIFp(const Fp::Install& fp, QMessageBox::Button abandonButton);

//-Signals & Slots-------------------------------------------------------------
private slots:
    // Import Handlers
    void handleBlockingError(std::shared_ptr<int> response, const Qx::Error& blockingError, QMessageBox::StandardButtons choices);
    void handleAuthRequest(const QString& prompt, QAuthenticator* authenticator);

public slots:
    void updateInstallPath(const QString& installPath, Import::Install type);
    void startImport(Import::Selections sel, Import::OptionSet opt, bool mayModify);
    void standaloneCLIFpDeploy();
};

#endif // CONTROLLER_H
