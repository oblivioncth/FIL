// Unit Include
#include "lr-install-interface.h"

// Project Includes
#include "import/backup.h"

namespace Lr
{

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
void IInstall::declareValid(bool valid)
{
    mValid = valid;
}

void IInstall::catalogueExistingDoc(IDataDoc::Identifier existingDoc) { mExistingDocuments.insert(existingDoc); }

DocHandlingError IInstall::checkoutDataDocument(std::shared_ptr<IDataDoc::Reader> docReader)
{
    auto docToOpen = docReader->target();

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

        // Add lease to ledger if no error occurred while reading, and run any post checkout handling
        if(!openReadError.isValid())
        {
            mLeasedDocuments.insert(docToOpen->identifier());
            docToOpen->postCheckout();
        }
    }

    // Return opened document and status
    return openReadError;
}

DocHandlingError IInstall::commitDataDocument(std::shared_ptr<IDataDoc::Writer> docWriter)
{
    auto docToSave = docWriter->source();
    IDataDoc::Identifier id = docToSave->identifier();

    // Backup (redundant backups are prevented). Acts as deletion for empty docs
    QString docPath = docToSave->path();
    Import::BackupError bErr = Import::BackupManager::instance()->backupCopy(docPath);
    if(bErr.type() == Import::BackupError::FileWontDelete)
        return DocHandlingError(*docToSave, DocHandlingError::CantRemoveBackup);
    else if(bErr.type() == Import::BackupError::FileWontBackup)
        return DocHandlingError(*docToSave, DocHandlingError::CantCreateBackup);
    Q_ASSERT(!bErr.isValid()); // All relevant types should be handled here

    // Error State
    DocHandlingError commitError;

    // Handle modification
    if(!docToSave->isEmpty())
    {
        mModifiedDocuments.insert(id);
        docToSave->preCommit();
        commitError = docWriter->writeOutOf();
        ensureModifiable(docToSave->path());
    }

    // Remove handle reservation
    mLeasedDocuments.remove(docToSave->identifier());

    // Return write status and let document ptr auto delete
    return commitError;
}

void IInstall::closeDataDocument(std::unique_ptr<IDataDoc> doc)
{
    // Closes without saving changes
    if(doc)
    {
        mLeasedDocuments.remove(doc->identifier());
        doc.reset();
    }
}

QList<QString> IInstall::modifiedPlatforms() const { return modifiedDataDocs(IDataDoc::Type::Platform); }
QList<QString> IInstall::modifiedPlaylists() const { return modifiedDataDocs(IDataDoc::Type::Playlist); }

//Public:
QString IInstall::versionString() const { return u"Unknown Version"_s; }
bool IInstall::isValid() const { return mValid; }
QString IInstall::path() const { return mRootDirectory.absolutePath(); }

void IInstall::softReset()
{
    mModifiedDocuments.clear();
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
