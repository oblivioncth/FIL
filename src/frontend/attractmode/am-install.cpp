// Unit Include
#include "am-install.h"

// Qt Includes
#include <QtDebug>

// Qx Includes
#include <qx/windows/qx-filedetails.h>
#include <qx/core/qx-regularexpression.h>

namespace Am
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath) :
    Fe::Install(installPath),
    mEmulatorsDirectory(installPath + '/' + EMULATORS_PATH),
    mRomlistsDirectory(installPath + '/' + ROMLISTS_PATH),
    mMainConfigFile(installPath + '/' + MAIN_CFG_PATH),
    mFpTagDirectory(installPath + '/' + ROMLISTS_PATH + '/' + FLASHPOINT_NAME),
    mFpScraperDirectory(installPath + '/' + SCRAPER_PATH + '/' + FLASHPOINT_NAME),
    mMainExe(installPath + '/' + MAIN_EXE_PATH),
    mConsoleExe(installPath + '/' + CONSOLE_EXE_PATH),
    mFpRomlist(installPath + '/' + ROMLISTS_PATH + '/' + FLASHPOINT_NAME + '.' + TXT_EXT),
    mClifpConfigFile(installPath + '/' + EMULATORS_PATH + '/' + CLIFp::NAME + '.' + CFG_EXT)
{
    /*
     *  Directories are required because they are generated byi default and help prevent
     *  false positives. The config file is required for obvious reasons. Executables
     *  are optional since the config folder might be separated by them.
     */

    // Check validity
    QFileInfo info(mMainConfigFile);
    if(!info.exists() || !info.isFile() ||
       !mEmulatorsDirectory.exists() ||
       !mRomlistsDirectory.exists())
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

    mEmulatorsDirectory = QDir();
    mRomlistsDirectory = QDir();
    mMainConfigFile.setFileName("");
    mFpTagDirectory = QDir();
    mFpScraperDirectory = QDir();
    mMainExe.setFileName("");
    mConsoleExe.setFileName("");
    mFpRomlist.setFileName("");
    mClifpConfigFile.setFileName("");
}

Qx::GenericError Install::populateExistingDocs()
{
    // Error template
    Qx::GenericError error(Qx::GenericError::Critical, ERR_INSEPECTION);

    // Temp storage
    QFileInfoList existingList;

    // Platforms and Playlists
    if(mFpTagDirectory.exists())
    {
        /* NOTE: Qt globbing syntax is slightly weird (mainly '\' cannot be used as an escape character, and instead character to
         * be escaped must individually be placed between braces. This makes using variables as part of the expression awkward
         * so instead they must be mostly written out and care must be taken to modify them if the file names change.
         *
         * See: https://doc.qt.io/qt-6/qregularexpression.html#wildcardToRegularExpression
         */

        // Check for platforms
        Qx::IoOpReport existingCheck = Qx::dirContentInfoList(existingList, mFpTagDirectory, {"[[]Platform[]] *." + TAG_EXT});
        if(existingCheck.isFailure())
            return error.setSecondaryInfo(existingCheck.outcome()).setDetailedInfo(existingCheck.outcomeInfo());

        for(const QFileInfo& platformFile : qAsConst(existingList))
             catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Platform, platformFile.baseName()));

        // Check for playlists
        existingCheck = Qx::dirContentInfoList(existingList, mFpTagDirectory, {"[[]Playlist[]] *." + TAG_EXT},
                                               QDir::NoFilter, QDirIterator::Subdirectories);
        if(existingCheck.isFailure())
            return error.setSecondaryInfo(existingCheck.outcome()).setDetailedInfo(existingCheck.outcomeInfo());

        for(const QFileInfo& playlistFile : qAsConst(existingList))
            catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Playlist, playlistFile.baseName()));

        // Check for special "Flashpoint" platform (more like a config doc but OK for now)
        QFileInfo mainRomlistInfo(mFpRomlist);
        if(mainRomlistInfo.exists())
            catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Platform, mainRomlistInfo.baseName()));
    }

    // Check for config docs
    QFileInfo mainCfgInfo(mMainConfigFile);
    catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config, mainCfgInfo.baseName())); // Must exist

    QFileInfo clifpCfgInfo(mClifpConfigFile);
    if(clifpCfgInfo.exists())
        catalogueExistingDoc(Fe::DataDoc::Identifier(Fe::DataDoc::Type::Config, clifpCfgInfo.baseName()));

    // Return success
    return Qx::GenericError();
}

QString Install::translateDocName(const QString& originalName, Fe::DataDoc::Type type) const
{
    // Perform general kosherization
    QString translatedName = Qx::kosherizeFileName(originalName);

    // Prefix platforms/playlists
    if(type == Fe::DataDoc::Type::Platform)
        translatedName.prepend(PLATFORM_TAG_PREFIX);
    else if(type == Fe::DataDoc::Type::Playlist)
        translatedName.prepend(PLAYLIST_TAG_PREFIX);

    return translatedName;
}

QString Install::imageDestinationPath(Fp::ImageType imageType, const Fe::Game* game) const
{
    return mFpScraperDirectory.absolutePath() + '/' +
           (imageType == Fp::ImageType::Logo ? LOGO_FOLDER_NAME : SCREENSHOT_FOLDER_NAME) + '/' +
           game->id().toString(QUuid::WithoutBraces) +
           '.' + IMAGE_EXT;
}

std::shared_ptr<Fe::PlatformDocReader> Install::preparePlatformDocCheckout(std::unique_ptr<Fe::PlatformDoc>& platformDoc, const QString& translatedName)
{
    // Determine path to the taglist that corresponds with the interface
    QString taglistPath = mFpTagDirectory.absoluteFilePath(translatedName + "." + TAG_EXT) ;

    // Overviews
    QDir overviewDir(mFpScraperDirectory.absoluteFilePath(OVERVIEW_FOLDER_NAME)); // Not a file, but works

    // Construct unopened document
    platformDoc = std::make_unique<PlatformInterface>(this, taglistPath, translatedName, overviewDir, DocKey{});

    // No reading to be done for this interface (tag lists are always overwritten)
    return std::shared_ptr<Fe::PlatformDocReader>();
}

std::shared_ptr<Fe::PlaylistDocReader> Install::preparePlaylistDocCheckout(std::unique_ptr<Fe::PlaylistDoc>& playlistDoc, const QString& translatedName)
{
    // Determine path to the taglist that corresponds with the interface
    QString taglistPath = mFpTagDirectory.absoluteFilePath(translatedName + "." + TAG_EXT) ;

    // Construct unopened document
    playlistDoc = std::make_unique<PlaylistInterface>(this, taglistPath, translatedName, DocKey{});

    // No reading to be done for this interface (tag lists are always overwritten)
    return std::shared_ptr<Fe::PlaylistDocReader>();
}

std::shared_ptr<Fe::PlatformDocWriter> Install::preparePlatformDocCommit(const std::unique_ptr<Fe::PlatformDoc>& platformDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlatformDocWriter> docWriter = std::make_shared<PlatformInterfaceWriter>(static_cast<PlatformInterface*>(platformDoc.get()));

    // Return writer
    return docWriter;
}

std::shared_ptr<Fe::PlaylistDocWriter> Install::preparePlaylistDocCommit(const std::unique_ptr<Fe::PlaylistDoc>& playlistDoc)
{
    // Construct doc writer
    std::shared_ptr<Fe::PlaylistDocWriter> docWriter = std::make_shared<PlaylistInterfaceWriter>(static_cast<PlaylistInterface*>(playlistDoc.get()));

    // Return writer
    return docWriter;
}

Qx::GenericError Install::checkoutMainConfig(std::unique_ptr<CrudeMainConfig>& returnBuffer)
{
    // Construct unopened document
    returnBuffer = std::make_unique<CrudeMainConfig>(this, mMainConfigFile.fileName(), DocKey{});

    // Construct doc reader
    std::shared_ptr<CrudeMainConfigReader> docReader = std::make_shared<CrudeMainConfigReader>(returnBuffer.get());

    // Open document
    Qx::GenericError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::checkoutFlashpointRomlist(std::unique_ptr<Romlist>& returnBuffer)
{
    // Construct unopened document
    returnBuffer = std::make_unique<Romlist>(this, mFpRomlist.fileName(), FLASHPOINT_NAME, mImportDetails->updateOptions, DocKey{});

    // Construct doc reader
    std::shared_ptr<RomlistReader> docReader = std::make_shared<RomlistReader>(returnBuffer.get());

    // Open document
    Qx::GenericError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::checkoutClifpEmulatorConfig(std::unique_ptr<Emulator>& returnBuffer)
{
    // Construct unopened document
    returnBuffer = std::make_unique<Emulator>(this, mClifpConfigFile.fileName(), DocKey{});

    // Construct doc reader
    std::shared_ptr<EmulatorReader> docReader = std::make_shared<EmulatorReader>(returnBuffer.get());

    // Open document
    Qx::GenericError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::commitMainConfig(std::unique_ptr<CrudeMainConfig> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<CrudeMainConfigWriter> docWriter = std::make_shared<CrudeMainConfigWriter>(document.get());

    // Write
    Qx::GenericError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Qx::GenericError Install::commitFlashpointRomlist(std::unique_ptr<Romlist> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<RomlistWriter> docWriter = std::make_shared<RomlistWriter>(document.get());

    // Write
    Qx::GenericError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}


Qx::GenericError Install::commitClifpEmulatorConfig(std::unique_ptr<Emulator> document)
{
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<EmulatorWriter> docWriter = std::make_shared<EmulatorWriter>(document.get());

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

    if(mRomlist)
        mRomlist.reset();
    mWorkerImageJobs.clear();
}

QString Install::name() const { return NAME; }
QString Install::executablePath() const { return path() + "/" + MAIN_EXE_PATH; }
QList<Fe::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }

QString Install::versionString() const
{
    // Limits to first 3 segments for consistency since that's what AttractMode seems to use

    // Try executables if they exist
    QString exePath = mMainExe.exists() ? mMainExe.fileName() :
                      mConsoleExe.exists() ? mConsoleExe.fileName() :
                      QString();

    if(!exePath.isEmpty())
    {
        Qx::FileDetails exeDetails = Qx::FileDetails::readFileDetails(exePath);
        if(!exeDetails.isNull())
        {
            Qx::VersionNumber ver = exeDetails.productVersion();
            if(!ver.isNull())
                return ver.first(3).toString();
        }
    }

    // Try main config file
    Qx::TextStreamReader configReader(mMainConfigFile.fileName());
    if(!configReader.openFile().isFailure())
    {
        QString heading = configReader.readLine();
        if(!configReader.status().isFailure())
        {
            QRegularExpressionMatch sv = Qx::RegularExpression::SEMANTIC_VERSION.match(heading);
            if(sv.hasMatch())
            {
                Qx::VersionNumber ver = Qx::VersionNumber::fromString(sv.captured());
                if(!ver.isNull())
                    return ver.first(3).toString();
            }
        }
    }

    // Can't determine version
    return QStringLiteral("UNKNOWN VERSION");
}

Qx::GenericError Install::preImport(const ImportDetails& details)
{
    Qx::GenericError reddit;

    //-Ensure that required directories exist----------------------------------------------------------------

    // Tag dir
    if(!mFpTagDirectory.exists())
        if(!mFpTagDirectory.mkpath("."))
            return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, mFpTagDirectory).toGenericError();

    // Overview dir
    QDir overviewDir(mFpScraperDirectory.absoluteFilePath(OVERVIEW_FOLDER_NAME));
    if(!overviewDir.exists())
        if(!overviewDir.mkpath("."))
            return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, overviewDir).toGenericError();

    // Logo and screenshot dir
    if(details.imageMode == Fe::ImageMode::Copy || details.imageMode == Fe::ImageMode::Link)
    {
        QDir logoDir(mFpScraperDirectory.absoluteFilePath(LOGO_FOLDER_NAME));
        if(!logoDir.exists())
            if(!logoDir.mkpath("."))
                return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, logoDir).toGenericError();

        QDir ssDir(mFpScraperDirectory.absoluteFilePath(SCREENSHOT_FOLDER_NAME));
        if(!ssDir.exists())
            if(!ssDir.mkpath("."))
                return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, ssDir).toGenericError();
    }

    // Perform base tasks
    return Fe::Install::preImport(details);
}

Qx::GenericError Install::prePlatformsImport()
{
    // Checkout romlist
    return checkoutFlashpointRomlist(mRomlist);
}

Qx::GenericError Install::postPlatformsImport()
{
    // Finalize romlist
    mRomlist->finalize();

    // Commit romlist (this will also null out its pointer since it's moved)
    return commitFlashpointRomlist(std::move(mRomlist));
}

Qx::GenericError Install::preImageProcessing(QList<ImageMap>& workerTransfers, Fe::ImageSources bulkSources)
{
    switch(mImportDetails->imageMode)
    {
        case Fe::ImageMode::Link:
        case Fe::ImageMode::Copy:
            workerTransfers.swap(mWorkerImageJobs);
            return Qx::GenericError();
        case Fe::ImageMode::Reference:
            qWarning() << Q_FUNC_INFO << "unsupported image mode";
            return Qx::GenericError();
        default:
            qWarning() << Q_FUNC_INFO << "unhandled image mode";
            return Qx::GenericError();
    }
}

Qx::GenericError Install::postImport()
{
    //-Create/update emulator settings-----------------------------------

    // Checkout emulator config
    std::unique_ptr<Emulator> emulatorConfig;
    Qx::GenericError emulatorConfigReadError = checkoutClifpEmulatorConfig(emulatorConfig);

    // Stop import if error occurred
    if(emulatorConfigReadError.isValid())
        return emulatorConfigReadError;

    // General emulator setup
    QString workingDir = QDir::toNativeSeparators(QFileInfo(mImportDetails->clifpPath).absolutePath());
    emulatorConfig->setExecutable(CLIFp::EXE_NAME);
    emulatorConfig->setArgs(R"(play -i "[romfilename]")");
    emulatorConfig->setWorkDir(workingDir);
    emulatorConfig->setRomPath("");
    emulatorConfig->setRomExt("");
    emulatorConfig->setSystem(FLASHPOINT_NAME);

    // Ensure image directories are clear
    EmulatorArtworkEntryBuilder aeb;

    aeb.wPaths({});
    aeb.wType("flyer");
    emulatorConfig->setArtworkEntry(aeb.build());
    aeb.wType("snap");
    emulatorConfig->setArtworkEntry(aeb.build());
    aeb.wType("marquee");
    emulatorConfig->setArtworkEntry(aeb.build());
    aeb.wType("wheel");
    emulatorConfig->setArtworkEntry(aeb.build());

    // Commit emulator config
    Qx::GenericError emulatorConfigWriteError = commitClifpEmulatorConfig(std::move(emulatorConfig));

    // Stop import if error occurred
    if(emulatorConfigWriteError.isValid())
        return emulatorConfigWriteError;

    //-Ensure display entry exists-----------------------------------

    // Checkout main config
    std::unique_ptr<CrudeMainConfig> mainConfig;
    Qx::GenericError mainConfigReadError = checkoutMainConfig(mainConfig);

    // Stop import if error occurred
    if(mainConfigReadError.isValid())
        return mainConfigReadError;

    // Display romlist section
    QString fpRomlistSection = "romlist              " + FLASHPOINT_NAME;

    // Add default display entry if not present
    if(!mainConfig->containsEntryWithContent("display", fpRomlistSection))
    {
        CrudeMainConfigEntryBuilder cmceb;
        cmceb.wTypeAndName("display", FLASHPOINT_NAME);
        cmceb.wContents({
            "\tlayout               Attrac-Man",
            "\t" + fpRomlistSection,
            "\tin_cycle             no",
            "\tin_menu              yes",
            "\tfilter               All",
            "\tfilter               Favourites",
            "\t\trule                 Favourite equals 1"
        });

        mainConfig->addEntry(cmceb.build());
    }

    // Commit main config
    return commitMainConfig(std::move(mainConfig));
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
