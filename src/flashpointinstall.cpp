#include "flashpointinstall.h"
#include <QFileInfo>
#include "qx.h"

//===============================================================================================================
// FLASHPOINT INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
FlashpointInstall::FlashpointInstall(QString installPath)
{
    // Ensure instance will be valid
    if(!pathIsValidFlashpointInstall(installPath))
        assert("Cannot create a FlashpointInstall instance with an invalid installPath. Check first with FlashpointInstall::pathIsValidFlashpointInstall(QString).");

    // Initialize files and directories;
    mRootDirectory = QDir(installPath);
    mLogosDirectory = QDir(installPath + "/" + LOGOS_PATH);
    mScreenshotsDirectory = QDir(installPath + "/" + SCREENSHOTS_PATH);
    mDatabaseFile = std::make_unique<QFile>(installPath + "/" + DATABASE_PATH);
    mMainEXEFile = std::make_unique<QFile>(installPath + "/" + MAIN_EXE_PATH);

    // Create database connection
    QSqlDatabase fpDB = QSqlDatabase::addDatabase("QSQLITE", DATABASE_CONNECTION_NAME);
    fpDB.setConnectOptions("QSQLITE_OPEN_READONLY");
    fpDB.setDatabaseName(mDatabaseFile->fileName());
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
FlashpointInstall::~FlashpointInstall()
{
    closeDatabaseConnection();
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
bool FlashpointInstall::pathIsValidFlashpointInstall(QString installPath)
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
bool FlashpointInstall::matchesTargetVersion()
{
    mMainEXEFile->open(QFile::ReadOnly);
    QByteArray mainEXEFileData = mMainEXEFile->readAll();
    mMainEXEFile->close();
    return Qx::Integrity::generateChecksum(mainEXEFileData, QCryptographicHash::Sha256) == TARGET_EXE_SHA256;
}

QSqlDatabase FlashpointInstall::openDatabaseConnection()
{
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME, false);

    if(fpDB.isValid())
        if(fpDB.open())
            return fpDB;

    // Default return - Only reached on failure
    return QSqlDatabase();
}

void FlashpointInstall::closeDatabaseConnection() { QSqlDatabase::database(DATABASE_CONNECTION_NAME, false).close(); }


QSqlError FlashpointInstall::checkDatabaseForRequiredTables(QSet<QString>& missingTablesReturnBuffer)
{
    // Prep return buffer
    missingTablesReturnBuffer.clear();

    for(QPair<QString, QSet<QString>> tableAndColumns : DATABASE_TABLE_COLUMN_SET)
        missingTablesReturnBuffer.insert(tableAndColumns.first);

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

QSqlError FlashpointInstall::checkDatabaseForRequiredColumns(QSet<QString> &missingColumsReturnBuffer)
{

    // Ensure return buffer starts empty
    missingColumsReturnBuffer.clear();

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Ensure each table has the required columns
    QSet<QString> existingColumns;

    for(QPair<QString, QSet<QString>> tableAndColumns : DATABASE_TABLE_COLUMN_SET)
    {
        // Clear previous data
        existingColumns.clear();

        // Make column name query
        QSqlQuery columnQuery("PRAGMA table_info(" + tableAndColumns.first + ")", fpDB);

        // Return if error occurs
        if(columnQuery.lastError().isValid())
            return columnQuery.lastError();

        // Parse query
        while(columnQuery.next())
            existingColumns.insert(columnQuery.value("name").toString());

        // Check for missing columns
        for(QString column : tableAndColumns.second)
            if(!existingColumns.contains(column))
                missingColumsReturnBuffer.insert(tableAndColumns.first + ": " + column);
    }


    // Return invalid SqlError
    return QSqlError();
}

QSqlError FlashpointInstall::populatePlatforms()
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

    // Return invalid SqlError
    return QSqlError();
}

QSqlError FlashpointInstall::populatePlaylists()
{
    // Get database and create reference query model
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Make platform query
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

QSqlError FlashpointInstall::initialGameQuery(QList<std::tuple<QString, QSqlQuery, int>>& platform_query_sizeListBuffer, QStringList selectedPlatforms)
{
    // Ensure return buffer is reset
    platform_query_sizeListBuffer.clear();

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    for(QString platform : selectedPlatforms)
    {
        // Query all games for the current platform (TODO: Restrict the columns of this query to only those actually used)

        QSqlQuery initialQuery("SELECT * FROM " + DBTable_Game::NAME + " WHERE " +
                               DBTable_Game::COL_PLATFORM + " = '" + platform + "'" , fpDB);

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
        platform_query_sizeListBuffer.append(std::make_tuple(platform, initialQuery, querySize));
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError FlashpointInstall::initialAdditionalAppQuery(std::pair<QSqlQuery, int>& query_sizeBuffer)
{
    // Ensure return buffer is null
    query_sizeBuffer = std::make_pair(QSqlQuery(), -1);

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Make query (TODO: Restrict the columns of this query to only those actually used)
    QSqlQuery initialQuery("SELECT * FROM " + DBTable_Additonal_App::NAME + " WHERE " +
                           DBTable_Additonal_App::COL_APP_PATH + " != '" + DBTable_Additonal_App::ENTRY_EXTRAS + "'", fpDB);

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
    query_sizeBuffer = std::make_pair(initialQuery, querySize);

    // Return invalid SqlError
    return QSqlError();
}

QSqlError FlashpointInstall::initialPlaylistQuery(std::pair<QSqlQuery, int>& query_sizeBuffer, QStringList selectedPlaylists)
{
    // Ensure return buffer is null
    query_sizeBuffer = std::make_pair(QSqlQuery(), -1);

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    // Query all selected playlists (TODO: Restrict the columns of this query to only those actually used)
    QSqlQuery initialQuery("SELECT * FROM " + DBTable_Playlist::NAME + " WHERE " +
                           DBTable_Playlist::COL_TITLE + " IN ('" + selectedPlaylists.join(",'") + "')", fpDB);

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
    query_sizeBuffer = std::make_pair(initialQuery, querySize);

    // Return invalid SqlError
    return QSqlError();
}

QSqlError FlashpointInstall::initialPlaylistGameQuery(QList<std::tuple<QString, QSqlQuery, int>>& playlist_query_sizeListBuffer, QList<std::pair<QString, QString>> playlistNamesAndIDs)
{
    // Ensure return buffer is empty
    playlist_query_sizeListBuffer.clear();

    // Get database
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);

    for(std::pair<QString, QString> playlistNameAndID : playlistNamesAndIDs)
    {
        // Query all games for the current playlist (TODO: Restrict the columns of this query to only those actually used)
        QSqlQuery initialQuery("SELECT * FROM " + DBTable_Playlist_Game::NAME + " WHERE " +
                               DBTable_Playlist_Game::COL_ID + " = '" + playlistNameAndID.second + "'", fpDB);

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
        playlist_query_sizeListBuffer.append(std::make_tuple(playlistNameAndID.first, initialQuery, querySize));
    }

    // Return invalid SqlError
    return QSqlError();
}

QStringList FlashpointInstall::getPlatformList() { return mPlatformList; }

QStringList FlashpointInstall::getPlaylistList() { return  mPlaylistList; }
