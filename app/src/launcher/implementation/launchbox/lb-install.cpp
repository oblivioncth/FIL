// Unit Include
#include "lb-install.h"

// Standard Library Includes

// Qt Includes
#include <QFileInfo>
#include <QDir>
#include <QHashFunctions>

// Qx Includes
#include <qx/io/qx-common-io.h>
#include <qx/core/qx-json.h>
#include <qx/core/qx-versionnumber.h>
#include <qx/core/qx-system.h>
#include <qx/windows/qx-filedetails.h>

// Project Includes
#include "import/details.h"

namespace Lb
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(const QString& installPath) :
    Lr::Install<LauncherId>(installPath),
    mDataDirectory(installPath + '/' + DATA_PATH),
    mPlatformsDirectory(installPath + '/' + PLATFORMS_PATH),
    mPlaylistsDirectory(installPath + '/' + PLAYLISTS_PATH),
    mPlatformImagesDirectory(installPath + '/' + PLATFORM_IMAGES_PATH),
    mPlatformIconsDirectory(installPath + '/' + PLATFORM_ICONS_PATH),
    mPlatformCategoryIconsDirectory(installPath + '/' + PLATFORM_CATEGORY_ICONS_PATH),
    mPlaylistIconsDirectory(installPath + '/' + PLAYLIST_ICONS_PATH),
    mCoreDirectory(installPath + '/' + CORE_PATH),
    mExeFile(installPath + '/' + MAIN_EXE_PATH)
{    
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
    Lr::IInstall::nullify();

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
         catalogueExistingDoc(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Platform, platformFile.baseName()));

    // Check for playlists
    existingCheck = Qx::dirContentInfoList(existingList, mPlaylistsDirectory, {u"*."_s + XML_EXT}, QDir::NoFilter, QDirIterator::Subdirectories);
    if(existingCheck.isFailure())
        return existingCheck;

    for(const QFileInfo& playlistFile : qAsConst(existingList))
        catalogueExistingDoc(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Playlist, playlistFile.baseName()));

    // Check for config docs
    existingCheck = Qx::dirContentInfoList(existingList, mDataDirectory, {u"*."_s + XML_EXT});
    if(existingCheck.isFailure())
        return existingCheck;

    for(const QFileInfo& configDocFile : qAsConst(existingList))
        catalogueExistingDoc(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Config, configDocFile.baseName()));

    // Return success
    return Qx::Error();
}

QString Install::imageDestinationPath(Fp::ImageType imageType, const Lr::Game& game) const
{
    return mPlatformImagesDirectory.absolutePath() + '/' +
           game.platform() + '/' +
           (imageType == Fp::ImageType::Logo ? LOGO_PATH : SCREENSHOT_PATH) + '/' +
           game.id().toString(QUuid::WithoutBraces) +
           '.' + IMAGE_EXT;
}

void Install::editBulkImageReferences(const Lr::ImagePaths& imageSources)
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

QString Install::dataDocPath(Lr::IDataDoc::Identifier identifier) const
{
    QString fileName = identifier.docName() + u"."_s + XML_EXT;

    switch(identifier.docType())
    {
        case Lr::IDataDoc::Type::Platform:
            return mPlatformsDirectory.absoluteFilePath(fileName);
            break;

        case Lr::IDataDoc::Type::Playlist:
            return mPlaylistsDirectory.absoluteFilePath(fileName);
            break;

        case Lr::IDataDoc::Type::Config:
            return mDataDirectory.absoluteFilePath(fileName);
            break;
        default:
                throw new std::invalid_argument("Function argument was not of type Lr::IDataDoc::Identifier");
    }
}

std::unique_ptr<PlatformDoc> Install::preparePlatformDocCheckout(const QString& translatedName)
{
    // Create doc file reference
    Lr::IDataDoc::Identifier docId(Lr::IDataDoc::Type::Platform, translatedName);

    // Construct unopened document and return
    return std::make_unique<PlatformDoc>(this, dataDocPath(docId), translatedName, Import::Details::current().updateOptions);
}

std::unique_ptr<PlaylistDoc> Install::preparePlaylistDocCheckout(const QString& translatedName)
{
     // Create doc file reference
    Lr::IDataDoc::Identifier docId(Lr::IDataDoc::Type::Playlist, translatedName);

    // Construct unopened document
    return std::make_unique<PlaylistDoc>(this, dataDocPath(docId), translatedName, Import::Details::current().updateOptions);
}

Lr::DocHandlingError Install::checkoutPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc>& returnBuffer)
{
    // Create doc file reference
    Lr::IDataDoc::Identifier docId(Lr::IDataDoc::Type::Config, PlatformsConfigDoc::STD_NAME);

    // Construct unopened document
    Import::UpdateOptions uo{.importMode = Import::UpdateMode::NewAndExisting, .removeObsolete = false};
    returnBuffer = std::make_unique<PlatformsConfigDoc>(this, dataDocPath(docId), uo);

    // Construct doc reader
    std::shared_ptr<PlatformsConfigDoc::Reader> docReader = std::make_shared<PlatformsConfigDoc::Reader>(returnBuffer.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::commitPlatformsConfigDoc(std::unique_ptr<PlatformsConfigDoc> document)
{
    Q_ASSERT(document->install() == this);

    // Prepare writer
    std::shared_ptr<PlatformsConfigDoc::Writer> docWriter = std::make_shared<PlatformsConfigDoc::Writer>(document.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Lr::DocHandlingError Install::checkoutParentsDoc(std::unique_ptr<ParentsDoc>& returnBuffer)
{
    // Create doc file reference
    Lr::IDataDoc::Identifier docId(Lr::IDataDoc::Type::Config, ParentsDoc::STD_NAME);

    // Construct unopened document
    returnBuffer = std::make_unique<ParentsDoc>(this, dataDocPath(docId));

    // Construct doc reader
    std::shared_ptr<ParentsDoc::Reader> docReader = std::make_shared<ParentsDoc::Reader>(returnBuffer.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::commitParentsDoc(std::unique_ptr<ParentsDoc> document)
{
    Q_ASSERT(document->install() == this);

    // Prepare writer
    std::shared_ptr<ParentsDoc::Writer> docWriter = std::make_shared<ParentsDoc::Writer>(document.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

//Public:
void Install::softReset()
{
    Lr::IInstall::softReset();

    mLbDatabaseIdTracker = Qx::FreeIndexTracker(0, LB_DB_ID_TRACKER_MAX);
    mPlaylistGameDetailsCache.clear();
}

QList<Import::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }
bool Install::isRunning() const { return Qx::processIsRunning(mExeFile.fileName()); }

QString Install::versionString() const
{
    Qx::FileDetails exeDetails = Qx::FileDetails::readFileDetails(mExeFile.absoluteFilePath());

    QString fileVersionStr = exeDetails.stringTable().fileVersion;
    QString productVersionStr = exeDetails.stringTable().productVersion;

    if(!fileVersionStr.isEmpty())
        return fileVersionStr;
    else if(!productVersionStr.isEmpty())
        return productVersionStr;
    else
        return Lr::IInstall::versionString();
}

QString Install::translateDocName(const QString& originalName, Lr::IDataDoc::Type type) const
{
    Q_UNUSED(type);

    /* LB has started doing something strange and annoying...
     * It appears it might be that it tries to override the filename of playlists with the
     * internal name (i.e. <Name> within the file), and then applies its own substitution
     * rules to deal with illegal characters. As such, here we try to do what LB does to avoid
     * unintended filename changes after an import (since that will lead to mismatches for
     * icons and future imports).
     *
     * The replacements needed, and the order of them, will need to be determined on a case-by-case
     * basis as they come up.
     */

    QString translatedName = originalName;

    // LB matched changes (LB might replace all illegal characters with underscores, but these are is known for sure)
    // TODO: Use Qx for this
    translatedName.replace(':','_');
    translatedName.replace('#','_');
    translatedName.replace('\'','_');

    // General kosherization
    translatedName = Qx::kosherizeFileName(translatedName);

    return translatedName;
}

Qx::Error Install::prePlatformsImport()
{
    if(Qx::Error superErr = Lr::IInstall::prePlatformsImport(); superErr.isValid())
        return superErr;

    // Open platforms document
    return checkoutPlatformsConfigDoc(mPlatformsConfig);
}

Qx::Error Install::postPlatformsImport()
{
    if(Qx::Error superErr = Lr::IInstall::postPlatformsImport(); superErr.isValid())
        return superErr;

    // Open Parents.xml
    if(Lr::DocHandlingError dhe = checkoutParentsDoc(mParents); dhe.isValid())
        return dhe;

    // Add PlatformCategories to Platforms.xml
    Lb::PlatformCategory::Builder pcb;
    pcb.wName(MAIN_PLATFORM_CATEGORY);
    pcb.wNestedName(MAIN_PLATFORM_CATEGORY);
    mPlatformsConfig->addPlatformCategory(pcb.build());
    pcb.wName(PLATFORMS_PLATFORM_CATEGORY);
    pcb.wNestedName(PLATFORMS_PLATFORM_CATEGORY_NESTED);
    mPlatformsConfig->addPlatformCategory(pcb.build());
    pcb.wName(PLAYLISTS_PLATFORM_CATEGORY);
    pcb.wNestedName(PLAYLISTS_PLATFORM_CATEGORY_NESTED);
    mPlatformsConfig->addPlatformCategory(pcb.build());

    // Add categories to Parents.xml
    if(!mParents->containsPlatformCategory(MAIN_PLATFORM_CATEGORY))
    {
        Lb::Parent::Builder pb;
        pb.wPlatformCategoryName(MAIN_PLATFORM_CATEGORY);
        mParents->addParent(pb.build());
    }

    if(!mParents->containsPlatformCategory(PLATFORMS_PLATFORM_CATEGORY, MAIN_PLATFORM_CATEGORY))
    {
        Lb::Parent::Builder pb;
        pb.wPlatformCategoryName(PLATFORMS_PLATFORM_CATEGORY);
        pb.wParentPlatformCategoryName(MAIN_PLATFORM_CATEGORY);
        mParents->addParent(pb.build());
    }

    if(!mParents->containsPlatformCategory(PLAYLISTS_PLATFORM_CATEGORY, MAIN_PLATFORM_CATEGORY))
    {
        Lb::Parent::Builder pb;
        pb.wPlatformCategoryName(PLAYLISTS_PLATFORM_CATEGORY);
        pb.wParentPlatformCategoryName(MAIN_PLATFORM_CATEGORY);
        mParents->addParent(pb.build());
    }

    // Add platforms to Platforms.xml and Parents.xml
    const QList<QString> affectedPlatforms = modifiedPlatforms();
    for(const QString& pn :affectedPlatforms)
    {
        Lb::Platform::Builder pb;
        pb.wName(pn);
        mPlatformsConfig->addPlatform(pb.build());

        if(!mParents->containsPlatform(pn, PLATFORMS_PLATFORM_CATEGORY))
        {
            Lb::Parent::Builder pb;
            pb.wParentPlatformCategoryName(PLATFORMS_PLATFORM_CATEGORY);
            pb.wPlatformName(pn);
            mParents->addParent(pb.build());
        }

        // Remove old categorization directly under top level category if present
        mParents->removePlatform(pn, MAIN_PLATFORM_CATEGORY);
    }

    return Qx::Error();
}

Qx::Error Install::preImageProcessing()
{
    if(Import::Details::current().imageMode != Import::ImageMode::Reference)
        editBulkImageReferences(Lr::ImagePaths());// Null arg will remove old references

    return Lr::IInstall::preImageProcessing();
}

Qx::Error Install::postImageProcessing()
{
    if(Qx::Error superErr = Lr::IInstall::postImageProcessing(); superErr.isValid())
            return superErr;

    // Save platforms document since it's no longer needed at this point
    mPlatformsConfig->finalize();
    Lr::DocHandlingError saveError = commitPlatformsConfigDoc(std::move(mPlatformsConfig));

    return saveError;
}

Qx::Error Install::postPlaylistsImport()
{
    // Add playlists to Parents.xml
    for(const QUuid& pId : qAsConst(mModifiedPlaylistIds))
    {
        if(!mParents->containsPlaylist(pId, PLAYLISTS_PLATFORM_CATEGORY))
        {
            Lb::Parent::Builder pb;
            pb.wParentPlatformCategoryName(PLAYLISTS_PLATFORM_CATEGORY);
            pb.wPlaylistId(pId);
            mParents->addParent(pb.build());
        }

        // Remove old categorization directly under top level category if present
        mParents->removePlaylist(pId, MAIN_PLATFORM_CATEGORY);
    }

    // Close Parents.xml
    return commitParentsDoc(std::move(mParents));
}

void Install::processBulkImageSources(const Lr::ImagePaths& bulkSources)
{
    editBulkImageReferences(bulkSources);
}

void Install::convertToDestinationImages(const Game& game, Lr::ImagePaths& images)
{
    if(!images.logoPath().isEmpty())
        images.setLogoPath(imageDestinationPath(Fp::ImageType::Logo, game));

    if(!images.screenshotPath().isEmpty())
        images.setScreenshotPath(imageDestinationPath(Fp::ImageType::Screenshot, game));
}

QString Install::platformCategoryIconPath() const { return mPlatformCategoryIconsDirectory.absoluteFilePath(u"Flashpoint.png"_s); }
std::optional<QDir> Install::platformIconsDirectory() const { return mPlatformIconsDirectory; }
std::optional<QDir> Install::playlistIconsDirectory() const { return mPlaylistIconsDirectory; }

}
