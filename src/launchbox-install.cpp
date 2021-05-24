#include "launchbox-install.h"
#include <QFileInfo>
#include <QDir>
#include <qhashfunctions.h>
#include <filesystem>

// Specifically for changing XML permissions
#include <atlstr.h>
#include "Aclapi.h"
#include "sddl.h"

namespace LB
{

//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath)
{
    // Ensure instance will be valid
    if(!pathIsValidInstall(installPath))
        assert("Cannot create a Install instance with an invalid installPath. Check first with Install::pathIsValidInstall(QString).");

    // Initialize files and directories;
    mRootDirectory = QDir(installPath);
    mPlatformImagesDirectory = QDir(installPath + '/' + PLATFORM_IMAGES_PATH);
    mDataDirectory = QDir(installPath + '/' + DATA_PATH);
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);
}

//-Class Functions------------------------------------------------------------------------------------------------
//Private:
void Install::allowUserWriteOnXML(QString xmlPath)
{
    PACL pDacl,pNewDACL;
    EXPLICIT_ACCESS ExplicitAccess;
    PSECURITY_DESCRIPTOR ppSecurityDescriptor;
    PSID psid;

    CString xmlPathC = xmlPath.toStdWString().c_str();
    LPTSTR lpStr = xmlPathC.GetBuffer();

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

    xmlPathC.ReleaseBuffer();
}

QString Install::makeFileNameLBKosher(QString fileName)
{
    // Perform general kosherization
    fileName = Qx::kosherizeFileName(fileName);

    // LB specific changes
    fileName.replace('#','_');
    fileName.replace('\'','_');

    return fileName;
}

//Public:
bool Install::pathIsValidInstall(QString installPath)
{
    QFileInfo platformsFolder(installPath + "/" + PLATFORMS_PATH);
    QFileInfo playlistsFolder(installPath + "/" + PLATFORMS_PATH);
    QFileInfo mainEXE(installPath + "/" + MAIN_EXE_PATH);

    return platformsFolder.exists() && platformsFolder.isDir() &&
           playlistsFolder.exists() && playlistsFolder.isDir() &&
           mainEXE.exists() && mainEXE.isExecutable();
}

//-Instance Functions----------------------------------------------------------------------------------------------
//Private:
QString Install::transferImage(ImageMode imageMode, QDir sourceDir, QString destinationSubPath, const LB::Game& game)
{
    // Parse to paths
    QString gameIDString = game.getID().toString(QUuid::WithoutBraces);
    QString sourcePath = sourceDir.absolutePath() + '/' + gameIDString.left(2) + '/' + gameIDString.mid(2, 2) + '/' + gameIDString + IMAGE_EXT;
    QString destinationPath = mPlatformImagesDirectory.absolutePath() + '/' + game.getPlatform() + '/' + destinationSubPath + '/' + gameIDString + IMAGE_EXT;

    // Image info
    QFileInfo destinationInfo(destinationPath);
    QFileInfo sourceInfo(sourcePath);
    bool destinationOccupied = destinationInfo.exists() && (destinationInfo.isFile() || destinationInfo.isSymLink());
    bool sourceAvailable = sourceInfo.exists();

    // Return if image is already up-to-date
    if(sourceAvailable && destinationOccupied)
    {
        if(destinationInfo.isSymLink() && imageMode == Link)
            return QString();
        else
        {
            QFile source(sourcePath);
            QFile destination(destinationPath);
            QString sourceChecksum;
            QString destinationChecksum;

            if(Qx::calculateFileChecksum(sourceChecksum, source, QCryptographicHash::Md5).wasSuccessful() &&
               Qx::calculateFileChecksum(destinationChecksum, destination, QCryptographicHash::Md5).wasSuccessful() &&
               sourceChecksum.compare(destinationChecksum, Qt::CaseInsensitive) == 0)
                return QString();
        }
    }

    // Determine backup path
    QString backupPath = destinationInfo.absolutePath() + '/' + destinationInfo.baseName() + MODIFIED_FILE_EXT;

    // Temporarily backup image if it already exists (also acts as deletion marking in case images for the title were removed in an update)
    if(destinationOccupied && sourceAvailable)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return ERR_IMAGE_WONT_BACKUP.arg(destinationPath);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer if source is available
    if(sourceAvailable)
    {
        switch(imageMode)
        {
            case Copy:
                if(!QFile::copy(sourcePath, destinationPath))
                {
                    QFile::rename(backupPath, destinationPath); // Restore Backup
                    return ERR_IMAGE_WONT_COPY.arg(sourcePath, destinationPath);
                }
                else if(QFile::exists(backupPath))
                    QFile::remove(backupPath);
                else
                    mPurgableImages.append(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
                break;

            case Link:
                std::filesystem::create_symlink(sourcePath.toStdString(), destinationPath.toStdString(), linkError);
                if(linkError)
                {
                    QFile::rename(backupPath, destinationPath); // Restore Backup
                    return ERR_IMAGE_WONT_LINK.arg(sourcePath, destinationPath);
                }
                else if(QFile::exists(backupPath))
                    QFile::remove(backupPath);
                else
                    mPurgableImages.append(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
                break;

            case Reference:
                throw std::runtime_error("transferImage() should not be called with imageMode Reference!");
                break;
        }
    }

    // Return null string on success
    return QString();
}

Qx::XmlStreamReaderError Install::openDataDocument(Xml::DataDoc* docToOpen, Xml::DataDocReader* docReader)
{
    // Error report to return
    Qx::XmlStreamReaderError openReadError; // Defaults to no error

    // Check if existing instance is already allocated and set handle to null if so
    if(mLeasedHandles.contains(docToOpen->getHandleTarget()))
        openReadError = Qx::XmlStreamReaderError(Xml::formatDataDocError(Xml::ERR_DOC_ALREADY_OPEN, docToOpen->getHandleTarget()));
    else
    {
        // Create backup if required
        QFileInfo targetInfo(docToOpen->mDocumentFile->fileName());

        if(targetInfo.exists() && targetInfo.isFile())
        {
            QString backupPath = targetInfo.absolutePath() + '/' + targetInfo.baseName() + MODIFIED_FILE_EXT;

            if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
            {
                if(!QFile::remove(backupPath))
                    return Qx::XmlStreamReaderError(Xml::formatDataDocError(Xml::ERR_BAK_WONT_DEL, docToOpen->getHandleTarget()));
            }

            if(!QFile::copy(targetInfo.absoluteFilePath(), backupPath))
                return Qx::XmlStreamReaderError(Xml::formatDataDocError(Xml::ERR_CANT_MAKE_BAK, docToOpen->getHandleTarget()));
        }

        // Add file to modified list
        mModifiedXMLDocuments.append(targetInfo.absoluteFilePath());

        // Open File
        if(docToOpen->mDocumentFile->open(QFile::ReadWrite)) // Ensures that empty file is created if the target doesn't exist
        {
            // Read existing file if present
            if(mExistingDocuments.contains(docToOpen->getHandleTarget()))
            {
                openReadError = docReader->readInto();

                // Clear file to prepare for writing
                docToOpen->clearFile();
            }

            // Add handle to lease set if no error occured while readding
            if(!openReadError.isValid())
                mLeasedHandles.insert(docToOpen->getHandleTarget());
        }
        else
            openReadError = Qx::XmlStreamReaderError(Xml::formatDataDocError(Xml::ERR_DOC_CANT_OPEN, docToOpen->getHandleTarget())
                                                     .arg(docToOpen->mDocumentFile->errorString()));
    }

    // Return new handle
    return openReadError;
}

bool Install::saveDataDocument(QString& errorMessage, Xml::DataDoc* docToSave, Xml::DataDocWriter* docWriter)
{
    // Write to file
    errorMessage = docWriter->writeOutOf();

    // Close document file
    docToSave->mDocumentFile->close();

    // Set document perfmissions
    allowUserWriteOnXML(docToSave->mDocumentFile->fileName());

    // Remove handle reservation
    mLeasedHandles.remove(docToSave->getHandleTarget());

    // Return write status and let document ptr auto delete
    return errorMessage.isNull();
}

QSet<QString> Install::getExistingDocs(QString type) const
{
    QSet<QString> nameList;

    for (const Xml::DataDocHandle& doc : mExistingDocuments)
        if(doc.docType == type)
            nameList.insert(doc.docName);

    return nameList;
}

//Public:
Qx::IOOpReport Install::populateExistingDocs(QStringList platformMatches, QStringList playlistMatches)
{
    // Clear existing
    mExistingDocuments.clear();

    // Temp storage
    QStringList existingList;

    // Check for platforms
    Qx::IOOpReport existingCheck = Qx::getDirFileList(existingList, mPlatformsDirectory, {XML_EXT}, QDirIterator::Subdirectories);
    if(existingCheck.wasSuccessful())
        for(const QString& platformPath : existingList)
            for(const QString& possibleMatch : platformMatches)
                if(QFileInfo(platformPath).baseName() == makeFileNameLBKosher(possibleMatch))
                    mExistingDocuments.insert(Xml::DataDocHandle{Xml::PlatformDoc::TYPE_NAME, possibleMatch});

    // Check for playlists
    if(existingCheck.wasSuccessful())
        existingCheck = Qx::getDirFileList(existingList, mPlaylistsDirectory, {XML_EXT}, QDirIterator::Subdirectories);
    if(existingCheck.wasSuccessful())
        for(const QString& playlistPath : existingList)
            for(const QString& possibleMatch : playlistMatches)
                if(QFileInfo(playlistPath).baseName() == makeFileNameLBKosher(possibleMatch))
                    mExistingDocuments.insert(Xml::DataDocHandle{Xml::PlaylistDoc::TYPE_NAME, possibleMatch});

    // Check for config docs
    if(existingCheck.wasSuccessful())
        existingCheck = Qx::getDirFileList(existingList, mDataDirectory, {XML_EXT});
    if(existingCheck.wasSuccessful())
        for(const QString& configDocPath : existingList)
            mExistingDocuments.insert(Xml::DataDocHandle{Xml::ConfigDoc::TYPE_NAME, QFileInfo(configDocPath).baseName()});

    return existingCheck;
}

Qx::XmlStreamReaderError Install::openPlatformDoc(std::unique_ptr<Xml::PlatformDoc>& returnBuffer, QString name, UpdateOptions updateOptions)
{
    // Create doc file reference
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(mPlatformsDirectory.absolutePath() + '/' + makeFileNameLBKosher(name) + XML_EXT);

    // Construct unopened document
    returnBuffer = std::make_unique<Xml::PlatformDoc>(std::move(docFile), name, updateOptions, Xml::PlatformDoc::Key{});

    // Construct doc reader
    Xml::PlatformDocReader docReader(returnBuffer.get());

    // Open document
    Qx::XmlStreamReaderError readErrorStatus = openDataDocument(returnBuffer.get(), &docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::XmlStreamReaderError Install::openPlaylistDoc(std::unique_ptr<Xml::PlaylistDoc>& returnBuffer, QString name, UpdateOptions updateOptions)
{
    // Create doc file reference
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(mPlaylistsDirectory.absolutePath() + '/' + makeFileNameLBKosher(name) + XML_EXT);

    // Construct unopened document
    returnBuffer = std::make_unique<Xml::PlaylistDoc>(std::move(docFile), name, updateOptions, &mLBDatabaseIDTracker, Xml::PlaylistDoc::Key{});

    // Construct doc reader
    Xml::PlaylistDocReader docReader(returnBuffer.get());

    // Open document
    Qx::XmlStreamReaderError readErrorStatus = openDataDocument(returnBuffer.get(), &docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::XmlStreamReaderError Install::openPlatformsDoc(std::unique_ptr<Xml::PlatformsDoc> &returnBuffer)
{
    // Create doc file reference
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(mDataDirectory.absolutePath() + '/' + Xml::PlatformsDoc::STD_NAME + XML_EXT);

    // Construct unopened document
    returnBuffer = std::make_unique<Xml::PlatformsDoc>(std::move(docFile), Xml::PlatformsDoc::Key{});

    // Construct doc reader
    Xml::PlatformsDocReader docReader(returnBuffer.get());

    // Open document
    Qx::XmlStreamReaderError readErrorStatus = openDataDocument(returnBuffer.get(), &docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

bool Install::savePlatformDoc(QString& errorMessage, std::unique_ptr<Xml::PlatformDoc> document)
{
    // Prepare writer
    Xml::PlatformDocWriter docWriter(document.get());

    // Write
    bool writeErrorStatus = saveDataDocument(errorMessage, document.get(), &docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

bool Install::savePlaylistDoc(QString& errorMessage, std::unique_ptr<Xml::PlaylistDoc> document)
{
    // Prepare writer
    Xml::PlaylistDocWriter docWriter(document.get());

    // Write
    bool writeErrorStatus = saveDataDocument(errorMessage, document.get(), &docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

bool Install::savePlatformsDoc(QString &errorMessage, std::unique_ptr<Xml::PlatformsDoc> document)
{
    // Prepare writer
    Xml::PlatformsDocWriter docWriter(document.get());

    // Write
    bool writeErrorStatus = saveDataDocument(errorMessage, document.get(), &docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

bool Install::ensureImageDirectories(QString& errorMessage,QString platform)
{
    // Ensure error message is null
    errorMessage = QString();

    QDir logoDir(mPlatformImagesDirectory.absolutePath() + '/' + platform + '/' + LOGO_PATH);
    QDir screenshotDir(mPlatformImagesDirectory.absolutePath() + '/' + platform + '/' + SCREENSHOT_PATH);

    if(!logoDir.mkpath(".")) // "." -> Make directory at its current path (no extra sub-folders)
    {
        errorMessage = ERR_CANT_MAKE_DIR.arg(logoDir.absolutePath());
        return false;
    }

    if(!screenshotDir.mkpath("."))
    {
        errorMessage = ERR_CANT_MAKE_DIR.arg(screenshotDir.absolutePath());
        return false;
    }

    // Directories are present
    return true;
}

bool Install::transferLogo(QString& errorMessage, ImageMode imageMode, QDir logoSourceDir, const LB::Game& game)
{
    errorMessage = transferImage(imageMode, logoSourceDir, LOGO_PATH, game);
    return errorMessage.isNull();
}

bool Install::transferScreenshot(QString& errorMessage, ImageMode imageMode, QDir screenshotSourceDir, const LB::Game& game)
{
    errorMessage = transferImage(imageMode, screenshotSourceDir, SCREENSHOT_PATH, game);
    return errorMessage.isNull();
}

int Install::revertNextChange(QString& errorMessage, bool skipOnFail)
{
    // Ensure error message is null
    errorMessage = QString();

    // Get operation count for return
    int operationsLeft = mModifiedXMLDocuments.size() + mPurgableImages.size();

    // Delete new XML files and restore backups if present
    if(!mModifiedXMLDocuments.isEmpty())
    {
        QString currentDoc = mModifiedXMLDocuments.front();

        QFileInfo currentDocInfo(currentDoc);
        QString backupPath = currentDocInfo.absolutePath() + '/' + currentDocInfo.baseName() + MODIFIED_FILE_EXT;

        if(currentDocInfo.exists() && !QFile::remove(currentDoc) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_REMOVE_XML.arg(currentDoc);
            return operationsLeft;
        }

        if(!QFile::exists(currentDoc) && QFile::exists(backupPath) && !QFile::rename(backupPath, currentDoc) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_RESTORE_XML.arg(backupPath);
            return operationsLeft;
        }

        // Remove entry on success
        mModifiedXMLDocuments.removeFirst();
        return operationsLeft - 1;
    }

    // Revert regular image changes
    if(!mPurgableImages.isEmpty())
    {
        QString currentImage = mPurgableImages.front();

        if(!QFile::remove(currentImage) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_REMOVE_IMAGE.arg(currentImage);
            return operationsLeft;
        }

        // Remove entry on success
        mPurgableImages.removeFirst();
        return operationsLeft - 1;
    }

    // Return 0 if all empty (shouldn't be reached if function is used correctly)
    return 0;
}

void Install::softReset()
{
    mModifiedXMLDocuments.clear();
    mPurgableImages.clear();
    mLeasedHandles.clear();
    mLBDatabaseIDTracker = Qx::FreeIndexTracker<int>(0, -1);
}

QString Install::getPath() const { return mRootDirectory.absolutePath(); }

int Install::getRevertQueueCount() const { return mModifiedXMLDocuments.size() + mPurgableImages.size(); }

QSet<QString> Install::getExistingPlatforms() const { return getExistingDocs(Xml::PlatformDoc::TYPE_NAME); }

QSet<QString> Install::getExistingPlaylists() const { return getExistingDocs(Xml::PlaylistDoc::TYPE_NAME); }

}
