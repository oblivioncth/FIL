// Unit Include
#include "fe-install.h"

// Qt Includes
#include <QFileInfo>

// Qx Includes
#include <qx/windows/qx-filedetails.h>

// Windows Includes (Specifically for changing file permissions)
#include <atlstr.h>
#include "Aclapi.h"
#include "sddl.h"

namespace Fe
{

//===============================================================================================================
// Install
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
Install::Install(QString installPath) :
    mValid(false), // Path is invalid until proven otherwise
    mRootDirectory(installPath)
{}

//-Class Functions--------------------------------------------------------------------------------------------
//Private:
QMap<QString, InstallFactory*>& Install::registry() { static QMap<QString, InstallFactory*> registry; return registry; }

void Install::allowUserWriteOnFile(QString filePath)
{
    PACL pDacl,pNewDACL;
    EXPLICIT_ACCESS ExplicitAccess;
    PSECURITY_DESCRIPTOR ppSecurityDescriptor;
    PSID psid;

    CString filePathC = filePath.toStdWString().c_str();
    LPTSTR lpStr = filePathC.GetBuffer();

    GetNamedSecurityInfo(lpStr, SE_FILE_OBJECT,DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, &ppSecurityDescriptor);
    ConvertStringSidToSid(L"S-1-1-0", &psid);

    ExplicitAccess.grfAccessMode = SET_ACCESS;
    ExplicitAccess.grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.ptstrName = (LPTSTR) psid;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;

    SetEntriesInAcl(1, &ExplicitAccess, pDacl, &pNewDACL);
    SetNamedSecurityInfo(lpStr,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL);

    LocalFree(pNewDACL);
    LocalFree(psid);

    filePathC.ReleaseBuffer();
}

//Public:
void Install::registerInstall(QString name, InstallFactory* factory) { registry()[name] = factory; }

std::shared_ptr<Install> Install::acquireMatch(const QString& installPath)
{
    // Check all installs against path and return match if found
    QMap<QString, InstallFactory*>::const_iterator i;

    for(i = registry().constBegin(); i != registry().constEnd(); ++i)
    {
        InstallFactory* installFactory = i.value();
        std::shared_ptr<Install> possibleMatch = installFactory->produce(installPath);

        if(possibleMatch->isValid())
            return possibleMatch;
    }

    // Return nullptr on failure to find match
    return nullptr;
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
QSet<QString> Install::getExistingDocs(DataDoc::Type docType) const
{
    QSet<QString> nameSet;

    for (const DataDoc::Identifier& identifier : mExistingDocuments)
        if(identifier.docType() == docType)
            nameSet.insert(identifier.docName());

    return nameSet;
}

//Protected:
void Install::nullify()
{
    mValid = false;
    mRootDirectory = QDir();
    nullifyDerived();
}

Qx::GenericError Install::openDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDocReader> docReader)
{
    // Error report to return
    Qx::GenericError openReadError; // Defaults to no error
    Qx::GenericError errorTemplate(Qx::GenericError::Critical, docToOpen->errorString(DataDoc::StandardError::DocCantOpen));

    // Check if lease is already out
    if(mLeasedDocuments.contains(docToOpen->identifier()))
        openReadError = errorTemplate.setSecondaryInfo(docToOpen->errorString(DataDoc::StandardError::DocAlreadyOpen));
    else
    {
        // Read existing file if present
        if(mExistingDocuments.contains(docToOpen->identifier()))
        {
            // Open File
            if(docToOpen->mDocumentFile->open(QFile::ReadOnly))
                openReadError = docReader->readInto();
            else
                openReadError = errorTemplate.setSecondaryInfo(docToOpen->mDocumentFile->errorString());
        }

        // Add lease to ledger if no error occurred while reading
        if(!openReadError.isValid())
            mLeasedDocuments.insert(docToOpen->identifier());
    }

    // Return opened document and status
    return openReadError;
}

Qx::GenericError Install::saveDataDocument(DataDoc* docToSave, std::shared_ptr<DataDocWriter> docWriter)
{
    // Error template
    Qx::GenericError errorTemplate(Qx::GenericError::Critical, docToSave->errorString(DataDoc::StandardError::DocCantSave));

    // Create backup if required
    QFileInfo targetInfo(docToSave->filePath());

    if(targetInfo.exists() && targetInfo.isFile())
    {
        QString backupPath = targetInfo.absolutePath() + '/' + targetInfo.baseName() + MODIFIED_FILE_EXT;

        if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
        {
            if(!QFile::remove(backupPath))
                return errorTemplate.setSecondaryInfo(docToSave->errorString(DataDoc::StandardError::CantRemoveBackup));
        }

        if(!QFile::copy(targetInfo.absoluteFilePath(), backupPath))
            return errorTemplate.setSecondaryInfo(docToSave->errorString(DataDoc::StandardError::CantCreateBackup));
    }

    // Add file to modified list
    mModifiedDocuments.insert(docToSave->identifier());

    // Open and clear document file
    if(!docToSave->mDocumentFile->open(QFile::WriteOnly))
        return errorTemplate.setSecondaryInfo(docToSave->mDocumentFile->errorString());

    docToSave->clearFile();

    // Write to file
    Qx::GenericError saveWriteError = docWriter->writeOutOf();

    // Close document file
    docToSave->mDocumentFile->close();

    // Set document permissions
    allowUserWriteOnFile(docToSave->mDocumentFile->fileName());

    // Remove handle reservation
    mLeasedDocuments.remove(docToSave->identifier());

    // Return write status and let document ptr auto delete
    return saveWriteError;
}

//Public:
void Install::linkClifpPath(QString clifpPath) { mLinkedClifpPath = clifpPath; }
QString Install::linkedClifpPath() const { return mLinkedClifpPath; }

QString Install::versionString() const
{
    Qx::FileDetails exeDetails = Qx::FileDetails::readFileDetails(executablePath());

    QString fileVersionStr = exeDetails.stringTable().fileVersion;
    QString productVersionStr = exeDetails.stringTable().productVersion;

    if(!fileVersionStr.isEmpty())
        return fileVersionStr;
    else if(!productVersionStr.isEmpty())
        return productVersionStr;
    else
        return "Unknown Version";
}

bool Install::isValid() const { return mValid; }
QString Install::getPath() const { return mRootDirectory.absolutePath(); }

QSet<QString> Install::getExistingPlatforms() const { return getExistingDocs(DataDoc::Type::Platform); }
QSet<QString> Install::getExistingPlaylists() const { return getExistingDocs(DataDoc::Type::Playlist); }

Qx::GenericError Install::openPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, QString name, UpdateOptions updateOptions)
{
    // Get initialized blank doc and reader
    std::shared_ptr<PlatformDocReader> docReader = prepareOpenPlatformDoc(returnBuffer, name, updateOptions);

    // Open document
    Qx::GenericError readErrorStatus = openDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::openPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, QString name, UpdateOptions updateOptions)
{
    // Get initialized blank doc and reader
    std::shared_ptr<PlaylistDocReader> docReader = prepareOpenPlaylistDoc(returnBuffer, name, updateOptions);

    // Open document
    Qx::GenericError readErrorStatus = openDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::savePlatformDoc(std::unique_ptr<PlatformDoc> document)
{
    // Doc should belong to this install
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlatformDocWriter> docWriter = prepareSavePlatformDoc(document);

    // Write
    Qx::GenericError writeErrorStatus = saveDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Qx::GenericError Install::savePlaylistDoc(std::unique_ptr<PlaylistDoc> document)
{
    // Doc should belong to this install
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlaylistDocWriter> docWriter = prepareSavePlaylistDoc(document);

    // Write
    Qx::GenericError writeErrorStatus = saveDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

void Install::addPurgeableImagePath(QString imagePath)
{
    /*TODO: This feels ugly, but due to the fact that a game's info is pulled before a transfer
     *      may be possible (i.e. if the image needs to be downloaded), and the info is needed
     *      to determine an images destination path (and caching game info in import worker also
     *      feels cruddy), this seemed like the best way to handle tracking modified images short
     *      of moving the image transfer duty back into Fe::Install, adding image transfers to a
     *      queue within Fe::Install when the game info is pulled, and then later when ImportWorker
     *      would performed the transfers, call a function to have Fe::Install initiate them. Though,
     *      this then makes reporting progress uglier...
    */
    mPurgeableImagePaths.append(imagePath);
}

Qx::GenericError Install::referenceImage(Fp::ImageType, QString, const Game&)
{
    throw new std::exception("UNSUPPORTED");
}

Qx::GenericError Install::bulkReferenceImages(QString, QString, QStringList)
{
    throw new std::exception("UNSUPPORTED");
}

void Install::softReset()
{
    mLinkedClifpPath.clear();
    mModifiedDocuments.clear();
    mLeasedDocuments.clear();
    mPurgeableImagePaths.clear();
    softResetDerived();
}

int Install::getRevertQueueCount() const { return mModifiedDocuments.size() + mPurgeableImagePaths.size(); }

int Install::revertNextChange(Qx::GenericError& error, bool skipOnFail)
{
    // Ensure error message is null
    error = Qx::GenericError();

    // Get operation count for return
    int operationsLeft = mModifiedDocuments.size() + mPurgeableImagePaths.size();

    // Delete new XML files and restore backups if present
    if(!mModifiedDocuments.isEmpty())
    {
        QSet<DataDoc::Identifier>::iterator setFront = mModifiedDocuments.begin();
        QString currentDocPath = dataDocPath(*setFront);

        QFileInfo currentDocInfo(currentDocPath);
        QString backupPath = currentDocInfo.absolutePath() + '/' + currentDocInfo.baseName() + MODIFIED_FILE_EXT;

        if(currentDocInfo.exists() && !QFile::remove(currentDocPath) && !skipOnFail)
        {
            error = Qx::GenericError(Qx::GenericError::Error, ERR_REVERT_CANT_REMOVE_DOC, currentDocPath);
            return operationsLeft;
        }

        if(!currentDocInfo.exists() && QFile::exists(backupPath) && !QFile::rename(backupPath, currentDocPath) && !skipOnFail)
        {
            error = Qx::GenericError(Qx::GenericError::Error, ERR_REVERT_CANT_RESTORE_DOC, backupPath);
            return operationsLeft;
        }

        // Remove entry on success
        mModifiedDocuments.erase(setFront);
        return operationsLeft - 1;
    }

    // Revert regular image changes
    if(!mPurgeableImagePaths.isEmpty())
    {
        QString currentImage = mPurgeableImagePaths.front();

        if(!QFile::remove(currentImage) && !skipOnFail)
        {
            error = Qx::GenericError(Qx::GenericError::Error, ERR_REVERT_CANT_REMOVE_IMAGE, currentImage);
            return operationsLeft;
        }

        // Remove entry on success
        mPurgeableImagePaths.removeFirst();
        return operationsLeft - 1;
    }

    // Return 0 if all empty (shouldn't be reached if function is used correctly)
    return 0;
}

}
