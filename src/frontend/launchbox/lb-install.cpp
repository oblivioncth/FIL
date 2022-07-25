// Unit Include
#include "lb-install.h"

// Standard Library Includes
#include <filesystem>

// Qt Includes
#include <QFileInfo>
#include <QDir>
#include <QHashFunctions>

// Qx Includes
#include <qx/io/qx-common-io.h>
#include <qx/core/qx-json.h>
#include <qx/core/qx-versionnumber.h>
#include <qx/windows/qx-filedetails.h>

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
    // Initialize files and directories;
    mPlatformImagesDirectory = QDir(installPath + '/' + PLATFORM_IMAGES_PATH);
    mDataDirectory = QDir(installPath + '/' + DATA_PATH);
    mCoreDirectory = QDir(installPath + '/' + CORE_PATH);
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);

    // Check validity
    QFileInfo mainExe(installPath + "/" + MAIN_EXE_PATH);
    if(!mainExe.exists() || !mainExe.isFile() || !mPlatformsDirectory.exists() || !mPlaylistsDirectory.exists())
        declareValid(false);

    // Give the OK
    declareValid(true);
}

//-Instance Functions----------------------------------------------------------------------------------------------
//Private:
void Install::nullify()
{
    Fe::Install::nullify();

    mDataDirectory = QDir();
    mPlatformsDirectory = QDir();
    mPlaylistsDirectory = QDir();
    mPlatformImagesDirectory = QDir();
}

Qx::GenericError Install::populateExistingDocs()
{
    // Error template
    Qx::GenericError error(Qx::GenericError::Critical, ERR_INSEPECTION);

    // Temp storage
    QFileInfoList existingList;

    // Check for platforms
    Qx::IoOpReport existingCheck = Qx::dirContentInfoList(existingList, mPlatformsDirectory, {"*." + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.isFailure())
        return error.setSecondaryInfo(existingCheck.outcome()).setDetailedInfo(existingCheck.outcomeInfo());

    for(const QFileInfo& platformFile : qAsConst(existingList))
         catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Platform, platformFile.baseName()));

    // Check for playlists
    existingCheck = Qx::dirContentInfoList(existingList, mPlaylistsDirectory, {"*." + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.isFailure())
        return error.setSecondaryInfo(existingCheck.outcome()).setDetailedInfo(existingCheck.outcomeInfo());

    for(const QFileInfo& playlistFile : qAsConst(existingList))
        catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Playlist, playlistFile.baseName()));

    // Check for config docs
    existingCheck = Qx::dirContentInfoList(existingList, mDataDirectory, {"*." + XML_EXT});
    if(existingCheck.isFailure())
        return error.setSecondaryInfo(existingCheck.outcome()).setDetailedInfo(existingCheck.outcomeInfo());

    for(const QFileInfo& configDocFile : qAsConst(existingList))
        catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config, configDocFile.baseName()));

    // Return success
    return Qx::GenericError();
}

QString Install::translateDocName(const QString& originalName) const
{
    // Perform general kosherization
    QString translatedName = Qx::kosherizeFileName(originalName);

    // LB specific changes
    translatedName.replace('#','_');
    translatedName.replace('\'','_');

    return translatedName;
}

QString Install::imageDestinationPath(Fp::ImageType imageType, const Fe::Game* game) const
{
    return mPlatformImagesDirectory.absolutePath() + '/' +
           game->platform() + '/' +
           (imageType == Fp::ImageType::Logo ? LOGO_PATH : SCREENSHOT_PATH) + '/' +
           game->id().toString(QUuid::WithoutBraces) +
           IMAGE_EXT;
}

Qx::GenericError Install::editBulkImageReferences(const Fe::ImageSources& imageSources)
{
    // Open platforms document
    std::unique_ptr<PlatformsConfigDoc> platformsConfig;
    Qx::GenericError platformsConfigReadError = checkoutPlatformsConfigDoc(platformsConfig);

    // Stop import if error occurred
    if(platformsConfigReadError.isValid())
        return platformsConfigReadError;

    // Set media folder paths
    for(const QString& platform : qAsConst(mImportDetails->involvedPlatforms))
    {
        if(!imageSources.isNull())
        {
            // Setting the folders also makes sure that the platform is added
            platformsConfig->setMediaFolder(platform, Lb::Install::LOGO_PATH, imageSources.logoPath());
            platformsConfig->setMediaFolder(platform, Lb::Install::SCREENSHOT_PATH, imageSources.screenshotPath());
        }
        else
            platformsConfig->removePlatform(platform);
    }

    // Save platforms document
    Qx::GenericError saveError = commitPlatformsConfigDoc(std::move(platformsConfig));

    return saveError;
}

QString Install::dataDocPath(Fe::DataDoc::Identifier identifier) const
{
    QString fileName = identifier.docName() + "." + XML_EXT;

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

std::shared_ptr<Fe::PlatformDocReader> Install::preparePlatformDocCheckout(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& translatedName)
{
    // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Platform, translatedName);

    // Construct unopened document
    platformDoc = std::make_unique<PlatformDoc>(this, dataDocPath(docId), translatedName, mImportDetails->updateOptions, DocKey{});

    // Construct doc reader (need to downcast pointer since doc pointer is upcasted after construction above)
    std::shared_ptr<Fe::PlatformDocReader> docReader = std::make_shared<PlatformDocReader>(static_cast<PlatformDoc*>(platformDoc.get()));

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlaylistDocReader> Install::preparePlaylistDocCheckout(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& translatedName)
{
     // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Playlist, translatedName);

    // Construct unopened document
    playlistDoc = std::make_unique<PlaylistDoc>(this, dataDocPath(docId), translatedName, mImportDetails->updateOptions, DocKey{});

    // Construct doc reader (need to downcast pointer since doc pointer is upcasted after construction above)
    std::shared_ptr<Fe::PlaylistDocReader> docReader = std::make_shared<PlaylistDocReader>(static_cast<PlaylistDoc*>(playlistDoc.get()));

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlatformDocWriter> Install::preparePlatformDocCommit(const std::unique_ptr<Fe::PlatformDoc>& platformDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlatformDocWriter> docWriter = std::make_shared<PlatformDocWriter>(static_cast<PlatformDoc*>(platformDoc.get()));

    // Return writer
    return docWriter;
}

std::shared_ptr<Fe::PlaylistDocWriter> Install::preparePlaylistDocCommit(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlaylistDocWriter> docWriter = std::make_shared<PlaylistDocWriter>(static_cast<PlaylistDoc*>(playlistDoc.get()));

    // Return writer
    return docWriter;
}

Qx::GenericError Install::checkoutPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc>& returnBuffer)
{
    // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Config, PlatformsConfigDoc::STD_NAME);

    // Construct unopened document
    returnBuffer = std::make_unique<PlatformsConfigDoc>(this, dataDocPath(docId), DocKey{});

    // Construct doc reader
    std::shared_ptr<PlatformsConfigDocReader> docReader = std::make_shared<PlatformsConfigDocReader>(returnBuffer.get());

    // Open document
    Qx::GenericError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::commitPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlatformsConfigDocWriter> docWriter = std::make_shared<PlatformsConfigDocWriter>(document.get());

    // Write
    Qx::GenericError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

//Public:
void Install::softReset()
{
    Fe::Install::softReset();

    mLbDatabaseIdTracker = Qx::FreeIndexTracker<int>(0, -1);
    mPlaylistGameDetailsCache.clear();
    mWorkerImageJobs.clear();
}

QString Install::name() const { return NAME; }
QString Install::executablePath() const { return path() + "/" + MAIN_EXE_PATH; }
QList<Fe::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }

QString Install::versionString() const
{
    // Try LaunchBox.deps.json
    QFile depsJson(mCoreDirectory.path() + "/" + "LaunchBox.deps.json");
    QByteArray settingsData;
    Qx::IoOpReport settingsLoadReport = Qx::readBytesFromFile(settingsData, depsJson);

    if(!settingsLoadReport.isFailure())
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
    Qx::FileDetails uninsDetails = Qx::FileDetails::readFileDetails(path() + "/" + "unins000.exe");
    if(!uninsDetails.isNull())
        return uninsDetails.productVersion().toString();

    // Fallback to generic method
    return Fe::Install::versionString();
}

Qx::GenericError Install::preImageProcessing(QList<ImageMap>& workerTransfers, Fe::ImageSources bulkSources)
{
    switch(mImportDetails->imageMode)
    {
        case Fe::ImageMode::Link:
        case Fe::ImageMode::Copy:
            workerTransfers.swap(mWorkerImageJobs);
            return editBulkImageReferences(bulkSources);
        case Fe::ImageMode::Reference:
            return editBulkImageReferences(bulkSources);
        default:
            return Qx::GenericError();
            qWarning("Lb::Install::preImageProcessing() unhandled image mode");
    }
}

void Install::processDirectGameImages(const Fe::Game* game, const Fe::ImageSources& imageSources)
{
    Fe::ImageMode mode = mImportDetails->imageMode;
    if((mode == Fe::ImageMode::Link || mode == Fe::ImageMode::Copy) && !imageSources.isNull())
    {
        ImageMap logoMap{.sourcePath = imageSources.logoPath(),
                         .destPath = imageDestinationPath(Fp::ImageType::Logo, game)};

        ImageMap ssMap{.sourcePath = imageSources.screenshotPath(),
                       .destPath = imageDestinationPath(Fp::ImageType::Screenshot, game)};

        mWorkerImageJobs.append(logoMap);
        mWorkerImageJobs.append(ssMap);
    }
}

}
