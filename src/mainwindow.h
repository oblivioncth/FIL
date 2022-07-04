#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt Includes
#include <QMainWindow>
#include <QListWidgetItem>
#include <QProgressDialog>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/core/qx-versionnumber.h>
#include <qx/io/qx-common-io.h>
#include <qx/widgets/qx-standarditemmodel.h>
#include <qx/windows-gui/qx-taskbarbutton.h>

// libfp Includes
#include <fp/flashpoint/fp-install.h>

// Project Includes
#include "project_vars.h"
#include "frontend/fe-install.h"
#include "import-worker.h"
#include "clifp.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums------------------------------------------------------------------------------------------------
private:
    enum class InputStage {Paths, Imports};
    enum class InstallType {Frontend, Flashpoint};

//-Class Variables--------------------------------------------------------------------------------------------
private:
    // UI Text
    static inline const QString REQUIRE_ELEV = " [Run as Admin/Dev Mode]";

    // Messages - General
    static inline const QString MSG_FATAL_NO_INTERNAL_CLIFP_VER = "Failed to get version information from the internal copy of CLIFp.exe!\n"
                                                                  "\n"
                                                                  "Execution cannot continue.";
    // Messages - Help
    static inline const QString MSG_PLAYLIST_GAME_MODE_HELP = "<b>%1</b> - Games found in the selected playlists that are not part of any selected platform will be excluded.<br>"
                                                              "<br>"
                                                              "<b>%2</b> - Unselected platforms that contain games from the selected playlists will be partially imported, so that they only contain said games. This guarantees that all games "
                                                              "from each selected playlist will be present in your collection. <i>NOTE:</i> It's possible that this will include existing platforms so make sure you have your update options set as "
                                                              "intended.";

    static inline const QString MSG_UPDATE_MODE_HELP = "<b>%1</b> - Only games not already present in your collection will be added, existing entries will be left completely untouched.<br>"
                                                       "<br>"
                                                       "<b>%2</b> - Games not already present in your collection will be added and existing entries will have their descriptive metadata (i.e. Title, Author, Images etc.) replaced by the "
                                                       "the details present in the target Flashpoint version; however, personal metadata (i.e. Playcount, Achievements, etc.) will not be altered.<br>"
                                                       "<br>"
                                                       "<b>%3</b> - Games in your collection that are not present in the target version of Flashpoint will be removed (only for selected items). You will no longer be able to play such "
                                                       "games if this option is unchecked, but this may be useful for archival purposes or in case you later want to revert to a previous version of Flashpoint and maintain the entries personal "
                                                       "metadata. Note that this option will still cause missing games to be removed even if you are going backwards to a previous version of FP, as implied above. Additionally, this option will "
                                                       "remove any existing Extreme games in your collection, for the select platforms, if you have the <i>%4</i> option unselected.";

    static inline const QString MSG_IMAGE_MODE_HELP = "<b>%1</b> - All relevant images from Flashpoint will be fully copied into your frontend installation. This causes zero overhead but will require additional storage space proportional to "
                                                      "the number of games you end up importing, up to double if all platforms are selected.<br>"
                                                      "<b>Space Consumption:</b> High<br>"
                                                      "<b>Import Speed:</b> Very Slow<br>"
                                                      "<b>Image Cache Build Speed:</b> Fast<br>"
                                                      "<br>"
                                                      "<b>%2</b> - Your frontend platform configuration will be altered so that the relevant image folders within Flashpoint are directly referenced by its media scanner, requiring no "
                                                      "extra space and causing no overhead.<br>"
                                                      "<b>Space Consumption:</b> None<br>"
                                                      "<b>Import Speed:</b> Fast<br>"
                                                      "<b>Image Cache Build Speed:</b> Very Slow<br>"
                                                      "<br>"
                                                      "<b>%3</b> - A symbolic link to each relevant image from Flashpoint will be created in your frontend installation. These appear like the real files to the frontend, adding only a minuscule "
                                                      "amount of overhead when it loads images and require almost no extra disk space to store.<br>"
                                                      "<b>Space Consumption:</b> Near-zero<br>"
                                                      "<b>Import Speed:</b> Slow<br>"
                                                      "<b>Image Cache Build Speed:</b> Fast<br>";

    // Messages - Input
    static inline const QString MSG_FE_INSTALL_INVALID = "The specified directory either doesn't contain a valid frontend install, or it contains a version that is incompatible with this tool.";
    static inline const QString MSG_FP_INSTALL_INVALID = "The specified directory either doesn't contain a valid Flashpoint install, or it contains a version that is incompatible with this tool.";
    static inline const QString MSG_FP_VER_NOT_TARGET = "The selected Flashpoint install contains a version of Flashpoint that is different from the target version series (" PROJECT_TARGET_FP_VER_PFX_STR "), but appears to have a compatible structure. "
                                                                "You may proceed at your own risk as the tool is not guaranteed to work correctly in this circumstance. Please use a newer version of " PROJECT_SHORT_NAME " if available.";

    static inline const QString MSG_INSTALL_CONTENTS_CHANGED = "The contents of your installs have been changed since the initial scan and therefore must be re-evaluated. You will need to make your selections again.";

    // Messages - General import procedure
    static inline const QString MSG_PRE_EXISTING_IMPORT = "One or more existing Platforms/Playlists may be affected by this import. These will be altered even if they did not originate from this program (i.e. if you "
                                                          "already happened to have a Platform/Playlist with the same name as one present in Flashpoint).\n"
                                                          "\n"
                                                          "Are you sure you want to proceed?";
    static inline const QString MSG_FRONTEND_CLOSE_PROMPT = "The importer has detected that the selected frontend is running. It must be closed in order to continue. If recently closed, wait a few moments before trying to proceed again as it performs significant cleanup in the background.";
    static inline const QString MSG_POST_IMPORT = "The Flashpoint import has completed successfully. Next time you start the frontend it may take longer than usual as it will have to fill in some default fields for the imported Platforms/Playlists.\n"
                                                  "\n"
                                                  "If you wish to import further selections or update to a newer version of Flashpoint, simply re-run this procedure after pointing it to the desired Flashpoint installation.";
    // Initial import status
    static inline const QString STEP_FP_DB_INITIAL_QUERY = "Making initial Flashpoint database queries...";

    // Messages - FP General
    static inline const QString MSG_FP_CLOSE_PROMPT = "It is strongly recommended to close Flashpoint before proceeding as it can severely slow or interfere with the import process";

    // Messages - FP Database read
    static inline const QString MSG_FP_DB_MISSING_TABLE = "The Flashpoint database is missing tables critical to the import process.";
    static inline const QString MSG_FP_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing columns critical to the import process.";
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occurred while reading the Flashpoint database:";

    // Messages - FP CLIFp
    static inline const QString MSG_FP_CLFIP_WILL_DOWNGRADE = "The existing version of " + CLIFp::EXE_NAME +  " in your Flashpoint install is newer than the version package with this tool.\n"
                                                              "\n"
                                                              "Replacing it with the packaged Version (downgrade) will likely cause compatibility issues unless you are specifically re-importing are downgrading your Flashpoint install to a previous version.\n"
                                                              "\n"
                                                              "Do you wish to downgrade " + CLIFp::EXE_NAME + "?";

    static inline const QString MSG_FP_CANT_DEPLOY_CLIFP = "Failed to deploy " + CLIFp::EXE_NAME + " to the selected Flashpoint install.\n"
                                                           "\n"
                                                           "%1\n"
                                                           "\n"
                                                           "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually.";

    // Messages - Revert
    static inline const QString MSG_HAVE_TO_REVERT = "Due to previous unrecoverable errors, all changes that occurred during import will now be reverted (other than existing images that were replaced with newer versions).\n"
                                                     "\n"
                                                     "Afterwards, check to see if there is a newer version of " PROJECT_SHORT_NAME " and try again using that version. If not ask for help on the relevant forums where this tool was released (see Help).\n"
                                                     "\n"
                                                     "If you believe this to be due to a bug with this software, please submit an issue to its GitHub page (listed under help)";

    static inline const QString MSG_USER_CANCELED = "Import canceled by user, all changes that occurred during import will now be reverted (other than existing images that were replaced with newer versions).";

    // Dialog captions
    static inline const QString CAPTION_GENERAL_FATAL_ERROR = "Fatal Error!";
    static inline const QString CAPTION_FRONTEND_BROWSE = "Select the root directory of your frontend install...";
    static inline const QString CAPTION_FLASHPOINT_BROWSE = "Select the root directory of your Flashpoint install...";
    static inline const QString CAPTION_PLAYLIST_GAME_MODE_HELP = "Playlist game mode options";
    static inline const QString CAPTION_UPDATE_MODE_HELP = "Update mode options";
    static inline const QString CAPTION_IMAGE_MODE_HELP = "Image mode options";\
    static inline const QString CAPTION_REVERT = "Reverting changes...";
    static inline const QString CAPTION_REVERT_ERR = "Error reverting changes";
    static inline const QString CAPTION_CLIFP_ERR = "Error deploying CLIFp";
    static inline const QString CAPTION_CLIFP_DOWNGRADE = "Downgrade CLIFp?";
    static inline const QString CAPTION_IMPORTING = "FP Import";
    static inline const QString CAPTION_TAG_FILTER = "Tag Filter";

    // Menus
    static inline const QString MENU_FE_HELP_OBJ_NAME_TEMPLATE = "action_%1Help";
    static inline const QRegularExpression MENU_FE_HELP_KEY_REGEX = QRegularExpression("action_(?<frontend>.*?)Help");

    // URLs
    static inline const QUrl URL_CLIFP_GITHUB = QUrl("https://github.com/oblivioncth/CLIFp");
    static inline const QUrl URL_OFLIB_GITHUB =  QUrl("https://github.com/oblivioncth/FIL");

    // Flashpoint version check
    static inline const Qx::VersionNumber TARGET_FP_VERSION_PREFIX = Qx::VersionNumber::fromString(PROJECT_TARGET_FP_VER_PFX_STR);

    // User Roles
    static inline const int USER_ROLE_TAG_ID = 0x200; // Value chosen arbitrarily (must be > 0x100)

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    Ui::MainWindow *ui;
    bool mInitCompleted;

    QHash<QWidget*, std::function<bool(void)>> mWidgetEnableConditionMap;
    QHash<QAction*, std::function<bool(void)>> mActionEnableConditionMap;
    QColor mExistingItemColor;

    std::shared_ptr<Fe::Install> mFrontendInstall;
    std::shared_ptr<Fp::Install> mFlashpointInstall;
    Qx::VersionNumber mInternalCLIFpVersion;

    QHash<QListWidgetItem*,Qt::CheckState> mPlatformItemCheckStates;
    QHash<QListWidgetItem*,Qt::CheckState> mPlaylistItemCheckStates;
    std::unique_ptr<Qx::StandardItemModel> mTagSelectionModel;

    bool mHasLinkPermissions = false;

    QString mArgedPlaylistGameModeHelp;
    QString mArgedUpdateModeHelp;
    QString mArgedImageModeHelp;

    // Process monitoring
    std::unique_ptr<QProgressDialog> mImportProgressDialog;
    Qx::TaskbarButton* mWindowTaskbarButton;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    MainWindow(QWidget *parent = nullptr);

//-Destructor----------------------------------------------------------------------------------------------------
public:
    ~MainWindow();

//-Instance Functions--------------------------------------------------------------------------------------------
private:
    bool testForLinkPermissions();
    void initializeForms();
    void initializeEnableConditionMaps();
    void initializeFrontendHelpActions();
    bool installMatchesTargetSeries(const Fp::Install& fpInstall);
    void checkManualInstallInput(InstallType install);
    void validateInstall(QString installPath, InstallType install);
    void gatherInstallInfo();
    void populateImportSelectionBoxes();
    void generateTagSelectionOptions();
    bool parseFrontendData();
    bool installsHaveChanged();
    void redoInputChecks();

    void invalidateInstall(InstallType install, bool informUser);
    void clearListWidgets();
    bool isExistingPlatformSelected();
    bool isExistingPlaylistSelected();

    void postSqlError(QString mainText, QSqlError sqlError);
    void postListError(QString mainText, QStringList detailedItems);
    void postIOError(QString mainText, Qx::IoOpReport report);

    void refreshEnableStates();
    void refreshCheckStates();

    QStringList getSelectedPlatforms() const;
    QStringList getSelectedPlaylists() const;
    Fp::Db::InclusionOptions getSelectedInclusionOptions() const;
    Fe::UpdateOptions getSelectedUpdateOptions() const;
    Fe::Install::ImageMode getSelectedImageMode() const;
    ImportWorker::PlaylistGameMode getSelectedPlaylistGameMode() const;
    bool getForceDownloadImages() const;

    void prepareImport();
    void revertAllFrontendChanges();
    void standaloneCLIFpDeploy();
    void showTagSelectionDialog();
    QSet<int> generateTagExlusionSet() const;

protected:
    void showEvent(QShowEvent* event);

public:
    bool initCompleted();

//-Slots---------------------------------------------------------------------------------------------------------
private slots:
    // Direct UI, start with "all" to avoid Qt calling "connectSlotsByName" on these slots (slots that start with "on_")
    void all_on_action_triggered();
    void all_on_lineEdit_editingFinished();
    void all_on_pushButton_clicked();
    void all_on_listWidget_itemChanged(QListWidgetItem* item);
    void all_on_radioButton_clicked();
    void all_on_menu_triggered(QAction *action);

    // Import Exception Handling
    void handleBlockingError(std::shared_ptr<int> response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices);
    void handleAuthRequest(QString prompt, QAuthenticator* authenticator);
    void handleImportResult(ImportWorker::ImportResult importResult, Qx::GenericError errorReport);
};

#endif // MAINWINDOW_H
