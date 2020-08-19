#ifndef FLASHPOINTINSTALL_H
#define FLASHPOINTINSTALL_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QtSql>
#include "qx.h"
#include "flashpoint.h"

namespace FP
{

class Install
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
        static inline const QString COL_EXTREME = "extreme";
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
                                               COL_BROKEN, COL_EXTREME, COL_PLAY_MODE, COL_STATUS, COL_NOTES, COL_SOURCE, COL_APP_PATH, COL_LAUNCH_COMMAND, COL_RELEASE_DATE,
                                               COL_VERSION, COL_ORIGINAL_DESC, COL_LANGUAGE, COL_LIBRARY, COL_ORDER_TITLE};

        static inline const QString GAME_LIBRARY = "arcade";
    };

    class DBTable_Add_App
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
        static inline const QString COL_AUTHOR = "author";
        static inline const QString COL_LIBRARY = "library";

        static inline const QString GAME_LIBRARY = "arcade";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TITLE, COL_DESCRIPTION, COL_AUTHOR, COL_LIBRARY};
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

    class CLIFp
    {
    // Class members
    public:
        static inline const QString EXE_NAME = "CLIFp.exe";
        static inline const QString APP_ARG = R"(--app="%1")";
        static inline const QString PARAM_ARG = R"(--param="%1")";
        static inline const QString MSG_ARG = R"(--msg="%1")";

    // Class functions
    public:
        static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    };

//-Class Structs-------------------------------------------------------------------------------------------------
    struct DBTableSpecs
    {
        QString name;
        QStringList columns;
    };

    struct DBQueryBuffer
    {
        QString source;
        QSqlQuery result;
        int size;
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
    static inline const QList<DBTableSpecs> DATABASE_SPECS_LIST = {{DBTable_Game::NAME, DBTable_Game::COLUMN_LIST},
                                                                        {DBTable_Add_App::NAME, DBTable_Add_App::COLUMN_LIST},
                                                                        {DBTable_Playlist::NAME, DBTable_Playlist::COLUMN_LIST},
                                                                        {DBTable_Playlist_Game::NAME, DBTable_Playlist_Game::COLUMN_LIST}};
    static inline const QString GENERAL_QUERY_SIZE_COMMAND = "COUNT(1)";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mLogosDirectory;
    QDir mScreenshotsDirectory;
    std::unique_ptr<QFile> mDatabaseFile;
    std::unique_ptr<QFile> mMainEXEFile;
    std::unique_ptr<QFile> mCLIFpEXEFile;

    // Database information
    QStringList mPlatformList;
    QStringList mPlaylistList;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~Install();

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static bool pathIsValidtInstall(QString installPath);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool matchesTargetVersion() const;
    QSqlDatabase openDatabaseConnection();
    void closeDatabaseConnection();
    QSqlError checkDatabaseForRequiredTables(QSet<QString>& missingTablesBuffer) const;
    QSqlError checkDatabaseForRequiredColumns(QSet<QString>& missingColumsBuffer) const;
    QSqlError populateAvailableItems();
    bool deployCLIFp(QString &errorMessage);

    QSqlError initialGameQuery(QList<DBQueryBuffer>& resultBuffer, QSet<QString> selectedPlatforms) const;
    QSqlError initialAddAppQuery(DBQueryBuffer& resultBuffer) const;
    QSqlError initialPlaylistQuery(DBQueryBuffer& resultBuffer, QSet<QString> selectedPlaylists) const;
    QSqlError initialPlaylistGameQuery(QList<QPair<DBQueryBuffer, FP::Playlist>>& resultBuffer, const QList<FP::Playlist>& knownPlaylistsToQuery) const;

    QString getPath() const;
    QStringList getPlatformList() const;
    QStringList getPlaylistList() const;
    QDir getLogosDirectory() const;
    QDir getScrenshootsDirectory() const;
    QString getOFLIbPath() const;
};

}



#endif // FLASHPOINTINSTALL_H
