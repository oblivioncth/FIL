#ifndef FLASHPOINTINSTALL_H
#define FLASHPOINTINSTALL_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QtSql>
#include "qx.h"

namespace FP
{

class FlashpointInstall
{
//-Inner Classes-------------------------------------------------------------------------------------------------
public:
    class DBTable_Game
    {
    public:
        static inline const QString NAME = "game";

        static inline const QString COL_ID = "id";
        static inline const QString COL_TITLE = "title";
        static inline const QString COL_SERIES = "series";
        static inline const QString COL_DEVELOPER = "developer";
        static inline const QString COL_PUBLISHER = "publisher";
        static inline const QString COL_DATE_ADDED = "dateAdded";
        static inline const QString COL_DATE_MODIFIED = "dateModified";
        static inline const QString COL_PLATFORM = "platform";
        static inline const QString COL_BROKEN = "broken";
        static inline const QString COL_PLAY_MODE = "playMode";
        static inline const QString COL_STATUS = "status";
        static inline const QString COL_NOTES = "notes";
        static inline const QString COL_SOURCE = "source";
        static inline const QString COL_APP_PATH = "applicationPath";
        static inline const QString COL_LAUNCH_COMMAND = "launchCommand";
        static inline const QString COL_RELEASE_DATE = "releaseDate";
        static inline const QString COL_VERSION = "version";
        static inline const QString COL_ORIGINAL_DESC = "originalDescription";
        static inline const QString COL_LANGUAGE = "language";
        static inline const QString COL_LIBRARY = "library";
        static inline const QString COL_ORDER_TITLE = "orderTitle";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TITLE, COL_SERIES, COL_DEVELOPER, COL_PUBLISHER, COL_DATE_ADDED, COL_DATE_MODIFIED, COL_PLATFORM,
                                               COL_BROKEN, COL_PLAY_MODE, COL_STATUS, COL_NOTES, COL_SOURCE, COL_APP_PATH, COL_LAUNCH_COMMAND, COL_RELEASE_DATE,
                                               COL_VERSION, COL_ORIGINAL_DESC, COL_LANGUAGE, COL_ORDER_TITLE, COL_LIBRARY};

        static inline const QString GAME_LIBRARY = "arcade";
    };

    class DBTable_Additonal_App
    {
    public:
        static inline const QString NAME = "additional_app";

        static inline const QString COL_ID = "id";
        static inline const QString COL_APP_PATH = "applicationPath";
        static inline const QString COL_AUTORUN = "autoRunBefore";
        static inline const QString COL_LAUNCH_COMMNAND = "launchCommand";
        static inline const QString COL_NAME = "name";
        static inline const QString COL_WAIT_EXIT = "waitForExit";
        static inline const QString COL_PARENT_ID = "parentGameId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_APP_PATH, COL_AUTORUN, COL_LAUNCH_COMMNAND, COL_NAME, COL_WAIT_EXIT, COL_PARENT_ID};

        static inline const QString ENTRY_EXTRAS = ":extras:";
        static inline const QString ENTRY_MESSAGE = ":message:";
    };

    class DBTable_Playlist
    {
    public:
        static inline const QString NAME = "playlist";
        static inline const QString COL_ID = "id";
        static inline const QString COL_TITLE = "title";
        static inline const QString COL_DESCRIPTION = "description";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TITLE, COL_DESCRIPTION};
    };

    class DBTable_Playlist_Game
    {
    public:
        static inline const QString NAME = "playlist_game";

        static inline const QString COL_ID = "id";
        static inline const QString COL_PLAYLIST_ID = "playlistId";
        static inline const QString COL_ORDER = "order";
        static inline const QString COL_GAME_ID = "gameId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_PLAYLIST_ID, COL_ORDER, COL_GAME_ID};
    };

    class OFLIb
    {
    // Class members
    public:
        static inline const QString EXE_NAME = "OFLIb.exe";
        static inline const QString APP_ARG = R"(--app="%1"")";
        static inline const QString PARAM_ARG = R"(--param="%1"")";
        static inline const QString MSG_ARG = R"(--msg="%1"")";

    // Class functions
    public:
        static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    };

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Paths
    static inline const QString LOGOS_PATH = "Data/Images/Logos";
    static inline const QString SCREENSHOTS_PATH = "Data/Images/Screenshots";
    static inline const QString DATABASE_PATH = "Data/flashpoint.sqlite";
    static inline const QString MAIN_EXE_PATH = "Launcher/Flashpoint.exe";

    // Version check
    static inline const QByteArray TARGET_EXE_SHA256 = Qx::ByteArray::RAWFromStringHex("868ab614df81ece9ab7b74e888f6888f474105cf60ea193856cf56e112086157");

    // Database
    static inline const QString DATABASE_CONNECTION_NAME = "Flashpoint Database";
    static inline const QSet<QPair<QString, QStringList>> DATABASE_TABLE_COLUMN_SET = {qMakePair(DBTable_Game::NAME, DBTable_Game::COLUMN_LIST),
                                                                                         qMakePair(DBTable_Additonal_App::NAME, DBTable_Additonal_App::COLUMN_LIST),
                                                                                         qMakePair(DBTable_Playlist::NAME, DBTable_Playlist::COLUMN_LIST),
                                                                                         qMakePair(DBTable_Playlist_Game::NAME, DBTable_Playlist_Game::COLUMN_LIST)};

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mLogosDirectory;
    QDir mScreenshotsDirectory;
    std::unique_ptr<QFile> mDatabaseFile;
    std::unique_ptr<QFile> mMainEXEFile;
    std::unique_ptr<QFile> mOFLIbEXEFile;

    // Database information
    QStringList mPlatformList;
    QStringList mPlaylistList;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    FlashpointInstall(QString installPath);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~FlashpointInstall();

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static bool pathIsValidFlashpointInstall(QString installPath);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool matchesTargetVersion() const;
    QSqlDatabase openDatabaseConnection();
    void closeDatabaseConnection();
    QSqlError checkDatabaseForRequiredTables(QSet<QString>& missingTablesBuffer) const;
    QSqlError checkDatabaseForRequiredColumns(QSet<QString>& missingColumsBuffer) const;
    QSqlError populatePlatforms();
    QSqlError populatePlaylists();

    QSqlError initialGameQuery(QList<std::tuple<QString, QSqlQuery, int>>& platform_query_sizeListBuffer, QStringList selectedPlatforms) const;
    QSqlError initialAdditionalAppQuery(std::pair<QSqlQuery, int>& query_sizeBuffer) const;
    QSqlError initialPlaylistQuery(std::pair<QSqlQuery, int>& query_sizeBuffer, QStringList selectedPlaylists) const;
    QSqlError initialPlaylistGameQuery(QList<std::tuple<QString, QSqlQuery, int>>& playlist_query_sizeListBuffer, QList<std::pair<QString, QString>> playlistNamesAndIDs) const;

    QStringList getPlatformList() const;
    QStringList getPlaylistList() const;
    QString getOFLIbPath() const;
};

}



#endif // FLASHPOINTINSTALL_H
