#ifndef BACKUP_H
#define BACKUP_H

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
        FileWontReplace
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {FileWontDelete, u"Cannot remove a file. It may need to be deleted manually."_s},
        {FileWontRestore, u"Cannot restore a file backup. It may need to be renamed manually.."_s},
        {FileWontBackup, u"Cannot backup file."_s},
        {FileWontReplace, u"A file that was part of a safe replace operation could not be transfered."_s}
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
//-Class Variables-----------------------------------------------------------------------------------------------
private:
    // Files
    static inline const QString BACKUP_FILE_EXT = u"fbk"_s;

//-Instance Variables-------------------------------------------------------------
private:
    QSet<QString> mRevertablePaths;

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
    BackupError restore(QSet<QString>::const_iterator pathItr);

public:
    BackupError backupCopy(const QString& path);
    BackupError backupRename(const QString& path);
    BackupError restore(const QString& path);
    BackupError safeReplace(const QString& src, const QString& dst, bool symlink);

    int revertQueueCount() const;
    int revertNextChange(BackupError& error, bool skipOnFail);
};

}

#endif // BACKUP_H
