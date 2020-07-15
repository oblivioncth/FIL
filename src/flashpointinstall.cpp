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

    // Get database and create reference query model
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);
    std::unique_ptr<QSqlQueryModel> databaseQueryModel = std::make_unique<QSqlQueryModel>();

    // Ensure each table has the required columns
    QSet<QString> existingColumns;

    for(QPair<QString, QSet<QString>> tableAndColumns : DATABASE_TABLE_COLUMN_SET)
    {
        // Clear previous data
        existingColumns.clear();

        // Make column name query
        databaseQueryModel->setQuery("PRAGMA table_info(" + tableAndColumns.first + ")", fpDB);

        // Return if error occurs
        if(databaseQueryModel->lastError().isValid())
            return databaseQueryModel->lastError();

        // Parse query
        for(int i = 0; i < databaseQueryModel->rowCount(); i++)
            existingColumns.insert(databaseQueryModel->record(i).value("name").toString());

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
    // Get database and create reference query model
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);
    std::unique_ptr<QSqlQueryModel> databaseQueryModel = std::make_unique<QSqlQueryModel>();

    // Make platform query
    databaseQueryModel->setQuery("SELECT DISTINCT " + DBTable_Game::COL_PLATFORM + " FROM " + DBTable_Game::NAME, fpDB);

    // Return if error occurs
    if(databaseQueryModel->lastError().isValid())
        return databaseQueryModel->lastError();

    // Parse query
    for(int i = 0; i < databaseQueryModel->rowCount(); i++)
        mPlatformList.append(databaseQueryModel->record(i).value(DBTable_Game::COL_PLATFORM).toString());

    // Sort list
    mPlatformList.sort();

    // Return invalid SqlError
    return QSqlError();
}

QSqlError FlashpointInstall::populatePlaylists()
{
    // Get database and create reference query model
    QSqlDatabase fpDB = QSqlDatabase::database(DATABASE_CONNECTION_NAME);
    std::unique_ptr<QSqlQueryModel> databaseQueryModel = std::make_unique<QSqlQueryModel>();

    // Make platform query
    databaseQueryModel->setQuery("SELECT DISTINCT " + DBTable_Playlist::COL_TITLE + " FROM " + DBTable_Playlist::NAME, fpDB);

    // Return if error occurs
    if(databaseQueryModel->lastError().isValid())
        return databaseQueryModel->lastError();

    // Parse query
    for(int i = 0; i < databaseQueryModel->rowCount(); i++)
        mPlaylistList.append(databaseQueryModel->record(i).value(DBTable_Playlist::COL_TITLE).toString());

    // Sort list
    mPlaylistList.sort();

    // Return invalid SqlError
    return QSqlError();
}

QStringList FlashpointInstall::getPlatformList() { return mPlatformList; }

QStringList FlashpointInstall::getPlaylistList() { return  mPlaylistList; }
