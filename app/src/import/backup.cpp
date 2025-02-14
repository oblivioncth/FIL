// Unit Includes
#include "backup.h"

// Qt Includes
#include <QFile>
#include <QFileInfo>

namespace Import
{

//===============================================================================================================
// BackupError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
BackupError::BackupError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//Public:
BackupError::BackupError() :
    mType(NoError)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool BackupError::isValid() const { return mType != NoError; }
QString BackupError::specific() const { return mSpecific; }
BackupError::Type BackupError::type() const { return mType; }

//Private:
Qx::Severity BackupError::deriveSeverity() const { return Qx::Err; }
quint32 BackupError::deriveValue() const { return mType; }
QString BackupError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString BackupError::deriveSecondary() const { return mSpecific; }
QString BackupError::deriveCaption() const { return CAPTION_REVERT_ERR; }

//===============================================================================================================
// BackupManager
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
BackupManager::BackupManager() {}

//-Class Functions-------------------------------------------------------------
//Private:
QString BackupManager::filePathToBackupPath(const QString& filePath)
{
    return filePath + '.' + BACKUP_FILE_EXT;
}

//Public:
BackupManager* BackupManager::instance() { static BackupManager inst; return &inst; }

//-Instance Functions-------------------------------------------------------------
//Private:
BackupError BackupManager::backup(const QString& path, bool (*fn)(const QString& a, const QString& b))
{
    // Prevent double+ backups (THIS IS CRITICAL, HENCE WHY A SET IS USED)
    if(mRevertablePaths.contains(path))
        return BackupError();

    // Note revertable
    mRevertablePaths.insert(path);

    // Backup if exists
    if(QFile::exists(path))
    {
        QString backupPath = filePathToBackupPath(path);

        if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
        {
            if(!QFile::remove(backupPath))
                return BackupError(BackupError::FileWontDelete, backupPath);
        }

        if(!fn(path, backupPath))
            return BackupError(BackupError::FileWontBackup, path);
    }

    return BackupError();
}

BackupError BackupManager::restore(QSet<QString>::const_iterator pathItr)
{
    Q_ASSERT(pathItr != mRevertablePaths.cend());

    const QString path = *pathItr;
    mRevertablePaths.erase(pathItr);
    QString backupPath = filePathToBackupPath(path);

    if(QFile::exists(path) && !QFile::remove(path))
        return BackupError(BackupError::FileWontDelete, path);

    if(!QFile::exists(path) && QFile::exists(backupPath) && !QFile::rename(backupPath, path))
        return BackupError(BackupError::FileWontRestore, backupPath);

    return BackupError();
}

//Public:
BackupError BackupManager::backupCopy(const QString& path)
{
    return backup(path, [](const QString& a, const QString& b){ return QFile::copy(a, b); });
}

BackupError BackupManager::backupRename(const QString& path)
{
    return backup(path, [](const QString& a, const QString& b){ return QFile::rename(a, b); });
}

BackupError BackupManager::restore(const QString& path)
{
    auto store = mRevertablePaths.constFind(path);
    if(store == mRevertablePaths.cend())
        return BackupError();

    return restore(store);
}

BackupError BackupManager::safeReplace(const QString& src, const QString& dst, bool symlink)
{
    // Maybe make sure destination folder exists here?

    // Backup
    QString backupPath = filePathToBackupPath(dst);
    bool dstOccupied = QFile::exists(dst);
    if(dstOccupied)
        if(!QFile::rename(dst, backupPath)) // Temp backup
            return BackupError(BackupError::FileWontBackup, dst);

    // Replace
    std::error_code replaceError;
    if(symlink)
        std::filesystem::create_symlink(src.toStdString(), dst.toStdString(), replaceError);
    else
        replaceError = QFile::copy(src, dst) ? std::error_code() : std::make_error_code(std::io_errc::stream);

    // Restore on fail
    if(replaceError)
    {
        if(dstOccupied)
            QFile::rename(backupPath, dst);
        return BackupError(BackupError::FileWontReplace, src);
    }

    // Remove backup immediately
    if(dstOccupied)
        QFile::remove(backupPath);
    else // Mark new files (only) as revertible so that existing ones will remain in the event of a revert
        mRevertablePaths.insert(dst);

    return BackupError();
}

int BackupManager::revertQueueCount() const { return mRevertablePaths.size(); }

int BackupManager::revertNextChange(BackupError& error, bool skipOnFail)
{
    // Ensure error message is null
    error = BackupError();

    // Delete new files and restore backups if present
    if(!mRevertablePaths.isEmpty())
    {
        BackupError rErr = restore(mRevertablePaths.cbegin());
        if(rErr && !skipOnFail)
            error = rErr;

        return mRevertablePaths.size();
    }

    // Return 0 if all empty (shouldn't be reached if function is used correctly)
    qWarning("Reversion function called with no reverts left!");
    return 0;
}

}
