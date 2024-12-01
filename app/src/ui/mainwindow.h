#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt Includes
#include <QMainWindow>
#include <QListWidgetItem>
#include <QMessageBox>

// Qx Includes
#include <qx/widgets/qx-standarditemmodel.h>
#include <qx/io/qx-common-io.h>
#include <qx/core/qx-bimap.h>
#include <qx/core/qx-property.h>

// Project Includes
#include "import/properties.h"
#include "project_vars.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QRadioButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT;
//-Class Structs------------------------------------------------------------------------------------------------
private:
    struct Bindings
    {
        /* Ideally these could all just be QBindable, but that only works for Q_PROPERTIES that have
         * a notify signal, and most of these don't, so we have to use our own property and subscribe
         * to it as a workaround instead.
         */
        Qx::Bindable<bool> forceAllModeChecked; // Defaults to false
        Qx::Property<bool> importSelectionEnabled;
        Qx::Property<bool> playlistGameModeEnabled;
        Qx::Property<bool> updateModeEnabled;
        Qx::Property<bool> imageModeEnabled;
        Qx::Property<bool> startImportEnabled;
        Qx::Property<bool> forceDownloadImagesEnabled;
        Qx::Property<bool> editTagFilterEnabled;
        Qx::Property<QString> launcherVersion;
        Qx::Property<QString> flashpointVersion;
        Qx::Property<QPixmap> launcherStatus;
        Qx::Property<QPixmap> flashpointStatus;
        Qx::Property<bool> launcherPathTouched; // Defaults to false
        Qx::Property<bool> flashpointPathTouched; // Defaults to false
    };

//-Inner Classes------------------------------------------------------------------------------------------------
private:
    class SelectionList
    {
        static constexpr int USER_ROLE_EXISTING = 0x333; // Value chosen arbitrarily (must be > 0x100)

        QListWidget* mWidget;
        Qx::Property<int> mSelCount;
        Qx::Property<int> mExistSelCount;

        void handleCheckChange(QListWidgetItem* item);

    public:
        SelectionList(QListWidget* widget);

        void fill(const QList<Import::Importee>& imps);
        void clear();

        int selectedCount() const;
        int existingSelectedCount() const;
    };

//-Class Variables--------------------------------------------------------------------------------------------
private:
    // UI Text
    static inline const QString REQUIRE_ELEV = u" [Run as Admin/Dev Mode]"_s;

    // Messages - Help
    static inline const QString MSG_ABOUT = PROJECT_FULL_NAME + u"\n\nVersion: "_s + PROJECT_VERSION_STR;
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
    static inline const QString MSG_IMAGE_MODE_HELP = u"<b>%1</b> - All relevant images from Flashpoint will be fully copied into your launcher installation. This causes zero overhead but will require additional storage space proportional to "
                                                      "the number of games you end up importing, up to double if all platforms are selected.<br>"
                                                      "<b>Space Consumption:</b> High<br>"
                                                      "<b>Import Speed:</b> Very Slow<br>"
                                                      "<b>Launcher Access Speed:</b> Fast<br>"
                                                      "<br>"
                                                      "<b>%2</b> - Your launcher platform configuration will be altered so that the relevant image folders within Flashpoint are directly referenced by its media scanner, requiring no "
                                                      "extra space and causing no overhead.<br>"
                                                      "<b>Space Consumption:</b> None<br>"
                                                      "<b>Import Speed:</b> Fast<br>"
                                                      "<b>Launcher Access Speed:</b>Varies, but usually slow<br>"
                                                      "<br>"
                                                      "<b>%3</b> - A symbolic link to each relevant image from Flashpoint will be created in your launcher installation. These appear like the real files to the launcher, adding only a minuscule "
                                                      "amount of overhead when it loads images and require almost no extra disk space to store.<br>"
                                                      "<b>Space Consumption:</b> Near-zero<br>"
                                                      "<b>Import Speed:</b> Slow<br>"
                                                      "<b>Launcher Access Speed:</b> Fast<br>"_s;

    // Dialog captions
    static inline const QString CAPTION_LAUNCHER_BROWSE = u"Select the root directory of your launcher install..."_s;
    static inline const QString CAPTION_FLASHPOINT_BROWSE = u"Select the root directory of your Flashpoint install..."_s;
    static inline const QString CAPTION_PLAYLIST_GAME_MODE_HELP = u"Playlist game mode options"_s;
    static inline const QString CAPTION_UPDATE_MODE_HELP = u"Update mode options"_s;
    static inline const QString CAPTION_IMAGE_MODE_HELP = u"Image mode options"_s;
    static inline const QString CAPTION_TAG_FILTER = u"Tag Filter"_s;

    // Menus
    static inline const QString MENU_LR_HELP_OBJ_NAME_TEMPLATE = u"action_%1Help"_s;
    static inline const QRegularExpression MENU_LR_HELP_KEY_REGEX = QRegularExpression(u"action_(?<launcher>.*?)Help"_s);

    // URLs
    static inline const QUrl URL_CLIFP_GITHUB = QUrl(u"https://github.com/oblivioncth/CLIFp"_s);
    static inline const QUrl URL_FIL_GITHUB =  QUrl(u"https://github.com/oblivioncth/FIL"_s);

    // User Roles
    static inline const int USER_ROLE_TAG_ID = 0x200; // Value chosen arbitrarily (must be > 0x100)

    // Color
    static inline QColor smExistingItemColor;

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    Ui::MainWindow* ui;
    SelectionList mPlatformSelections;
    SelectionList mPlaylistSelections;
    const Import::Properties& mImportProperties;

    Qx::Bimap<Import::ImageMode, QRadioButton*> mImageModeMap;
    QHash<QRadioButton*, Import::PlaylistGameMode> mPlaylistGameModeMap;
    QHash<QRadioButton*, Import::UpdateOptions> mUpdateModeMap;
    Qx::StandardItemModel mTagModel;

    QString mArgedPlaylistGameModeHelp;
    QString mArgedUpdateModeHelp;
    QString mArgedImageModeHelp;

    // Behavior
    Bindings mBindings;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    MainWindow(const Import::Properties& importProperties, QWidget *parent = nullptr);

//-Destructor----------------------------------------------------------------------------------------------------
public:
    ~MainWindow();

//-Instance Functions--------------------------------------------------------------------------------------------
private:
    // Init
    Qx::Bimap<Import::ImageMode, QRadioButton*> initializeImageModeMap() const;
    QHash<QRadioButton*, Import::PlaylistGameMode> initializePlaylistGameModeMap() const;
    QHash<QRadioButton*, Import::UpdateMode> initializeUpdateModeMap() const;
    void initializeBindings();
    void initializeForms();
    void initializeLauncherHelpActions();

    // Input querying
    bool selectionsMayModify();
    QStringList getSelectedPlatforms() const;
    QStringList getSelectedPlaylists() const;
    Import::PlaylistGameMode getSelectedPlaylistGameMode() const;
    Fp::Db::InclusionOptions getSelectedInclusionOptions() const;
    Import::UpdateOptions getSelectedUpdateOptions() const;
    Import::ImageMode getSelectedImageMode() const;
    bool getForceDownloadImages() const;

    // Import
    void prepareImport();

    // Tags (move to controller?)
    void showTagSelectionDialog();
    QSet<int> generateTagExlusionSet() const;

//-Signals & Slots----------------------------------------------------------------------------------------------------
private slots:
    // Direct UI, start with "all" to avoid Qt calling "connectSlotsByName" on these slots (slots that start with "on_")
    void all_on_action_triggered();
    void all_on_pushButton_clicked();
    void all_on_menu_triggered(QAction *action);
    void all_on_lineEdit_editingFinished();

signals:
    void installPathChanged(const QString& installPath, Import::Install type);
    void importTriggered(Import::Selections sel, Import::OptionSet opt, bool mayModify);
    void standaloneDeployTriggered();
};

#endif // MAINWINDOW_H
