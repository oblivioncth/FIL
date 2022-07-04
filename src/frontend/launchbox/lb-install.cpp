// Unit Include
#include "lb-install.h"

// Standard Library Includes
#include <filesystem>

// Qt Includes
#include <QFileInfo>
#include <QDir>
#include <qhashfunctions.h>

// Qx Includes
#include <qx/io/qx-common-io.h>
#include <qx/core/qx-json.h>
#include <qx/core/qx-versionnumber.h>
#include <qx/windows/qx-filedetails.h>

// Windows Includes (Specifically for changing XML permissions)
#include <atlstr.h>
#include "Aclapi.h"
#include "sddl.h"

namespace Lb
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath) :
    Fe::Install(installPath)
{
    QScopeGuard validityGuard([this](){ nullify(); }); // Automatically nullify on fail

    // Initialize files and directories;
    mPlatformImagesDirectory = QDir(installPath + '/' + PLATFORM_IMAGES_PATH);
    mDataDirectory = QDir(installPath + '/' + DATA_PATH);
    mCoreDirectory = QDir(installPath + '/' + CORE_PATH);
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);

    // Check validity
    QFileInfo mainExe(installPath + "/" + MAIN_EXE_PATH);
    if(!mainExe.exists() || !mainExe.isFile() || !mPlatformsDirectory.exists() || !mPlaylistsDirectory.exists())
        return;

    // Give the OK
    mValid = true;
    validityGuard.dismiss();
}

//-Class Functions------------------------------------------------------------------------------------------------
//Private:
QString Install::makeFileNameLBKosher(QString fileName)
{
    // Perform general kosherization
    fileName = Qx::kosherizeFileName(fileName);

    // LB specific changes
    fileName.replace('#','_');
    fileName.replace('\'','_');

    return fileName;
}

//-Instance Functions----------------------------------------------------------------------------------------------
//Private:
void Install::nullifyDerived()
{
    mDataDirectory = QDir();
    mPlatformsDirectory = QDir();
    mPlaylistsDirectory = QDir();
    mPlatformImagesDirectory = QDir();
}

void Install::softResetDerived()
{
    mLbDatabaseIdTracker = Qx::FreeIndexTracker<int>(0, -1);
    mPlaylistGameDetailsCache.clear();
}

QString Install::dataDocPath(Fe::DataDoc::Identifier identifier) const
{
    QString fileName = makeFileNameLBKosher(identifier.docName()) + "." + XML_EXT;

    switch(identifier.docType())
    {
        case Fe::DataDoc::Type::Platform:
            return mPlatformsDirectory.absoluteFilePath(fileName);
            break;

        case Fe::DataDoc::Type::Playlist:
            return mPlaylistsDirectory.absoluteFilePath(fileName);
            break;

        case Fe::DataDoc::Type::Config:
            return mDataDirectory.absoluteFilePath(fileName);
            break;
        default:
                throw new std::invalid_argument("Function argument was not of type Fe::DataDoc::Identifier");
    }
}

std::shared_ptr<Fe::PlatformDocReader> Install::prepareOpenPlatformDoc(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& name, const Fe::UpdateOptions& updateOptions)
{
    // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Platform, name);
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(dataDocPath(docId));

    // Construct unopened document
    platformDoc = std::make_unique<PlatformDoc>(this, std::move(docFile), name, updateOptions, DocKey{});

    // Construct doc reader (need to downcast pointer since doc pointer is upcasted after construction above)
    std::shared_ptr<Fe::PlatformDocReader> docReader = std::make_shared<PlatformDocReader>(static_cast<PlatformDoc*>(platformDoc.get()));

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlaylistDocReader> Install::prepareOpenPlaylistDoc(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& name, const Fe::UpdateOptions& updateOptions)
{
     // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Playlist, name);
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(dataDocPath(docId));

    // Construct unopened document
    playlistDoc = std::make_unique<PlaylistDoc>(this, std::move(docFile), name, updateOptions, DocKey{});

    // Construct doc reader (need to downcast pointer since doc pointer is upcasted after construction above)
    std::shared_ptr<Fe::PlaylistDocReader> docReader = std::make_shared<PlaylistDocReader>(static_cast<PlaylistDoc*>(playlistDoc.get()));

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlatformDocWriter> Install::prepareSavePlatformDoc(const std::unique_ptr<Fe::PlatformDoc>& platformDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlatformDocWriter> docWriter = std::make_shared<PlatformDocWriter>(static_cast<PlatformDoc*>(platformDoc.get()));

    // Return writer
    return docWriter;
}

std::shared_ptr<Fe::PlaylistDocWriter> Install::prepareSavePlaylistDoc(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlaylistDocWriter> docWriter = std::make_shared<PlaylistDocWriter>(static_cast<PlaylistDoc*>(playlistDoc.get()));

    // Return writer
    return docWriter;
}

Qx::GenericError Install::openPlatformsDoc(std::unique_ptr<PlatformsDoc>& returnBuffer)
{
    // Create doc file reference
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(dataDocPath(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config,
                                                                                                 PlatformsDoc::STD_NAME)));

    // Construct unopened document
    returnBuffer = std::make_unique<PlatformsDoc>(this, std::move(docFile), DocKey{});

    // Construct doc reader
    std::shared_ptr<PlatformsDocReader> docReader = std::make_shared<PlatformsDocReader>(returnBuffer.get());

    // Open document
    Qx::GenericError readErrorStatus = openDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::savePlatformsDoc(std::unique_ptr<PlatformsDoc> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlatformsDocWriter> docWriter = std::make_shared<PlatformsDocWriter>(document.get());

    // Write
    Qx::GenericError writeErrorStatus = saveDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

//Public:
QString Install::name() const { return NAME; }
QString Install::executablePath() const { return mRootDirectory.absoluteFilePath(MAIN_EXE_PATH); }
Install::ImageRefType Install::imageRefType() const { return ImageRefType::Bulk; }
bool Install::supportsImageMode(ImageMode imageMode) const { return IMAGE_MODES.contains(imageMode); }

QString Install::versionString() const
{
    // Try LaunchBox.deps.json
    QFile depsJson(mCoreDirectory.path() + "/" + "LaunchBox.deps.json");
    QByteArray settingsData;
    Qx::IoOpReport settingsLoadReport = Qx::readBytesFromFile(settingsData, depsJson);

    if(settingsLoadReport.wasSuccessful())
    {
        // Parse original JSON data
        QJsonObject settingsObj = QJsonDocument::fromJson(settingsData).object();

        if(!settingsObj.isEmpty())
        {
            // Get key that should have version
            QList<QJsonValue> res = Qx::Json::findAllValues(QJsonValue(settingsObj), "Unbroken.LaunchBox.Windows");

            if(!res.isEmpty() && res.first().isString())
            {
                // Check for valid version number
                Qx::VersionNumber ver = Qx::VersionNumber::fromString(res.first().toString());

                if(!ver.isNull())
                    return ver.toString();
            }
        }
    }

    // Try unins000.exe
    Qx::FileDetails uninsDetails = Qx::FileDetails::readFileDetails(mRootDirectory.path() + "/" + "unins000.exe");
    if(!uninsDetails.isNull())
        return uninsDetails.productVersion().toString();

    // Fallback to generic method
    return Fe::Install::versionString();
}

Qx::GenericError Install::populateExistingDocs(QStringList targetPlatforms, QStringList targetPlaylists)
{
    // Clear existing
    mExistingDocuments.clear();

    // Temp storage
    QFileInfoList existingList;

    // Check for platforms (Likely dissolve Qx::getDirFileList in favor of QFileInfoList and QDir::entryInfoList())
    Qx::IoOpReport existingCheck = Qx::dirContentInfoList(existingList, mPlatformsDirectory, {"*." + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.wasSuccessful())
        for(const QFileInfo& platformFile : qAsConst(existingList))
            for(const QString& possibleMatch : targetPlatforms)
                if(platformFile.baseName() == makeFileNameLBKosher(possibleMatch))
                    mExistingDocuments.insert(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Platform, possibleMatch));

    // Check for playlists
    if(existingCheck.wasSuccessful())
        existingCheck = Qx::dirContentInfoList(existingList, mPlaylistsDirectory, {"*." + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.wasSuccessful())
        for(const QFileInfo& playlistFile : qAsConst(existingList))
            for(const QString& possibleMatch : targetPlaylists)
                if(playlistFile.baseName() == makeFileNameLBKosher(possibleMatch))
                    mExistingDocuments.insert(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Playlist, possibleMatch));

    // Check for config docs
    if(existingCheck.wasSuccessful())
        existingCheck = Qx::dirContentInfoList(existingList, mDataDirectory, {"*." + XML_EXT});
    if(existingCheck.wasSuccessful())
        for(const QFileInfo& configDocFile : qAsConst(existingList))
            mExistingDocuments.insert(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config, configDocFile.baseName()));

    return !existingCheck.wasSuccessful() ?
                Qx::GenericError(Qx::GenericError::Critical, ERR_INSEPECTION, existingCheck.outcome(), existingCheck.outcomeInfo()) :
                Qx::GenericError();
}

QString Install::imageDestinationPath(Fp::ImageType imageType, const Fe::Game& game) const
{
    return mPlatformImagesDirectory.absolutePath() + '/' +
           game.getPlatform() + '/' +
           (imageType == Fp::ImageType::Logo ? LOGO_PATH : SCREENSHOT_PATH) + '/' +
           game.getId().toString(QUuid::WithoutBraces) +
           IMAGE_EXT;
}

Qx::GenericError Install::bulkReferenceImages(QString logoRootPath, QString screenshotRootPath, QStringList platforms)
{
    // Open platforms document
    std::unique_ptr<PlatformsDoc> platformConfigXML;
    Qx::GenericError platformConfigReadError = openPlatformsDoc(platformConfigXML);

    // Stop import if error occurred
    if(platformConfigReadError.isValid())
        return platformConfigReadError;

    // Set media folder paths and ensure document contains platform or else image paths will be ignored
    for(const QString& platform : platforms)
    {
        platformConfigXML->setMediaFolder(platform, Lb::Install::LOGO_PATH, logoRootPath);
        platformConfigXML->setMediaFolder(platform, Lb::Install::SCREENSHOT_PATH, screenshotRootPath);

        if(!platformConfigXML->containsPlatform(platform))
        {
            Lb::PlatformBuilder pb;
            pb.wName(platform);
            platformConfigXML->addPlatform(pb.build());
        }
    }

    // Save platforms document
    Qx::GenericError saveError = savePlatformsDoc(std::move(platformConfigXML));

    return saveError.isValid() ? saveError : Qx::GenericError();
}

}
