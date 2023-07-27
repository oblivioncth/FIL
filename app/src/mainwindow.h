#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt Includes
#include <QMainWindow>
#include <QListWidgetItem>
#include <QProgressDialog>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-versionnumber.h>
#include <qx/io/qx-common-io.h>
#include <qx/widgets/qx-standarditemmodel.h>
#include <qx/windows-gui/qx-taskbarbutton.h>

// libfp Includes
#include <fp/fp-install.h>

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
    static inline const QString REQUIRE_ELEV = u" [Run as Admin/Dev Mode]"_s;

    // Messages - General
    static inline const QString MSG_FATAL_NO_INTERNAL_CLIFP_VER = u"Failed to get version information from the internal copy of CLIFp.exe!\n"
                                                                  "\n"
                                                                  "Execution cannot continue."_s;
    // Messages - Help
    static inline const QString MSG_PLAYLIST_GAME_MODE_HELP = u"<b>%1</b> - Games found in the selected playlists that are not part of any selected platform will be excluded.<br>"
                                                              "<br>"
                                                              "<b>%2</b> - Unselected platforms that contain games from the selected playlists will be partially imported, so that they only contain said games. This guarantees that all games "
                                                              "from each selected playlist will be present in your collection. <i>NOTE:</i> It's possible that this will include existing platforms so make sure you have your update options set as "
                                                              "intended."_s;

    static inline const QString MSG_UPDATE_MODE_HELP = u"<b>%1</b> - Only games not already present in your collection will be added, existing entries will be left completely untouched.<br>"
                                                       "<br>"
                                                       "<b>%2</b> - Games not already present in your collection will be added and existing entries will have their descriptive metadata (i.e. Title, Author, Images etc.) replaced by the "
                                                       "the details present in the target Flashpoint version; however, personal metadata (i.e. Playcount, Achievements, etc.) will not be altered.<br>"
                                                       "<br>"
                                                       "<b>%3</b> - Games in your collection that are not present in the target version of Flashpoint will be removed (only for selected items). You will no longer be able to play such "
                                                       "games if this option is unchecked, but this may be useful for archival purposes or in case you later want to revert to a previous version of Flashpoint and maintain the entries personal "
                                                       "metadata. Note that this option will also remove any games that are not covered by your current import selections (i.e. platforms, playlists, tag filter, etc.), even if they still remain "
                                                       "in Flashpoint"_s;
    static inline const QString MSG_IMAGE_MODE_HELP = u"<b>%1</b> - All relevant images from Flashpoint will be fully copied into your frontend installation. This causes zero overhead but will require additional storage space proportional to "
                                                      "the number of games you end up importing, up to double if all platforms are selected.<br>"
                                                      "<b>Space Consumption:</b> High<br>"
                                                      "<b>Import Speed:</b> Very Slow<br>"
                                                      "<b>Frontend Access Speed:</b> Fast<br>"
                                                      "<br>"
                                                      "<b>%2</b> - Your frontend platform configuration will be altered so that the relevant image folders within Flashpoint are directly referenced by its media scanner, requiring no "
                                                      "extra space and causing no overhead.<br>"
                                                      "<b>Space Consumption:</b> None<br>"
                                                      "<b>Import Speed:</b> Fast<br>"
                                                      "<b>>Frontend Access Speed:</b>Varies, but usually slow<br>"
                                                      "<br>"
                                                      "<b>%3</b> - A symbolic link to each relevant image from Flashpoint will be created in your frontend installation. These appear like the real files to the frontend, adding only a minuscule "
                                                      "amount of overhead when it loads images and require almost no extra disk space to store.<br>"
                                                      "<b>Space Consumption:</b> Near-zero<br>"
                                                      "<b>Import Speed:</b> Slow<br>"
                                                      "<b>>Frontend Access Speed:</b> Fast<br>"_s;

    // Messages - Input
    static inline const QString MSG_FE_INSTALL_INVALID = u"The specified directory either doesn't contain a valid frontend install, or it contains a version that is incompatible with this tool."_s;
    static inline const QString MSG_FP_INSTALL_INVALID = u"The specified directory either doesn't contain a valid Flashpoint install, or it contains a version that is incompatible with this tool."_s;
    static inline const QString MSG_FP_VER_NOT_TARGET = u"The selected Flashpoint install contains a version of Flashpoint that is different from the target version series (" PROJECT_TARGET_FP_VER_PFX_STR "), but appears to have a compatible structure. "
                                                                "You may proceed at your own risk as the tool is not guaranteed to work correctly in this circumstance. Please use a newer version of " PROJECT_SHORT_NAME " if available."_s;

    static inline const QString MSG_INSTALL_CONTENTS_CHANGED = u"The contents of your installs have been changed since the initial scan and therefore must be re-evaluated. You will need to make your selections again."_s;

    // Messages - General import procedure
    static inline const QString MSG_PRE_EXISTING_IMPORT = u"One or more existing Platforms/Playlists may be affected by this import. These will be altered even if they did not originate from this program (i.e. if you "
                                                          "already happened to have a Platform/Playlist with the same name as one present in Flashpoint).\n"
                                                          "\n"
                                                          "Are you sure you want to proceed?"_s;
    static inline const QString MSG_FRONTEND_CLOSE_PROMPT = u"The importer has detected that the selected frontend is running. It must be closed in order to continue. If recently closed, wait a few moments before trying to proceed again as it performs significant cleanup in the background."_s;
    static inline const QString MSG_POST_IMPORT = u"The Flashpoint import has completed successfully. Next time you start the frontend it may take longer than usual as it may have to fill in some default fields for the imported Platforms/Playlists.\n"
                                                  "\n"
                                                  "If you wish to import further selections or update to a newer version of Flashpoint, simply re-run this procedure after pointing it to the desired Flashpoint installation."_s;
    // Initial import status
    static inline const QString STEP_FP_DB_INITIAL_QUERY = u"Making initial Flashpoint database queries..."_s;

    // Messages - FP General
    static inline const QString MSG_FP_CLOSE_PROMPT = u"It is strongly recommended to close Flashpoint before proceeding as it can severely slow or interfere with the import process"_s;

    // Messages - FP Database read
    static inline const QString MSG_FP_DB_MISSING_TABLE = u"The Flashpoint database is missing tables critical to the import process."_s;
    static inline const QString MSG_FP_DB_TABLE_MISSING_COLUMN = u"The Flashpoint database tables are missing columns critical to the import process."_s;
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = u"An unexpected SQL error occurred while reading the Flashpoint database:"_s;

    // Messages - FP CLIFp
    static inline const QString MSG_FP_CLFIP_WILL_DOWNGRADE = u"The existing version of "_s + CLIFp::EXE_NAME +  u" in your Flashpoint install is newer than the version package with this tool.\n"
                                                              "\n"
                                                              "Replacing it with the packaged Version (downgrade) will likely cause compatibility issues unless you are specifically re-importing are downgrading your Flashpoint install to a previous version.\n"
                                                              "\n"
                                                              "Do you wish to downgrade "_s + CLIFp::EXE_NAME + u"?"_s;

    static inline const QString MSG_FP_CANT_DEPLOY_CLIFP = u"Failed to deploy "_s + CLIFp::EXE_NAME + u" to the selected Flashpoint install.\n"
                                                           "\n"
                                                           "%1\n"
                                                           "\n"
                                                           "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually."_s;

    // Messages - Import Result
    static inline const QString MSG_HAVE_TO_REVERT = u"Due to previous unrecoverable errors, all changes that occurred during import will now be reverted (other than existing images that were replaced with newer versions).\n"
                                                     "\n"
                                                     "Afterwards, check to see if there is a newer version of " PROJECT_SHORT_NAME " and try again using that version. If not ask for help on the relevant forums where this tool was released (see Help).\n"
                                                     "\n"
                                                     "If you believe this to be due to a bug with this software, please submit an issue to its GitHub page (listed under help)"_s;

    static inline const QString MSG_USER_CANCELED = u"Import canceled by user, all changes that occurred during import will now be reverted (other than existing images that were replaced with newer versions)."_s;
    static inline const QString MSG_NO_WORK = u"The provided import selections/options resulted in no tasks to perform. Double-check your settings."_s;

    // Dialog captions
    static inline const QString CAPTION_GENERAL_FATAL_ERROR = u"Fatal Error!"_s;
    static inline const QString CAPTION_FRONTEND_BROWSE = u"Select the root directory of your frontend install..."_s;
    static inline const QString CAPTION_FLASHPOINT_BROWSE = u"Select the root directory of your Flashpoint install..."_s;
    static inline const QString CAPTION_PLAYLIST_GAME_MODE_HELP = u"Playlist game mode options"_s;
    static inline const QString CAPTION_UPDATE_MODE_HELP = u"Update mode options"_s;
    static inline const QString CAPTION_IMAGE_MODE_HELP = u"Image mode options"_s;
    static inline const QString CAPTION_TASKLESS_IMPORT = u"Nothing to do"_s;
    static inline const QString CAPTION_REVERT = u"Reverting changes..."_s;
    static inline const QString CAPTION_CLIFP_ERR = u"Error deploying CLIFp"_s;
    static inline const QString CAPTION_CLIFP_DOWNGRADE = u"Downgrade CLIFp?"_s;
    static inline const QString CAPTION_IMPORTING = u"FP Import"_s;
    static inline const QString CAPTION_TAG_FILTER = u"Tag Filter"_s;

    // Menus
    static inline const QString MENU_FE_HELP_OBJ_NAME_TEMPLATE = u"action_%1Help"_s;
    static inline const QRegularExpression MENU_FE_HELP_KEY_REGEX = QRegularExpression(u"action_(?<frontend>.*?)Help"_s);

    // URLs
    static inline const QUrl URL_CLIFP_GITHUB = QUrl(u"https://github.com/oblivioncth/CLIFp"_s);
    static inline const QUrl URL_FIL_GITHUB =  QUrl(u"https://github.com/oblivioncth/FIL"_s);

    // Flashpoint version check
    static inline const Qx::VersionNumber TARGET_FP_VERSION_PREFIX = Qx::VersionNumber::fromString(PROJECT_TARGET_FP_VER_PFX_STR);

    // Selection defaults
    static inline const QList<Fe::ImageMode> DEFAULT_IMAGE_MODE_ORDER = {
        Fe::ImageMode::Link,
        Fe::ImageMode::Reference,
        Fe::ImageMode::Copy
    };

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
    void validateInstall(const QString& installPath, InstallType install);
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
    bool selectionsMayModify();

    void postSqlError(const QString& mainText, const QSqlError& sqlError);
    void postListError(const QString& mainText, const QStringList& detailedItems);
    void postIOError(const QString& mainText, const Qx::IoOpReport& report);

    void refreshEnableStates();
    void refreshCheckStates();

    QStringList getSelectedPlatforms() const;
    QStringList getSelectedPlaylists() const;
    Fp::Db::InclusionOptions getSelectedInclusionOptions() const;
    Fe::UpdateOptions getSelectedUpdateOptions() const;
    Fe::ImageMode getSelectedImageMode() const;
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
    void handleBlockingError(std::shared_ptr<int> response, const Qx::Error& blockingError, QMessageBox::StandardButtons choices);
    void handleAuthRequest(const QString& prompt, QAuthenticator* authenticator);
    void handleImportResult(ImportWorker::ImportResult importResult, const Qx::Error& errorReport);
};

#endif // MAINWINDOW_H
