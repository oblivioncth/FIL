#include "flashpointinstall.h"
#include "qx-io.h"

namespace FP
{

//===============================================================================================================
// INSTALL::OFLIb
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QString Install::OFLIb::parametersFromStandard(QString originalAppPath, QString originalAppParams)
{
    if(originalAppPath == DBTable_Add_App::ENTRY_MESSAGE)
        return MSG_ARG.arg(originalAppParams);
    else
        return APP_ARG.arg(originalAppPath) + " " + PARAM_ARG.arg(originalAppParams);
}

//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath)
{
    // Ensure instance will be valid
    if(!pathIsValidtInstall(installPath))
        assert("Cannot create a Install instance with an invalid installPath. Check first with Install::pathIsValidInstall(QString).");

    // Initialize files and directories;
    mRootDirectory = QDir(installPath);
    mLogosDirectory = QDir(installPath + "/" + LOGOS_PATH);
    mScreenshotsDirectory = QDir(installPath + "/" + SCREENSHOTS_PATH);
    mDatabaseFile = std::make_unique<QFile>(installPath + "/" + DATABASE_PATH);
    mMainEXEFile = std::make_unique<QFile>(installPath + "/" + MAIN_EXE_PATH);
    mOFLIbEXEFile = std::make_unique<QFile>(installPath + "/" + OFLIb::EXE_NAME);

    // Create database connection
    QSqlDatabase fpDB = QSqlDatabase::addDatabase("QSQLITE", DATABASE_CONNECTION_NAME);
    fpDB.setConnectOptions("QSQLITE_OPEN_READONLY");
    fpDB.setDatabaseName(mDatabaseFile->fileName());
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
Install::~Install()
{
    closeDatabaseConnection();
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
bool Install::pathIsValidtInstall(QString installPath)
{
    QFileInfo logosFolder(installPath + "/" + LOGOS_PATH);
    QFileInfo screenshotsFolder(installPath + "/" + SCREENSHOTS_PATH);
    QFileInfo database(installPath + "/" + DATABASE_PATH);
    QFileInfo mainEXE(installPath + "/" + MAIN_EXE_PATH);

    return logosFolder.exists() && logosFolder.isDir() &&
           screenshotsFolder.exists() && screenshotsFolder.isDir() &&
           database.exists() && database.isFile() &&
           mainEXE.exists() && mainEXE.isExecutable();
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
bool Install::matchesTargetVersion() const
{
    mMainEXEFile->open(QFile::ReadOnly);
    QByteArray mainEXEFileData = mMainEXEFile->readAll();
    mMainEXEFile->close();
    return Qx::Integrity::generateChecksum(mainEXEFileData, QCryptographicHash::Sha256) == TARGET_EXE_SHA256;
}

QSqlDatabase Install::openDatabaseConnection()
{
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME, false);

    if(fpDB.isValid())
        if(fpDB.open())
            return fpDB;

    // Default return - Only reached on failure
    return QSqlDatabase();
}

void Install::closeDatabaseConnection() { QSqlDatabase::database(DATABASE_CONNECTION_NAME, false).close(); }


QSqlError Install::checkDatabaseForRequiredTables(QSet<QString>& missingTablesReturnBuffer) const
{
    // Prep return buffer
    missingTablesReturnBuffer.clear();

    for(DBTableSpecs tableAndColumns : DATABASE_SPECS_LIST)
        missingTablesReturnBuffer.insert(tableAndColumns.name);

    // Get tables from DB
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);
    QStringList existingTables = fpDB.tables();

    // Return if DB error occured
    if(fpDB.lastError().isValid())
        return fpDB.lastError();

    for(QString table : existingTables)
        missingTablesReturnBuffer.remove(table);

    // Return an invalid error
    return  QSqlError();
}

QSqlError Install::checkDatabaseForRequiredColumns(QSet<QString> &missingColumsReturnBuffer) const
{

    // Ensure return buffer starts empty
    missingColumsReturnBuffer.clear();

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Ensure each table has the required columns
    QSet<QString> existingColumns;

    for(DBTableSpecs tableAndColumns : DATABASE_SPECS_LIST)
    {
        // Clear previous data
        existingColumns.clear();

        // Make column name query
        QSqlQuery columnQuery("PRAGMA table_info(" + tableAndColumns.name + ")", fpDB);

        // Return if error occurs
        if(columnQuery.lastError().isValid())
            return columnQuery.lastError();

        // Parse query
        while(columnQuery.next())
            existingColumns.insert(columnQuery.value("name").toString());

        // Check for missing columns
        for(QString column : tableAndColumns.columns)
            if(!existingColumns.contains(column))
                missingColumsReturnBuffer.insert(tableAndColumns.name + ": " + column);
    }


    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::populateAvailableItems()
{
    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Make platform query
    QSqlQuery platformQuery("SELECT DISTINCT " + DBTable_Game::COL_PLATFORM + " FROM " + DBTable_Game::NAME, fpDB);

    // Return if error occurs
    if(platformQuery.lastError().isValid())
        return platformQuery.lastError();

    // Parse query
    while(platformQuery.next())
        mPlatformList.append(platformQuery.value(DBTable_Game::COL_PLATFORM).toString());

    // Sort list
    mPlatformList.sort();

    // Make playlist query
    QSqlQuery playlistQuery("SELECT DISTINCT " + DBTable_Playlist::COL_TITLE + " FROM " + DBTable_Playlist::NAME, fpDB);

    // Return if error occurs
    if(playlistQuery.lastError().isValid())
        return playlistQuery.lastError();

    // Parse query
    while(playlistQuery.next())
        mPlaylistList.append(playlistQuery.value(DBTable_Playlist::COL_TITLE).toString());

    // Sort list
    mPlaylistList.sort();

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::initialGameQuery(QList<DBQueryBuffer>& resultBuffer, QStringList selectedPlatforms) const
{
    // Ensure return buffer is reset
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    for(QString platform : selectedPlatforms)
    {
        // Query all games for the current platform
        QSqlQuery initialQuery("SELECT " + DBTable_Game::COLUMN_LIST.join(",") + " FROM " + DBTable_Game::NAME + " WHERE " +
                               DBTable_Game::COL_PLATFORM + " = '" + platform + "' AND " +
                               DBTable_Game::COL_LIBRARY + " = '" + DBTable_Game::GAME_LIBRARY + "'", fpDB);

        // Return if error occurs
        if(initialQuery.lastError().isValid())
           return initialQuery.lastError();

        // Get size of the query result
        int querySize = 0;
        while(initialQuery.next())
            querySize++;

        // Reset query pointer
        initialQuery.seek(QSql::BeforeFirstRow);

        // Add result to buffer
        resultBuffer.append({platform, initialQuery, querySize});
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::initialAddAppQuery(DBQueryBuffer& resultBuffer) const
{
    // Ensure return buffer is effectively null
    resultBuffer.source = QString();
    resultBuffer.result = QSqlQuery();
    resultBuffer.size = -1;

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Make query (TODO: Restrict the columns of this query to only those actually used)
    QSqlQuery initialQuery("SELECT " + DBTable_Add_App::COLUMN_LIST.join(",") + " FROM " + DBTable_Add_App::NAME + " WHERE " +
                           DBTable_Add_App::COL_APP_PATH + " != '" + DBTable_Add_App::ENTRY_EXTRAS + "'", fpDB);

    // Return if error occurs
    if(initialQuery.lastError().isValid())
        return initialQuery.lastError();

    // Get size of the query result
    int querySize = 0;
    while(initialQuery.next())
        querySize++;

    // Reset query pointer
    initialQuery.seek(QSql::BeforeFirstRow);

    // Set buffer instance to result
    resultBuffer.source = DBTable_Add_App::NAME;
    resultBuffer.result = initialQuery;
    resultBuffer.size = querySize;

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::initialPlaylistQuery(DBQueryBuffer& resultBuffer, QStringList selectedPlaylists) const
{
    // Ensure return buffer is effectively null
    resultBuffer.source = QString();
    resultBuffer.result = QSqlQuery();
    resultBuffer.size = -1;

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Query all selected playlists
    QSqlQuery initialQuery("SELECT " + DBTable_Playlist::COLUMN_LIST.join(",") + " FROM " + DBTable_Playlist::NAME + " WHERE " +
                           DBTable_Playlist::COL_TITLE + " IN ('" + selectedPlaylists.join(",'") + "') AND " +
                           DBTable_Playlist::COL_LIBRARY + " = '" + DBTable_Playlist::GAME_LIBRARY + "'", fpDB);

    // Return if error occurs
    if(initialQuery.lastError().isValid())
        return initialQuery.lastError();

    // Get size of the query result
    int querySize = 0;
    while(initialQuery.next())
        querySize++;

    // Reset query pointer
    initialQuery.seek(QSql::BeforeFirstRow);

    // Set buffer instance to result
    resultBuffer.source = DBTable_Playlist::NAME;
    resultBuffer.result = initialQuery;
    resultBuffer.size = querySize;

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::initialPlaylistGameQuery(QList<QPair<DBQueryBuffer, FP::Playlist>>& resultBuffer, const QList<FP::Playlist>& knownPlaylistsToQuery) const
{
    // Ensure return buffer is empty
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    for(const FP::Playlist& playlist : knownPlaylistsToQuery)
    {
        // Query all games for the current playlist (TODO: Restrict the columns of this query to only those actually used)
        QSqlQuery initialQuery("SELECT " + DBTable_Playlist_Game::COLUMN_LIST.join(",") + " FROM " + DBTable_Playlist_Game::NAME + " WHERE " +
                               DBTable_Playlist_Game::COL_PLAYLIST_ID + " = '" + playlist.getID().toString(QUuid::WithoutBraces) + "'", fpDB);

        // Return if error occurs
        if(initialQuery.lastError().isValid())
            return initialQuery.lastError();

        // Get size of the query result
        int querySize = 0;
        while(initialQuery.next())
            querySize++;

        // Reset query pointer
        initialQuery.seek(QSql::BeforeFirstRow);

        // Add result to buffer
        resultBuffer.append(qMakePair(DBQueryBuffer{playlist.getTitle(), initialQuery, querySize}, playlist));
    }

    // Return invalid SqlError
    return QSqlError();
}

QStringList Install::getPlatformList() const { return mPlatformList; }
QStringList Install::getPlaylistList() const { return mPlaylistList; }
QDir Install::getLogosDirectory() const { return mLogosDirectory; }
QDir Install::getScrenshootsDirectory() const { return mScreenshotsDirectory; }
QString Install::getOFLIbPath() const { return mOFLIbEXEFile->fileName(); }

}
