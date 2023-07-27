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
Install::Install(const QString& installPath) :
    Fe::Install(installPath)
{
    // Initialize files and directories;
    mPlatformImagesDirectory = QDir(installPath + '/' + PLATFORM_IMAGES_PATH);
    mPlatformIconsDirectory = QDir(installPath + '/' + PLATFORM_ICONS_PATH);
    mPlaylistIconsDirectory = QDir(installPath + '/' + PLAYLIST_ICONS_PATH);
    mPlatformCategoryIconsDirectory = QDir(installPath + '/' + PLATFORM_CATEGORY_ICONS_PATH);
    mDataDirectory = QDir(installPath + '/' + DATA_PATH);
    mCoreDirectory = QDir(installPath + '/' + CORE_PATH);
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);

    // Check validity
    QFileInfo mainExe(installPath + '/' + MAIN_EXE_PATH);
    if(!mainExe.exists() || !mainExe.isFile() || !mPlatformsDirectory.exists() || !mPlaylistsDirectory.exists())
    {
        declareValid(false);
        return;
    }

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
    mPlatformIconsDirectory = QDir();
    mPlaylistIconsDirectory = QDir();
    mPlatformCategoryIconsDirectory = QDir();
}

Qx::Error Install::populateExistingDocs()
{
    // Temp storage
    QFileInfoList existingList;

    // Check for platforms
    Qx::IoOpReport existingCheck = Qx::dirContentInfoList(existingList, mPlatformsDirectory, {u"*."_s + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.isFailure())
        return existingCheck;

    for(const QFileInfo& platformFile : qAsConst(existingList))
         catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Platform, platformFile.baseName()));

    // Check for playlists
    existingCheck = Qx::dirContentInfoList(existingList, mPlaylistsDirectory, {u"*."_s + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.isFailure())
        return existingCheck;

    for(const QFileInfo& playlistFile : qAsConst(existingList))
        catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Playlist, playlistFile.baseName()));

    // Check for config docs
    existingCheck = Qx::dirContentInfoList(existingList, mDataDirectory, {u"*."_s + XML_EXT});
    if(existingCheck.isFailure())
        return existingCheck;

    for(const QFileInfo& configDocFile : qAsConst(existingList))
        catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config, configDocFile.baseName()));

    // Return success
    return Qx::Error();
}

QString Install::translateDocName(const QString& originalName, Fe::DataDoc::Type type) const
{
    Q_UNUSED(type);

    // Perform general kosherization
    QString translatedName = Qx::kosherizeFileName(originalName);

    // LB specific changes
    translatedName.replace('#','_');
    translatedName.replace('\'','_');

    return translatedName;
}

QString Install::executableSubPath() const { return MAIN_EXE_PATH; }

QString Install::imageDestinationPath(Fp::ImageType imageType, const Fe::Game* game) const
{
    return mPlatformImagesDirectory.absolutePath() + '/' +
           game->platform() + '/' +
           (imageType == Fp::ImageType::Logo ? LOGO_PATH : SCREENSHOT_PATH) + '/' +
           game->id().toString(QUuid::WithoutBraces) +
           '.' + IMAGE_EXT;
}

void Install::editBulkImageReferences(const Fe::ImageSources& imageSources)
{
    // Set media folder paths
    const QList<QString> affectedPlatforms = modifiedPlatforms();
    for(const QString& platform : affectedPlatforms)
    {
        if(!imageSources.isNull())
        {
            Lb::PlatformFolder::Builder pfbLogos;
            pfbLogos.wPlatform(platform);
            pfbLogos.wMediaType(Lb::Install::LOGO_PATH);
            pfbLogos.wFolderPath(imageSources.logoPath());

            Lb::PlatformFolder::Builder pfbScreenshots;
            pfbScreenshots.wPlatform(platform);
            pfbScreenshots.wMediaType(Lb::Install::SCREENSHOT_PATH);
            pfbScreenshots.wFolderPath(imageSources.screenshotPath());

            mPlatformsConfig->addPlatformFolder(pfbLogos.build());
            mPlatformsConfig->addPlatformFolder(pfbScreenshots.build());
        }
        else
            mPlatformsConfig->removePlatformFolders(platform);
    }
}

QString Install::dataDocPath(Fe::DataDoc::Identifier identifier) const
{
    QString fileName = identifier.docName() + u"."_s + XML_EXT;

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

std::shared_ptr<Fe::PlatformDoc::Reader> Install::preparePlatformDocCheckout(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& translatedName)
{
    // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Platform, translatedName);

    // Construct unopened document
    platformDoc = std::make_unique<PlatformDoc>(this, dataDocPath(docId), translatedName, mImportDetails->updateOptions, DocKey{});

    // Construct doc reader (need to downcast pointer since doc pointer is upcasted after construction above)
    std::shared_ptr<Fe::PlatformDoc::Reader> docReader = std::make_shared<PlatformDoc::Reader>(static_cast<PlatformDoc*>(platformDoc.get()));

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlaylistDoc::Reader> Install::preparePlaylistDocCheckout(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& translatedName)
{
     // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Playlist, translatedName);

    // Construct unopened document
    playlistDoc = std::make_unique<PlaylistDoc>(this, dataDocPath(docId), translatedName, mImportDetails->updateOptions, DocKey{});

    // Construct doc reader (need to downcast pointer since doc pointer is upcasted after construction above)
    std::shared_ptr<Fe::PlaylistDoc::Reader> docReader = std::make_shared<PlaylistDoc::Reader>(static_cast<PlaylistDoc*>(playlistDoc.get()));

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlatformDoc::Writer> Install::preparePlatformDocCommit(const std::unique_ptr<Fe::PlatformDoc>& platformDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlatformDoc::Writer> docWriter = std::make_shared<PlatformDoc::Writer>(static_cast<PlatformDoc*>(platformDoc.get()));

    // Return writer
    return docWriter;
}

std::shared_ptr<Fe::PlaylistDoc::Writer> Install::preparePlaylistDocCommit(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc)
{
    // Work with native type
    auto lbPlaylistDoc = static_cast<PlaylistDoc*>(playlistDoc.get());

    // Store playlist ID
    mModifiedPlaylistIds[lbPlaylistDoc->playlistHeader()->name()] = lbPlaylistDoc->playlistHeader()->id();

    // Construct doc writer
    std::shared_ptr<Fe::PlaylistDoc::Writer> docWriter = std::make_shared<PlaylistDoc::Writer>(lbPlaylistDoc);

    // Return writer
    return docWriter;
}

Fe::DocHandlingError Install::checkoutPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc>& returnBuffer)
{
    // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Config, PlatformsConfigDoc::STD_NAME);

    // Construct unopened document
    Fe::UpdateOptions uo{.importMode = Fe::ImportMode::NewAndExisting, .removeObsolete = false};
    returnBuffer = std::make_unique<PlatformsConfigDoc>(this, dataDocPath(docId), uo, DocKey{});

    // Construct doc reader
    std::shared_ptr<PlatformsConfigDoc::Reader> docReader = std::make_shared<PlatformsConfigDoc::Reader>(returnBuffer.get());

    // Open document
    Fe::DocHandlingError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Fe::DocHandlingError Install::commitPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlatformsConfigDoc::Writer> docWriter = std::make_shared<PlatformsConfigDoc::Writer>(document.get());

    // Write
    Fe::DocHandlingError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Fe::DocHandlingError Install::checkoutParentsDoc(std::unique_ptr<ParentsDoc>& returnBuffer)
{
    // Create doc file reference
    Fe::DataDoc::Identifier docId(Fe::DataDoc::Type::Config, ParentsDoc::STD_NAME);

    // Construct unopened document
    returnBuffer = std::make_unique<ParentsDoc>(this, dataDocPath(docId), DocKey{});

    // Construct doc reader
    std::shared_ptr<ParentsDoc::Reader> docReader = std::make_shared<ParentsDoc::Reader>(returnBuffer.get());

    // Open document
    Fe::DocHandlingError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Fe::DocHandlingError Install::commitParentsDoc(std::unique_ptr<ParentsDoc> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<ParentsDoc::Writer> docWriter = std::make_shared<ParentsDoc::Writer>(document.get());

    // Write
    Fe::DocHandlingError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

//Public:
void Install::softReset()
{
    Fe::Install::softReset();

    mLbDatabaseIdTracker = Qx::FreeIndexTracker(0, -1);
    mPlaylistGameDetailsCache.clear();
    mWorkerImageJobs.clear();
}

QString Install::name() const { return NAME; }
QList<Fe::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }

Qx::Error Install::prePlatformsImport()
{
    if(Qx::Error superErr = Fe::Install::prePlatformsImport(); superErr.isValid())
        return superErr;

    // Open platforms document
    return checkoutPlatformsConfigDoc(mPlatformsConfig);
}

Qx::Error Install::postPlatformsImport()
{
    if(Qx::Error superErr = Fe::Install::postPlatformsImport(); superErr.isValid())
        return superErr;

    // Open Parents.xml
    if(Fe::DocHandlingError dhe = checkoutParentsDoc(mParents); dhe.isValid())
        return dhe;

    // Add PlatformCategory to Platforms.xml
    Lb::PlatformCategory::Builder pcb;
    pcb.wName(PLATFORM_CATEGORY);
    pcb.wNestedName(PLATFORM_CATEGORY);
    mPlatformsConfig->addPlatformCategory(pcb.build());

    // Add ParentCategory to Parents.xml
    if(!mParents->containsPlatformCategory(PLATFORM_CATEGORY))
    {
        Lb::Parent::Builder pb;
        pb.wPlatformCategoryName(PLATFORM_CATEGORY);
        mParents->addParent(pb.build());
    }

    // Add platforms to Platforms.xml and Parents.xml
    const QList<QString> affectedPlatforms = modifiedPlatforms();
    for(const QString& pn :affectedPlatforms)
    {
        Lb::Platform::Builder pb;
        pb.wName(pn);
        mPlatformsConfig->addPlatform(pb.build());

        if(!mParents->containsPlatformUnderCategory(pn, PLATFORM_CATEGORY))
        {
            Lb::Parent::Builder pb;
            pb.wParentPlatformCategoryName(PLATFORM_CATEGORY);
            pb.wPlatformName(pn);
            mParents->addParent(pb.build());
        }
    }

    return Qx::Error();
}

Qx::Error Install::preImageProcessing(QList<ImageMap>& workerTransfers, const Fe::ImageSources& bulkSources)
{
    if(Qx::Error superErr = Fe::Install::preImageProcessing(workerTransfers, bulkSources); superErr.isValid())
        return superErr;

    switch(mImportDetails->imageMode)
    {
        case Fe::ImageMode::Link:
        case Fe::ImageMode::Copy:
            workerTransfers.swap(mWorkerImageJobs);
            editBulkImageReferences(bulkSources);
            break;
        case Fe::ImageMode::Reference:
            editBulkImageReferences(bulkSources);
            break;
        default:
            qWarning() << Q_FUNC_INFO << u"unhandled image mode"_s;
    }

    return Qx::Error();
}

Qx::Error Install::postImageProcessing()
{
    if(Qx::Error superErr = Fe::Install::postImageProcessing(); superErr.isValid())
            return superErr;

    // Save platforms document since it's no longer needed at this point
    mPlatformsConfig->finalize();
    Fe::DocHandlingError saveError = commitPlatformsConfigDoc(std::move(mPlatformsConfig));

    return saveError;
}

Qx::Error Install::postPlaylistsImport()
{
    // Add playlists to Parents.xml
    const QList<QString> affectedPlaylists = modifiedPlaylists();
    for(const QString& pn :affectedPlaylists)
    {
        QUuid playlistId = mModifiedPlaylistIds.value(pn);
        if(!mParents->containsPlaylistUnderCategory(playlistId, PLATFORM_CATEGORY))
        {
            Lb::Parent::Builder pb;
            pb.wParentPlatformCategoryName(PLATFORM_CATEGORY);
            pb.wPlaylistId(playlistId);
            mParents->addParent(pb.build());
        }
    }

    // Close Parents.xml
    return commitParentsDoc(std::move(mParents));
}

void Install::processDirectGameImages(const Fe::Game* game, const Fe::ImageSources& imageSources)
{
    Fe::ImageMode mode = mImportDetails->imageMode;
    if(mode == Fe::ImageMode::Link || mode == Fe::ImageMode::Copy)
    {
        if(!imageSources.logoPath().isEmpty())
        {
            ImageMap logoMap{.sourcePath = imageSources.logoPath(),
                             .destPath = imageDestinationPath(Fp::ImageType::Logo, game)};
            mWorkerImageJobs.append(logoMap);
        }

        if(!imageSources.screenshotPath().isEmpty())
        {
            ImageMap ssMap{.sourcePath = imageSources.screenshotPath(),
                           .destPath = imageDestinationPath(Fp::ImageType::Screenshot, game)};
            mWorkerImageJobs.append(ssMap);
        }
    }
}

QString Install::platformCategoryIconPath() const { return mPlatformCategoryIconsDirectory.absoluteFilePath(u"Flashpoint.png"_s); }
std::optional<QDir> Install::platformIconsDirectory() const { return mPlatformIconsDirectory; };
std::optional<QDir> Install::playlistIconsDirectory() const { return mPlaylistIconsDirectory; };

}
