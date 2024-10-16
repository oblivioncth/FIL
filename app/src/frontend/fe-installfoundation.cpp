// Unit Include
#include "fe-installfoundation.h"

namespace Fe
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
// InstallFoundation
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
InstallFoundation::InstallFoundation(const QString& installPath) :
    mValid(false), // Path is invalid until proven otherwise
    mRootDirectory(installPath)
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
InstallFoundation::~InstallFoundation() {}

//Public:
QString InstallFoundation::filePathToBackupPath(const QString& filePath)
{
    return filePath + '.' + BACKUP_FILE_EXT;
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
bool InstallFoundation::containsAnyDataDoc(DataDoc::Type type, const QList<QString>& names) const
{
    // Create identifier set of names
    QSet<DataDoc::Identifier> searchSet;
    for(const QString& docName : names)
        searchSet << DataDoc::Identifier(type, translateDocName(docName, type));

    // Cross reference with existing documents
    return mExistingDocuments.intersects(searchSet);
}

QList<QString> InstallFoundation::modifiedDataDocs(DataDoc::Type type) const
{
    QList<QString> modList;

    for(const DataDoc::Identifier& dataDocId : mModifiedDocuments)
        if(dataDocId.docType() == type)
            modList.append(dataDocId.docName());

    return modList;
}

//Protected:
void InstallFoundation::nullify()
{
    mValid = false;
    mRootDirectory = QDir();
}

void InstallFoundation::softReset()
{
    mRevertableFilePaths.clear();
    mModifiedDocuments.clear();
    mDeletedDocuments.clear();
    mLeasedDocuments.clear();
    mImportDetails.reset();
}

void InstallFoundation::declareValid(bool valid)
{
    mValid = valid;
    if(!valid)
        nullify();
}

QString InstallFoundation::translateDocName(const QString& originalName, DataDoc::Type type) const
{
    Q_UNUSED(type);
    return originalName;
}

void InstallFoundation::catalogueExistingDoc(DataDoc::Identifier existingDoc) { mExistingDocuments.insert(existingDoc); }

Fe::DocHandlingError InstallFoundation::checkoutDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDoc::Reader> docReader)
{
    // Error report to return
    Fe::DocHandlingError openReadError; // Defaults to no error

    // Check if lease is already out
    if(mLeasedDocuments.contains(docToOpen->identifier()))
        openReadError = Fe::DocHandlingError(*docToOpen, Fe::DocHandlingError::DocAlreadyOpen);
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

Fe::DocHandlingError InstallFoundation::commitDataDocument(DataDoc* docToSave, std::shared_ptr<DataDoc::Writer> docWriter)
{
    DataDoc::Identifier id = docToSave->identifier();
    bool wasDeleted = mDeletedDocuments.contains(id);
    bool wasModified = mDeletedDocuments.contains(id);
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
                    return Fe::DocHandlingError(*docToSave, Fe::DocHandlingError::CantRemoveBackup);
            }

            if(!QFile::copy(docPath, backupPath))
                return Fe::DocHandlingError(*docToSave, Fe::DocHandlingError::CantCreateBackup);
        }
    }

    // Error State
    Fe::DocHandlingError commitError;

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

QList<QString> InstallFoundation::modifiedPlatforms() const { return modifiedDataDocs(DataDoc::Type::Platform); }
QList<QString> InstallFoundation::modifiedPlaylists() const { return modifiedDataDocs(DataDoc::Type::Playlist);}

//Public:
bool InstallFoundation::isValid() const { return mValid; }
QString InstallFoundation::path() const { return mRootDirectory.absolutePath(); }

Qx::Error InstallFoundation::refreshExistingDocs(bool* changed)
{
    QSet<DataDoc::Identifier> oldDocSet;
    oldDocSet.swap(mExistingDocuments);
    Qx::Error error = populateExistingDocs();
    if(changed)
        *changed = mExistingDocuments != oldDocSet;
    return error;
}

bool InstallFoundation::containsPlatform(const QString& name) const
{
    return containsAnyDataDoc(DataDoc::Type::Platform, {name});
}

bool InstallFoundation::containsPlaylist(const QString& name) const
{
    return containsAnyDataDoc(DataDoc::Type::Playlist, {name});
}

bool InstallFoundation::containsAnyPlatform(const QList<QString>& names) const
{
    return containsAnyDataDoc(DataDoc::Type::Platform, names);
}

bool InstallFoundation::containsAnyPlaylist(const QList<QString>& names) const
{
    return containsAnyDataDoc(DataDoc::Type::Playlist, names);
}

void InstallFoundation::addRevertableFile(const QString& filePath) { mRevertableFilePaths.append(filePath); }
int InstallFoundation::revertQueueCount() const { return mRevertableFilePaths.size(); }

int InstallFoundation::revertNextChange(RevertError& error, bool skipOnFail)
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

}
