#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QProgressDialog>
#include <QMessageBox>
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#include "version.h"
#include "launchbox-install.h"
#include "launchbox-xml.h"
#include "flashpoint-install.h"
#include "import-worker.h"
#include "qx-io.h"
#include "qx-widgets.h"
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
    enum class Install {LaunchBox, Flashpoint};

//-Class Variables--------------------------------------------------------------------------------------------
private:
    // Constants
    //static const int IMPORT_UI_UPD_INTERVAL = 17; // Workaround update tick speed in ms

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
                                                       "the details present in the target Flashpoint version; however, personal metadata (i.e. Playcount, Acheivements, etc.) will not be altered.<br>"
                                                       "<br>"
                                                       "<b>%3</b> - Games in your collection that are not present in the target version of Flashpoint will be removed (only for selected items). You will no longer be able to play such "
                                                       "games if this option is unchecked, but this may be useful for archival purposes or incase you later want to revert to a previous version of Flashpoint and maintain the entries personal "
                                                       "metadata. Note that this option will still cause missing games to be removed even if you are going backwards to a previous version of FP, as implied above. Additionally, this option will "
                                                       "remove any existing Extreme games in your collection, for the select platforms, if you have the <i>%4</i> option unselected.";

    static inline const QString MSG_IMAGE_MODE_HELP = "<b>%1</b> - All relevant images from Flashpoint will be fully copied into your LaunchBox installation. This causes zero overhead but will require additional storage space proportional to "
                                                      "the number of games you end up importing, up to double if all platforms are selected.<br>"
                                                      "<b>Space Consumption:</b> High<br>"
                                                      "<b>Import Speed:</b> Very Slow<br>"
                                                      "<b>Image Cache Build Speed:</b> Fast<br>"
                                                      "<br>"
                                                      "<b>%2</b> - Your LaunchBox platform configuration will be altered so that the relavent image folders within Flashpoint are directly referenced by LaunchBox's media scanner, requiring no "
                                                      "extra space and causing no overhead.<br>"
                                                      "<b>Space Consumption:</b> None<br>"
                                                      "<b>Import Speed:</b> Fast<br>"
                                                      "<b>Image Cache Build Speed:</b> Very Slow<br>"
                                                      "<br>"
                                                      "<b>%3</b> - A symbolic link to each relavent image from Flashpoint will be created in your LaunchBox installation. These appear like the real files to LaunchBox, adding only a miniscle "
                                                      "amount of overhead when it loads images and require almost no extra disk space to store.<br>"
                                                      "<b>Space Consumption:</b> Near-zero<br>"
                                                      "<b>Import Speed:</b> Slow<br>"
                                                      "<b>Image Cache Build Speed:</b> Fast<br>";

    // Messages - Input
    static inline const QString MSG_LB_INSTALL_INVALID = "The specified directory either doesn't contain a valid LaunchBox install, or it contains a version that is incompatible with this tool.";
    static inline const QString MSG_FP_INSTALL_INVALID = "The specified directory either doesn't contain a valid Flashpoint install, or it contains a version that is incompatible with this tool.";
    static inline const QString MSG_FP_VER_NOT_TARGET = "The selected Flashpoint install contains a version of Flashpoint that is different from the target version (" VER_PRODUCTVERSION_STR "), but appears to have a compatible structure. "
                                                                "You may proceed at your own risk as the tool is not guarnteed to work correctly in this circumstance. Please use a newer version of " VER_INTERNALNAME_STR " if available.";

    static inline const QString MSG_INSTALL_CONTENTS_CHANGED = "The contents of your installs have been changed since the initial scan and therefore must be re-evaluated. You will need to make your selections again.";

    // Messages - General import procedure
    static inline const QString MSG_PRE_EXISTING_IMPORT = "One or more existing Platforms/Playlists may be affected by this import. These will be altered even if they did not orignate from this program (i.e. if you "
                                                          "already happened to have a Platform/Playlist with the same name as one present in Flashpoint).\n"
                                                          "\n"
                                                          "Are you sure you want to proceed?";
    static inline const QString MSG_LB_CLOSE_PROMPT = "The importer has detected that LaunchBox is running. It must be closed in order to continue. If recently closed, wait a few moments before trying to proceed again as it performs significant cleanup in the background.";
    static inline const QString MSG_POST_IMPORT = "The Flashpoint import has completed succesfully. Next time you start LaunchBox it may take longer than usual as it will have to fill in some default fields for the imported Platforms/Playlists.\n"
                                                  "\n"
                                                  "If you wish to import further selections or update to a newer version of Flashpoint, simply re-run this procedure after pointing it to the desired Flashpoint installation.";
    // Initial import status
    static inline const QString STEP_FP_DB_INITIAL_QUERY = "Making initial Flashpoint database queries...";

    // Messages - FP General
    static inline const QString MSG_FP_CLOSE_PROMPT = "It is strongly recommended to close Flashpoint before proceeding as it can severely slow or interfer with the import process";

    // Messages - FP Database read
    static inline const QString MSG_FP_DB_CANT_CONNECT = "Failed to establish a handle to the Flashpoint database:\n"
                                                         "\n"
                                                         "%1";
    static inline const QString MSG_FP_DB_MISSING_TABLE = "The Flashpoint database is missing tables critical to the import process.";
    static inline const QString MSG_FP_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing columns critical to the import process.";
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occured while reading the Flashpoint database:";

    // Messages - FP CLIFp
    static inline const QString MSG_FP_CLFIP_WILL_DOWNGRADE = "The existing version of " + CLIFp::EXE_NAME +  " in your Flashpoint install is newer than the version package with this tool.\n"
                                                              "\n"
                                                              "Replacing it with the packaged Version (downgrade) will likely cause compatability issues unless you are specifically re-importing are downgrading your Flashpoint install to a previous version.\n"
                                                              "\n"
                                                              "Do you wish to downgrade " + CLIFp::EXE_NAME + "?";

    static inline const QString MSG_FP_CANT_DEPLOY_CLIFP = "Failed to deploy " + CLIFp::EXE_NAME + " to the selected Flashpoint install.\n"
                                                           "\n"
                                                           "%1\n"
                                                           "\n"
                                                           "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually.";

    // Messages - LB XML read
    static inline const QString MSG_LB_XML_UNEXPECTED_ERROR = "An unexpected error occured while reading Launchbox XML (%1 | %2):";

    // Messages - Revert
    static inline const QString MSG_HAVE_TO_REVERT = "Due to previous unrecoverable errors, all changes that occured during import will now be reverted (other than existing images that were replaced with newer versions).\n"
                                                     "\n"
                                                     "Aftewards, check to see if there is a newer version of " VER_INTERNALNAME_STR " and try again using that version. If not ask for help on the LaunchBox forums where this tool was released (see Tools).\n"
                                                     "\n"
                                                     "If you beleive this to be due to a bug with this software, please submit an issue to its GitHub page (listed under help)";

    static inline const QString MSG_USER_CANCELED = "Import canceled by user, all changes that occured during import will now be reverted (other than existing images that were replaced with newer versions).";

    // Dialog captions
    static inline const QString CAPTION_GENERAL_FATAL_ERROR = "Fatal Error!";
    static inline const QString CAPTION_LAUNCHBOX_BROWSE = "Select the root directory of your LaunchBox install...";
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

    // URLs
    static inline const QUrl URL_CLIFP_GITHUB = QUrl("https://github.com/oblivioncth/CLIFp");
    static inline const QUrl URL_OFLIB_GITHUB =  QUrl("https://github.com/oblivioncth/OFILb");
    static inline const QUrl URL_LB_FORUMS =  QUrl("https://forums.launchbox-app.com/files/file/2652-obbys-flashpoint-importer-for-launchbox");

    // Flashpoint version check
    static inline const QString TARGET_EXE_SHA256 = "ec90eee88448071eef65365604c16d437522f23fe1a5b21dd3dc972bc47cf9af";
    static inline const QString TARGET_ULT_VER_STRING = R"(Flashpoint 10.1 Ultimate - "Absence II")";
    static inline const QString TARGET_INF_VER_STRING = R"(Flashpoint 10.1.0.1 Infinity - "Absence II")";


    // User Roles
    static inline const int USER_ROLE_TAG_ID = 0x200; // Value chosen arbitrarily (must be > 0x100)

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    Ui::MainWindow *ui;
    bool mInitCompleted;

    QHash<QWidget*, std::function<bool(void)>> mWidgetEnableConditionMap;
    QHash<QAction*, std::function<bool(void)>> mActionEnableConditionMap;
    QColor mExistingItemColor;

    std::shared_ptr<LB::Install> mLaunchBoxInstall;
    std::shared_ptr<FP::Install> mFlashpointInstall;
    Qx::MMRB mInternalCLIFpVersion;

    int mLineEdit_launchBoxPath_blocker = 0; // Required due to an oversight with QLineEdit::editingFinished()
    int mLineEdit_flashpointPath_blocker = 0; // Required due to an oversight with QLineEdit::editingFinished()

    QHash<QListWidgetItem*,Qt::CheckState> mPlatformItemCheckStates;
    QHash<QListWidgetItem*,Qt::CheckState> mPlaylistItemCheckStates;
    std::unique_ptr<Qx::StandardItemModelX> mTagSelectionModel;

    bool mHasLinkPermissions = false;

    QString mArgedPlaylistGameModeHelp;
    QString mArgedUpdateModeHelp;
    QString mArgedImageModeHelp;

    // Process monitoring
    std::unique_ptr<QProgressDialog> mImportProgressDialog;
    QWinTaskbarButton* mWindowTaskbarButton; // TODO: Remove for Qt6

    //QTimer mUIUpdateWorkaroundTimer;

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
    bool installMatchesTargetVersion(const FP::Install& fpInstall);
    void checkManualInstallInput(Install install);
    void validateInstall(QString installPath, Install install);
    void gatherInstallInfo();
    void populateImportSelectionBoxes();
    void generateTagSelectionOptions();
    bool parseLaunchBoxData();
    bool parseFlashpointData();
    bool installsHaveChanged();
    void redoInputChecks();

    void clearListWidgets();
    bool isExistingPlatformSelected();
    bool isExistingPlaylistSelected();

    void postSqlError(QString mainText, QSqlError sqlError);
    void postListError(QString mainText, QStringList detailedItems);
    void postIOError(QString mainText, Qx::IOOpReport report);
    int postGenericError(Qx::GenericError error, QMessageBox::StandardButtons choices);

    void refreshEnableStates();

    QStringList getSelectedPlatforms() const;
    QStringList getSelectedPlaylists() const;
    FP::Install::InclusionOptions getSelectedInclusionOptions() const;
    LB::UpdateOptions getSelectedUpdateOptions() const;
    LB::Install::ImageMode getSelectedImageMode() const;
    LB::Install::PlaylistGameMode getSelectedPlaylistGameMode() const;

    void prepareImport();
    void revertAllLaunchBoxChanges();
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
    void all_on_lineEdit_textEdited();
    void all_on_lineEdit_returnPressed();
    void all_on_pushButton_clicked();
    void all_on_listWidget_itemChanged(QListWidgetItem* item);
    void all_on_radioButton_clicked();

    // Workaround update
    //void resetUpdateTimer();
    //void updateUI();

    // Import Error Handling
    void handleBlockingError(std::shared_ptr<int> response, Qx::GenericError blockingError, QMessageBox::StandardButtons choices);
    void handleImportResult(ImportWorker::ImportResult importResult, Qx::GenericError errorReport);
};

//-Metatype Declarations-----------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(std::shared_ptr<int>);

#endif // MAINWINDOW_H
