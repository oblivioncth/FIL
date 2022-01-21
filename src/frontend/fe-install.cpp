#include "fe-install.h"

#include "qx-io.h"
#include "qx-windows.h"
#include <filesystem>
#include <QFileInfo>

// Specifically for changing file permissions
#include <atlstr.h>
#include "Aclapi.h"
#include "sddl.h"

namespace Fe
{

//===============================================================================================================
// Install
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
Install::Install(QString installPath, QString linkedClifpPath) :
    mValid(false), // Path is invalid until proven otherwise
    mLinkedClifpPath(linkedClifpPath),
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

Qx::GenericError Install::transferImage(bool symlink, ImageType imageType, QDir sourceDir, const Game& game)
{
    // Parse to paths
    QString gameIDString = game.getId().toString(QUuid::WithoutBraces);
    QString sourcePath = sourceDir.absolutePath() + '/' + gameIDString.left(2) + '/' + gameIDString.mid(2, 2) + '/' + gameIDString + IMAGE_EXT;
    QString destinationPath = imageDestinationPath(imageType, game);

    // Image info
    QFileInfo destinationInfo(destinationPath);
    QFileInfo sourceInfo(sourcePath);
    QDir destinationDir(destinationInfo.absolutePath());
    bool destinationOccupied = destinationInfo.exists() && (destinationInfo.isFile() || destinationInfo.isSymLink());
    bool sourceAvailable = sourceInfo.exists();

    // Return if image is already up-to-date
    if(sourceAvailable && destinationOccupied)
    {
        if(destinationInfo.isSymLink() && symlink)
            return Qx::GenericError();
        else
        {
            QFile source(sourcePath);
            QFile destination(destinationPath);
            QString sourceChecksum;
            QString destinationChecksum;

            if(Qx::calculateFileChecksum(sourceChecksum, source, QCryptographicHash::Md5).wasSuccessful() &&
               Qx::calculateFileChecksum(destinationChecksum, destination, QCryptographicHash::Md5).wasSuccessful() &&
               sourceChecksum.compare(destinationChecksum, Qt::CaseInsensitive) == 0)
                return Qx::GenericError();
        }
    }

    // Ensure destination path exists
    if(!destinationDir.mkpath("."))
        return Qx::GenericError(Qx::GenericError::Error, ERR_CANT_MAKE_DIR, destinationDir.absolutePath());

    // Determine backup path
    QString backupPath = destinationInfo.absolutePath() + '/' + destinationInfo.baseName() + MODIFIED_FILE_EXT;

    // Temporarily backup image if it already exists (also acts as deletion marking in case images for the title were removed in an update)
    if(destinationOccupied && sourceAvailable)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_BACKUP, destinationPath);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer if source is available
    if(sourceAvailable)
    {
        if(symlink)
        {
            if(!QFile::copy(sourcePath, destinationPath))
            {
                QFile::rename(backupPath, destinationPath); // Restore Backup
                return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_COPY.arg(sourcePath), destinationPath);
            }
            else if(QFile::exists(backupPath))
                QFile::remove(backupPath);
            else
                mPurgeableImagePaths.append(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
        }
        else
        {
            std::filesystem::create_symlink(sourcePath.toStdString(), destinationPath.toStdString(), linkError);
            if(linkError)
            {
                QFile::rename(backupPath, destinationPath); // Restore Backup
                return Qx::GenericError(Qx::GenericError::Error, ERR_IMAGE_WONT_LINK.arg(sourcePath), destinationPath);
            }
            else if(QFile::exists(backupPath))
                QFile::remove(backupPath);
            else
                mPurgeableImagePaths.append(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
        }
    }

    // Return null error on success
    return Qx::GenericError();
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
    // TODO: Docs should probably be backed-up and marked as modified on save instead of on open

    // Error report to return
    Qx::GenericError openReadError; // Defaults to no error
    Qx::GenericError errorTemplate(Qx::GenericError::Critical, docToOpen->errorString(DataDoc::StandardError::DocCantOpen));

    // Check if lease is already
    if(mLeasedDocuments.contains(docToOpen->identifier()))
        openReadError = errorTemplate.setSecondaryInfo(docToOpen->errorString(DataDoc::StandardError::DocAlreadyOpen));
    else
    {
        // Create backup if required
        QFileInfo targetInfo(docToOpen->filePath());

        if(targetInfo.exists() && targetInfo.isFile())
        {
            QString backupPath = targetInfo.absolutePath() + '/' + targetInfo.baseName() + MODIFIED_FILE_EXT;

            if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
            {
                if(!QFile::remove(backupPath))
                    return errorTemplate.setSecondaryInfo(docToOpen->errorString(DataDoc::StandardError::CantRemoveBackup));
            }

            if(!QFile::copy(targetInfo.absoluteFilePath(), backupPath))
                return errorTemplate.setSecondaryInfo(docToOpen->errorString(DataDoc::StandardError::CantCreateBackup));
        }

        // Add file to modified list
        mModifiedDocuments.insert(docToOpen->identifier());

        // Open File
        if(docToOpen->mDocumentFile->open(QFile::ReadWrite)) // Ensures that empty file is created if the target doesn't exist
        {
            // Read existing file if present
            if(mExistingDocuments.contains(docToOpen->identifier()))
            {
                openReadError = docReader->readInto();

                // Clear file to prepare for writing
                docToOpen->clearFile();
            }

            // Add lease to ledger if no error occured while readding
            if(!openReadError.isValid())
                mLeasedDocuments.insert(docToOpen->identifier());
        }
        else
            openReadError = errorTemplate.setSecondaryInfo(docToOpen->mDocumentFile->errorString());
    }

    // Return opened document and status
    return openReadError;
}

Qx::GenericError Install::saveDataDocument(DataDoc* docToSave, std::shared_ptr<DataDocWriter> docWriter)
{
    // Write to file
    Qx::GenericError saveWriteError = docWriter->writeOutOf();

    // Close document file
    docToSave->mDocumentFile->close();

    // Set document perfmissions
    allowUserWriteOnFile(docToSave->mDocumentFile->fileName());

    // Remove handle reservation
    mLeasedDocuments.remove(docToSave->identifier());

    // Return write status and let document ptr auto delete
    return saveWriteError;
}

Qx::GenericError Install::referenceImage(ImageType imageType, QDir sourceDir, const Game& game)
{
    return Qx::GenericError(Qx::GenericError::Critical, ERR_UNSUPPORTED_FEATURE, "Image Referencing");
}

//Public:
QString Install::linkedClifpPath() { return mLinkedClifpPath; }

QString Install::versionString() const
{
    Qx::FileDetails exeDetails = Qx::getFileDetails(executablePath());

    QString fileVersionStr = exeDetails.getStringTable().fileVersion;
    QString productVersionStr = exeDetails.getStringTable().productVersion;

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

Qx::GenericError Install::importImage(ImageMode imageMode, ImageType imageType, QDir sourceDir, const Game& game)
{
    if(imageMode == ImageMode::Copy || imageMode == ImageMode::Link)
        return transferImage(imageMode == Link, imageType, sourceDir, game);
    else
        return referenceImage(imageType, sourceDir, game);
}

Qx::GenericError Install::bulkReferenceImages(QString logoRootPath, QString screenshotRootPath, QStringList platforms)
{
    return Qx::GenericError(Qx::GenericError::Critical, ERR_UNSUPPORTED_FEATURE, "Image Referencing");
}

void Install::softReset()
{
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
