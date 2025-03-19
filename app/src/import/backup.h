#ifndef IMPORT_BACKUP_H
#define IMPORT_BACKUP_H

// Qt Includes
#include <QString>
#include <QSet>

// Qx Includes
#include <qx/core/qx-abstracterror.h>

using namespace Qt::StringLiterals;

/*  TODO: The approach, or at least the language around doing a full revert (i.e. emptying the revert
 *  queue) could use some touch-up.
 */

namespace Import
{

class QX_ERROR_TYPE(BackupError, "Lr::BackupError", 1301)
{
    friend class BackupManager;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        FileWontDelete,
        FileWontRestore,
        FileWontBackup,
        FileWontReplace,
        FileWontCreate
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {FileWontDelete, u"Cannot remove a file. It may need to be deleted manually."_s},
        {FileWontRestore, u"Cannot restore a file backup. It may need to be renamed manually.."_s},
        {FileWontBackup, u"Cannot backup file."_s},
        {FileWontReplace, u"A file that was part of a safe replace operation could not be transferred."_s},
        {FileWontCreate, u"A file that was part of a safe touch operation could not be created."_s}
    };

    static inline const QString CAPTION_REVERT_ERR = u"Error managing backups"_s;

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    BackupError(Type t, const QString& s);

public:
    BackupError();

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
    QString deriveCaption() const override;
};

class BackupManager
{
//-Aliases---------------------------------------------------------------------------
private:
    using OriginalPath = QString;
    using Purge = bool;
    using Reverts = QHash<OriginalPath, Purge>;
    using RevertItr = Reverts::const_iterator;

//-Class Variables-------------------------------------------------------------------
private:
    // Files
    static inline const QString BACKUP_FILE_EXT = u"fbk"_s;

//-Instance Variables-------------------------------------------------------------
private:
    Reverts mRevertables;

//-Constructor-------------------------------------------------------------
private:
    BackupManager();

//-Class Functions-------------------------------------------------------------
private:
    static QString filePathToBackupPath(const QString& filePath);

public:
    static BackupManager* instance();

//-Instance Functions-------------------------------------------------------------
private:
    BackupError backup(const QString& path, bool (*fn)(const QString& a, const QString& b));
    BackupError restore(RevertItr itr);

public:
    // - If it exists, backs up 'path' via copy, original remains in place to be worked on
    // - Backup is restored on revert
    // - 'path' is marked such that any new file placed there is deleted on revert
    BackupError backupCopy(const QString& path);

    // - If it exists, backs up 'path' via rename, original remains in place to be worked on
    // - Backup is restored on revert
    // - 'path' is marked such that any new file placed there is deleted on revert
    BackupError backupRename(const QString& path);

    // - Immediately restores a backed up file using its original path
    BackupError restore(const QString& path);

    // - Replaces dst with src (via a copy or symlink)
    // - dst, if present, if temporarily backed up in case the replacement fails and is immediately
    //   is restored if so. If the replacement succeeds the backup is immediately deleted
    // - If dst is new (did not originally exist), the file is marked as revertable for if
    //   a revert occurs
    BackupError safeReplace(const QString& src, const QString& dst, bool symlink);

    // - Creates an empty file at 'path' (fails if it already exists) and marks it for deletion
    //   in the event of a revert
    BackupError revertableTouch(const QString& path);

    // - Backs up the file at 'path' via copy, and then deletes the backup at the end of the import
    // - File is restored on revert.
    BackupError revertableRemove(const QString& path);

    bool hasReversions() const;
    int revertQueueCount() const;
    int revertNextChange(BackupError& error, bool skipOnFail);
    void purge();
};

}

#endif // IMPORT_BACKUP_H
