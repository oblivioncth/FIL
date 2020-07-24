#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "version.h"
#include "launchboxinstall.h"
#include "flashpointinstall.h"
#include "qx-io.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT // Required for classes that use Qt elements

//-Class Enums------------------------------------------------------------------------------------------------
    enum InputStage {PATHS, IMPORTS};
    enum UpdateMode {NEW_ONLY, REVISE_EXISTING};

//-Class Variables--------------------------------------------------------------------------------------------
private:
    // Messages - Input
    static inline const QString MSG_LB_INSTALL_INVALID = "The specified directory either doesn't contain a valid LaunchBox install, or it contains a version that is incompatible with this tool.";
    static inline const QString MSG_FP_INSTALL_INVALID = "The specified directory either doesn't contain a valid Flashpoint install, or it contains a version that is incompatible with this tool.";
    static inline const QString MSG_FP_VER_NOT_TARGET = "The selected Flashpoint install contains a version of Flashpoint that is different from the target version (" VER_PRODUCTVERSION_STR "), but appears to have a compatible structure. "
                                                                "You may proceed at your own risk as the tool is not guarnteed to work correctly in this circumstance. Please use a newer version of " VER_INTERNALNAME_STR " if available.";

    // Messages - FP Database read
    static inline const QString MSG_FP_DB_CANT_CONNECT = "Failed to establish a handle to the Flashpoint database! Make sure it is not being used by another program (i.e. Flashpoint may be running).";
    static inline const QString MSG_FP_DB_UNEXPECTED_ERROR = "An unexpected SQL error occured while reading the Flashpoint database:";
    static inline const QString MSG_FP_DB_MISSING_TABLE = "The Flashpoint database is missing tables critical to the import process.";
    static inline const QString MSG_FP_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing columns critical to the import process.";

    // Messages - LB XML read
    static inline const QString MSG_LB_XML_UNEXPECTED_ERROR = "An unexpected error occured while reading Launchbox XMLs:";

    // ProgressDialog - Import Operation
    static inline const QString PD_LABEL_FP_DB_INITIAL_QUERY = "Making initial Flashpoint database queries...";
    static inline const QString PD_BUTTON_CANCEL = "Cancel";

    // Dialog captions
    static inline const QString CAPTION_LAUNCHBOX_BROWSE = "Select the root directory of your LaunchBox install...";
    static inline const QString CAPTION_FLASHPOINT_BROWSE = "Select the root directory of your Flashpoint install...";

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    Ui::MainWindow *ui;
    QColor mExistingItemColor;

    std::unique_ptr<LB::LaunchBoxInstall> mLaunchBoxInstall;
    std::unique_ptr<FP::FlashpointInstall> mFlashpointInstall;

    int mLineEdit_launchBoxPath_blocker = 0; // Required due to an oversight with QLineEdit::editingFinished()
    int mLineEdit_flashpointPath_blocker = 0; // Required due to an oversight with QLineEdit::editingFinished()

    bool mAlteringListWidget = false;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    MainWindow(QWidget *parent = nullptr);

//-Destructor----------------------------------------------------------------------------------------------------
public:
    ~MainWindow();

//-Instance Functions--------------------------------------------------------------------------------------------
private:
    void initializeForms();
    void setInputStage(InputStage stage);
    void checkLaunchBoxInput(QString installPath);
    void checkFlashpointInput(QString installPath);
    void gatherInstallInfo();
    bool parseLaunchBoxData();
    bool parseFlashpointData();
    void postSqlError(QSqlError sqlError);
    void postListError(QString mainText, QStringList detailedItems);
    void postIOError(Qx::IO::IOOpReport report);
    void importSelectionReaction(QListWidgetItem* item, QWidget* parent);
    QStringList getSelectedPlatforms() const;
    QStringList getSelectedPlaylists() const;
    UpdateMode getSelectedUpdateMode() const;
    void importProcess();

//-Slots---------------------------------------------------------------------------------------------------------
private slots: // Start with "all" to avoid Qt calling "connectSlotsByName" on these slots (slots that start with "on_")
    void all_on_lineEdit_editingFinished();
    void all_on_lineEdit_textEdited();
    void all_on_lineEdit_returnPressed();
    void all_on_pushButton_clicked();
    void all_on_listWidget_itemChanged(QListWidgetItem* item);
};
#endif // MAINWINDOW_H
