// Unit Include
#include "lr-install-interface.h"

namespace Lr
{

//===============================================================================================================
// RevertError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
RevertError::RevertError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//Public:
RevertError::RevertError() :
    mType(NoError)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool RevertError::isValid() const { return mType != NoError; }
QString RevertError::specific() const { return mSpecific; }
RevertError::Type RevertError::type() const { return mType; }

//Private:
Qx::Severity RevertError::deriveSeverity() const { return Qx::Err; }
quint32 RevertError::deriveValue() const { return mType; }
QString RevertError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString RevertError::deriveSecondary() const { return mSpecific; }
QString RevertError::deriveCaption() const { return CAPTION_REVERT_ERR; }

//===============================================================================================================
// IInstall
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
IInstall::IInstall(const QString& installPath) :
    mValid(false), // Path is invalid until proven otherwise
    mRootDirectory(installPath)
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
IInstall::~IInstall() {}

//Public:
QString IInstall::filePathToBackupPath(const QString& filePath)
{
    return filePath + '.' + BACKUP_FILE_EXT;
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
bool IInstall::containsAnyDataDoc(IDataDoc::Type type, const QList<QString>& names) const
{
    // Create identifier set of names
    QSet<IDataDoc::Identifier> searchSet;
    for(const QString& docName : names)
        searchSet << IDataDoc::Identifier(type, translateDocName(docName, type));

    // Cross reference with existing documents
    return mExistingDocuments.intersects(searchSet);
}

bool IInstall::supportsImageMode(Import::ImageMode imageMode) const { return preferredImageModeOrder().contains(imageMode); }

QList<QString> IInstall::modifiedDataDocs(IDataDoc::Type type) const
{
    QList<QString> modList;

    for(const IDataDoc::Identifier& dataDocId : mModifiedDocuments)
        if(dataDocId.docType() == type)
            modList.append(dataDocId.docName());

    return modList;
}

//Protected:
void IInstall::nullify()
{
    mValid = false;
    mRootDirectory = QDir();
}

void IInstall::declareValid(bool valid)
{
    mValid = valid;
    if(!valid)
        nullify();
}

void IInstall::catalogueExistingDoc(IDataDoc::Identifier existingDoc) { mExistingDocuments.insert(existingDoc); }

DocHandlingError IInstall::checkoutDataDocument(IDataDoc* docToOpen, std::shared_ptr<IDataDoc::Reader> docReader)
{
    // Error report to return
    DocHandlingError openReadError; // Defaults to no error

    // Check if lease is already out
    if(mLeasedDocuments.contains(docToOpen->identifier()))
        openReadError = DocHandlingError(*docToOpen, DocHandlingError::DocAlreadyOpen);
    else
    {
        // Read existing file if present and a reader was provided
        if(docReader && mExistingDocuments.contains(docToOpen->identifier()))
             openReadError = docReader->readInto();

        // Add lease to ledger if no error occurred while reading
        if(!openReadError.isValid())
            mLeasedDocuments.insert(docToOpen->identifier());
    }

    // Return opened document and status
    return openReadError;
}

DocHandlingError IInstall::commitDataDocument(IDataDoc* docToSave, std::shared_ptr<IDataDoc::Writer> docWriter)
{
    IDataDoc::Identifier id = docToSave->identifier();

    // Check if the doc was saved previously to prevent double-backups
    bool wasDeleted = mDeletedDocuments.contains(id);
    bool wasModified = mModifiedDocuments.contains(id);
    bool wasUntouched = !wasDeleted && !wasModified;

    // Handle backup/revert prep
    if(wasUntouched)
    {
        QString docPath = docToSave->path();
        mRevertableFilePaths.append(docPath); // Correctly handles if doc ends up deleted

        // Backup
        if(QFile::exists(docPath))
        {
            QString backupPath = filePathToBackupPath(docPath);

            if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
            {
                if(!QFile::remove(backupPath))
                    return DocHandlingError(*docToSave, DocHandlingError::CantRemoveBackup);
            }

            if(!QFile::copy(docPath, backupPath))
                return DocHandlingError(*docToSave, DocHandlingError::CantCreateBackup);
        }
    }

    // Error State
    DocHandlingError commitError;

    // Handle modification
    if(!docToSave->isEmpty())
    {
        mModifiedDocuments.insert(id);
        if(wasDeleted)
            mDeletedDocuments.remove(id);

        commitError = docWriter->writeOutOf();
        ensureModifiable(docToSave->path());
    }
    else // Handle deletion
    {
        mDeletedDocuments.insert(id);
        if(wasModified)
            mModifiedDocuments.remove(id);
    }

    // Remove handle reservation
    mLeasedDocuments.remove(docToSave->identifier());

    // Return write status and let document ptr auto delete
    return commitError;
}

QList<QString> IInstall::modifiedPlatforms() const { return modifiedDataDocs(IDataDoc::Type::Platform); }
QList<QString> IInstall::modifiedPlaylists() const { return modifiedDataDocs(IDataDoc::Type::Playlist); }

//Public:
QString IInstall::versionString() const { return u"Unknown Version"_s; }
bool IInstall::isValid() const { return mValid; }
QString IInstall::path() const { return mRootDirectory.absolutePath(); }

void IInstall::softReset()
{
    mRevertableFilePaths.clear();
    mModifiedDocuments.clear();
    mDeletedDocuments.clear();
    mLeasedDocuments.clear();
}

QString IInstall::translateDocName(const QString& originalName, IDataDoc::Type type) const
{
    Q_UNUSED(type);
    return originalName;
}

Qx::Error IInstall::refreshExistingDocs(bool* changed)
{
    QSet<IDataDoc::Identifier> oldDocSet;
    oldDocSet.swap(mExistingDocuments);
    Qx::Error error = populateExistingDocs();
    if(changed)
        *changed = mExistingDocuments != oldDocSet;
    return error;
}

bool IInstall::containsPlatform(const QString& name) const
{
    return containsAnyDataDoc(IDataDoc::Type::Platform, {name});
}

bool IInstall::containsPlaylist(const QString& name) const
{
    return containsAnyDataDoc(IDataDoc::Type::Playlist, {name});
}

bool IInstall::containsAnyPlatform(const QList<QString>& names) const
{
    return containsAnyDataDoc(IDataDoc::Type::Platform, names);
}

bool IInstall::containsAnyPlaylist(const QList<QString>& names) const
{
    return containsAnyDataDoc(IDataDoc::Type::Playlist, names);
}

void IInstall::addRevertableFile(const QString& filePath) { mRevertableFilePaths.append(filePath); }
int IInstall::revertQueueCount() const { return mRevertableFilePaths.size(); }

int IInstall::revertNextChange(RevertError& error, bool skipOnFail)
{
    // Ensure error message is null
    error = RevertError();

    // Get operation count for return
    int operationsLeft = mRevertableFilePaths.size();

    // Delete new files and restore backups if present
    if(!mRevertableFilePaths.isEmpty())
    {
        QString filePath = mRevertableFilePaths.takeFirst();
        QString backupPath = filePathToBackupPath(filePath);

        if(QFile::exists(filePath) && !QFile::remove(filePath) && !skipOnFail)
        {
            error = RevertError(RevertError::FileWontDelete, filePath);
            return operationsLeft;
        }

        if(!QFile::exists(filePath) && QFile::exists(backupPath) && !QFile::rename(backupPath, filePath) && !skipOnFail)
        {
            error = RevertError(RevertError::FileWontRestore, backupPath);
            return operationsLeft;
        }

        // Decrement op count
        return operationsLeft - 1;
    }

    // Return 0 if all empty (shouldn't be reached if function is used correctly)
    return 0;
}

/* These functions can be overridden by children as needed.
 * Work within them should be kept as minimal as possible since they are not accounted
 * for by the import progress indicator.
 */
Qx::Error IInstall::preImport() { return {}; }
Qx::Error IInstall::postImport() { return {}; }
Qx::Error IInstall::prePlatformsImport() { return {}; }
Qx::Error IInstall::postPlatformsImport() { return {}; }
Qx::Error IInstall::preImageProcessing() { return {}; }
Qx::Error IInstall::postImageProcessing() { return {}; }
Qx::Error IInstall::prePlaylistsImport() { return {}; }
Qx::Error IInstall::postPlaylistsImport() { return {}; }

QString IInstall::platformCategoryIconPath() const { return QString(); } // Unsupported in default implementation
std::optional<QDir> IInstall::platformIconsDirectory() const { return std::nullopt; } // Unsupported in default implementation
std::optional<QDir> IInstall::playlistIconsDirectory() const { return std::nullopt; } // Unsupported in default implementation

}
