#include "lb-install.h"
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
Install::Install(QString installPath, QString linkedClifpPath) :
    Fe::Install(installPath, linkedClifpPath)
{
    QScopeGuard validityGuard([this](){ nullify(); }); // Automatically nullify on fail

    // Initialize files and directories;
    mPlatformImagesDirectory = QDir(installPath + '/' + PLATFORM_IMAGES_PATH);
    mDataDirectory = QDir(installPath + '/' + DATA_PATH);
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
    mLBDatabaseIDTracker = Qx::FreeIndexTracker<int>(0, -1);
    mPlaylistGameDetailsCache.clear();
}

QString Install::dataDocPath(Fe::DataDoc::Identifier identifier) const
{
    switch(identifier.docType())
    {
        case Fe::DataDoc::Type::Platform:
            return mPlatformsDirectory.absoluteFilePath(identifier.docName());
            break;

        case Fe::DataDoc::Type::Playlist:
            return mPlaylistsDirectory.absoluteFilePath(identifier.docName());
            break;

        case Fe::DataDoc::Type::Config:
            return mDataDirectory.absoluteFilePath(identifier.docName());
            break;
    }
}

QString Install::imageDestinationPath(ImageType imageType, const Fe::Game& game) const
{
    return mPlatformImagesDirectory.absolutePath() + '/' +
           game.getPlatform() + '/' +
           imageType == Logo ? LOGO_PATH : SCREENSHOT_PATH + '/' +
           game.getId().toString(QUuid::WithoutBraces) +
           IMAGE_EXT;
}

std::shared_ptr<Fe::PlatformDocReader> Install::prepareOpenPlatformDoc(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& name, const Fe::UpdateOptions& updateOptions)
{
    // Create doc file reference
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(mPlatformsDirectory.absolutePath() + '/' + makeFileNameLBKosher(name) + XML_EXT);

    // Construct unopened document
    platformDoc = std::make_unique<PlatformDoc>(this, std::move(docFile), name, updateOptions, DocKey{});

    // Construct doc reader
    std::shared_ptr<Fe::PlatformDocReader> docReader = std::make_shared<PlatformDocReader>(platformDoc.get());

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlaylistDocReader> Install::prepareOpenPlaylistDoc(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& name, const Fe::UpdateOptions& updateOptions)
{
     // Create doc file reference
    std::unique_ptr<QFile> docFile = std::make_unique<QFile>(mPlaylistsDirectory.absolutePath() + '/' + makeFileNameLBKosher(name) + XML_EXT);

    // Construct unopened document
    playlistDoc = std::make_unique<PlaylistDoc>(this, std::move(docFile), name, updateOptions, DocKey{});

    // Construct doc reader
    std::shared_ptr<Fe::PlaylistDocReader> docReader = std::make_shared<PlaylistDocReader>(playlistDoc.get());

    // Return reader and doc
    return docReader;
}

std::shared_ptr<Fe::PlatformDocWriter> Install::prepareSavePlatformDoc(const std::unique_ptr<Fe::PlatformDoc>& platformDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlatformDocWriter> docWriter = std::make_shared<PlatformDocWriter>(platformDoc.get());

    // Return writer
    return docWriter;
}

std::shared_ptr<Fe::PlaylistDocWriter> Install::prepareSavePlaylistDoc(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlaylistDocWriter> docWriter = std::make_shared<PlaylistDocWriter>(playlistDoc.get());

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
    assert(document.parent() == this);

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

Qx::GenericError Install::populateExistingDocs(QStringList targetPlatforms, QStringList targetPlaylists)
{
    // Clear existing
    mExistingDocuments.clear();

    // Temp storage
    QStringList existingList;

    // Check for platforms (Likely disolve Qx::getDirFileList in favor of QFileInfoList and QDir::entryInfoList())
    Qx::IOOpReport existingCheck = Qx::getDirFileList(existingList, mPlatformsDirectory, {XML_EXT}, QDirIterator::Subdirectories);
    if(existingCheck.wasSuccessful())
        for(const QString& platformPath : qAsConst(existingList))
            for(const QString& possibleMatch : targetPlatforms)
                if(QFileInfo(platformPath).baseName() == makeFileNameLBKosher(possibleMatch))
                    mExistingDocuments.insert(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Platform, possibleMatch));

    // Check for playlists
    if(existingCheck.wasSuccessful())
        existingCheck = Qx::getDirFileList(existingList, mPlaylistsDirectory, {XML_EXT}, QDirIterator::Subdirectories);
    if(existingCheck.wasSuccessful())
        for(const QString& playlistPath : qAsConst(existingList))
            for(const QString& possibleMatch : targetPlaylists)
                if(QFileInfo(playlistPath).baseName() == makeFileNameLBKosher(possibleMatch))
                    mExistingDocuments.insert(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Playlist, possibleMatch));

    // Check for config docs
    if(existingCheck.wasSuccessful())
        existingCheck = Qx::getDirFileList(existingList, mDataDirectory, {XML_EXT});
    if(existingCheck.wasSuccessful())
        for(const QString& configDocPath : qAsConst(existingList))
            mExistingDocuments.insert(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config, QFileInfo(configDocPath).baseName()));

    return !existingCheck.wasSuccessful() ?
                Qx::GenericError(Qx::GenericError::Critical, ERR_INSEPECTION, existingCheck.getOutcome(), existingCheck.getOutcomeInfo()) :
                Qx::GenericError();
}

Qx::GenericError Install::bulkReferenceImages(QString logoRootPath, QString screenshotRootPath, QStringList platforms)
{
    // Open platforms document
    std::unique_ptr<PlatformsDoc> platformConfigXML;
    Qx::GenericError platformConfigReadError = openPlatformsDoc(platformConfigXML);

    // Stop import if error occured
    if(platformConfigReadError.isValid())
        return platformConfigReadError;

    // Set media folder paths and ensure document contains platform or else image paths will be ignored
    for(const QString& platform : platforms)
    {
        platformConfigXML->setMediaFolder(platform, LB::Install::LOGO_PATH, logoRootPath);
        platformConfigXML->setMediaFolder(platform, LB::Install::SCREENSHOT_PATH, screenshotRootPath);

        if(!platformConfigXML->containsPlatform(platform))
        {
            LB::PlatformBuilder pb;
            pb.wName(platform);
            platformConfigXML->addPlatform(pb.build());
        }
    }

    // Save platforms document
    Qx::GenericError saveError = savePlatformsDoc(std::move(platformConfigXML));

    return saveError.isValid() ? saveError : Qx::GenericError();
}

}
