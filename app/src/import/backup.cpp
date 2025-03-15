// Unit Includes
#include "backup.h"

// Qt Includes
#include <QFile>
#include <QFileInfo>

// Qx Includes
#include <qx/io/qx-common-io.h>

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
    // Prevent double+ backups (THIS IS CRITICAL, HENCE WHY A HASH IS USED)
    if(mRevertables.contains(path))
        return BackupError();

    // Note revertable
    mRevertables[path] = false;

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

BackupError BackupManager::restore(RevertItr itr)
{
    Q_ASSERT(itr != mRevertables.cend());

    const QString path = itr.key();
    mRevertables.erase(itr);
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
    auto store = mRevertables.constFind(path);
    if(store == mRevertables.cend())
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
        mRevertables[dst] = false;

    return BackupError();
}

BackupError BackupManager::revertableTouch(const QString& path)
{
    if(!Qx::createFile(path))
        return BackupError(BackupError::FileWontCreate, path);

    mRevertables[path] = false;
    return BackupError();
}

// - Backs up the file at 'path' via copy, and then deletes the backup at the end of the import
// - File is restored on revert.
BackupError BackupManager::revertableRemove(const QString& path)
{
    // TODO: Use this for AM extra files
    QString backupPath = filePathToBackupPath(path);

    if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
    {
        if(!QFile::remove(backupPath))
            return BackupError(BackupError::FileWontDelete, backupPath);
    }

    if(!QFile::rename(path, backupPath))
        return BackupError(BackupError::FileWontBackup, path);

    mRevertables[path] = true;
    return BackupError();
}

bool BackupManager::hasReversions() const { return !mRevertables.isEmpty(); }
int BackupManager::revertQueueCount() const { return mRevertables.size(); }

int BackupManager::revertNextChange(BackupError& error, bool skipOnFail)
{
    // Ensure error message is null
    error = BackupError();

    // Delete new files and restore backups if present
    if(!mRevertables.isEmpty())
    {
        BackupError rErr = restore(mRevertables.cbegin());
        if(rErr && !skipOnFail)
            error = rErr;

        return mRevertables.size();
    }

    // Return 0 if all empty (shouldn't be reached if function is used correctly)
    qWarning("Reversion function called with no reverts left!");
    return 0;
}

void BackupManager::purge()
{
    for(auto itr = mRevertables.cbegin(); itr != mRevertables.cend();)
    {
        bool purge = itr.value();
        if(purge)
            QFile::remove(itr.key());
        itr = mRevertables.erase(itr); // clazy:exclude=strict-iterators
    }
}

}
