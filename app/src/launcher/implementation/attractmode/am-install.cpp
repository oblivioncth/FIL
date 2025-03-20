// Unit Include
#include "am-install.h"

// Qt Includes
#include <QtDebug>

// Qx Includes
#include <qx/core/qx-regularexpression.h>
#include <qx/core/qx-system.h>
#include <qx/core/qx-string.h>

// Project Includes
#include "kernel/clifp.h"
#include "import/details.h"

namespace Am
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(const QString& installPath) :
    Lr::Install<LauncherId>(installPath),
    mEmulatorsDirectory(installPath + '/' + EMULATORS_PATH),
    mRomlistsDirectory(installPath + '/' + ROMLISTS_PATH),
    mMainConfigFile(installPath + '/' + MAIN_CFG_PATH),
    mFpTagDirectory(installPath + '/' + ROMLISTS_PATH + '/' + Fp::NAME),
    mFpScraperDirectory(installPath + '/' + SCRAPER_PATH + '/' + Fp::NAME),
    mMainExe(installPath + '/' + MAIN_EXE_PATH),
    mConsoleExe(installPath + '/' + CONSOLE_EXE_PATH),
    mFpRomlist(installPath + '/' + ROMLISTS_PATH + '/' + Fp::NAME + '.' + TXT_EXT),
    mEmulatorConfigFile(installPath + '/' + EMULATORS_PATH + '/' + Fp::NAME + '.' + CFG_EXT)
{
    /*
     *  Directories are required because they are generated by default and help prevent
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
Qx::Error Install::populateExistingDocs(QSet<Lr::IDataDoc::Identifier>& existingDocs)
{
    // Temp storage
    QFileInfoList existingList;

    // Platforms and Playlists
    if(mFpTagDirectory.exists())
    {
        /* NOTE: Qt globbing syntax is slightly weird (mainly '\' cannot be used as an escape character, and instead characters to
         * be escaped must individually be placed between braces. This makes using variables as part of the expression awkward
         * so instead they must be mostly written out and care must be taken to modify them if the file names change.
         *
         * See: https://doc.qt.io/qt-6/qregularexpression.html#wildcardToRegularExpression
         */

        // Check for platforms
        Qx::IoOpReport existingCheck = Qx::dirContentInfoList(existingList, mFpTagDirectory, {u"[[]Platform[]] *."_s + TAG_EXT});
        if(existingCheck.isFailure())
            return existingCheck;

        for(const QFileInfo& platformFile : std::as_const(existingList))
             existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Platform, platformFile.baseName()));

        // Check for playlists
        existingCheck = Qx::dirContentInfoList(existingList, mFpTagDirectory, {u"[[]Playlist[]] *."_s + TAG_EXT},
                                               QDir::NoFilter, QDirIterator::Subdirectories);
        if(existingCheck.isFailure())
            return existingCheck;

        for(const QFileInfo& playlistFile : std::as_const(existingList))
            existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Playlist, playlistFile.baseName()));

        // Check for special "Flashpoint" platform (more like a config doc but OK for now)
        QFileInfo mainRomlistInfo(mFpRomlist);
        if(mainRomlistInfo.exists())
            existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Platform, mainRomlistInfo.baseName()));
    }

    // Check for config docs
    QFileInfo mainCfgInfo(mMainConfigFile);
    existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Config, mainCfgInfo.baseName())); // Must exist

    QFileInfo emulatorCfgInfo(mEmulatorConfigFile);
    if(emulatorCfgInfo.exists())
        existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Config, emulatorCfgInfo.baseName()));

    // Return success
    return Qx::Error();
}

std::unique_ptr<PlatformInterface> Install::preparePlatformDocCheckout(const QString& translatedName)
{
    // Determine path to the taglist that corresponds with the interface
    QString taglistPath = mFpTagDirectory.absoluteFilePath(translatedName + u"."_s + TAG_EXT) ;

    // Overviews
    QDir overviewDir(mFpScraperDirectory.absoluteFilePath(OVERVIEW_FOLDER_NAME)); // Not a file, but works

    // Construct unopened document
    return std::make_unique<PlatformInterface>(this, taglistPath, translatedName, overviewDir);
}

std::unique_ptr<PlaylistInterface> Install::preparePlaylistDocCheckout(const QString& translatedName)
{
    // Determine path to the taglist that corresponds with the interface
    QString taglistPath = mFpTagDirectory.absoluteFilePath(translatedName + u"."_s + TAG_EXT) ;

    // Construct unopened document
    return std::make_unique<PlaylistInterface>(this, taglistPath, translatedName);
}

Lr::DocHandlingError Install::checkoutMainConfig(std::unique_ptr<CrudeSettings>& returnBuffer)
{
    // Construct unopened document
    returnBuffer = std::make_unique<CrudeSettings>(this, mMainConfigFile.fileName());

    // Construct doc reader
    std::shared_ptr<CrudeSettingsReader> docReader = std::make_shared<CrudeSettingsReader>(returnBuffer.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::checkoutFlashpointRomlist(std::unique_ptr<Romlist>& returnBuffer)
{
    // Construct unopened document
    returnBuffer = std::make_unique<Romlist>(this, mFpRomlist.fileName(), Fp::NAME, Import::Details::current().updateOptions);

    // Construct doc reader
    std::shared_ptr<Romlist::Reader> docReader = std::make_shared<Romlist::Reader>(returnBuffer.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::checkoutClifpEmulatorConfig(std::unique_ptr<Emulator>& returnBuffer)
{
    // Construct unopened document
    returnBuffer = std::make_unique<Emulator>(this, mEmulatorConfigFile.fileName());

    // Construct doc reader
    std::shared_ptr<EmulatorReader> docReader = std::make_shared<EmulatorReader>(returnBuffer.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::commitMainConfig(std::unique_ptr<CrudeSettings> document)
{
    Q_ASSERT(document->install() == this);

    // Prepare writer
    std::shared_ptr<CrudeSettingsWriter> docWriter = std::make_shared<CrudeSettingsWriter>(document.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Lr::DocHandlingError Install::commitFlashpointRomlist(std::unique_ptr<Romlist> document)
{
    Q_ASSERT(document->install() == this);

    // Prepare writer
    std::shared_ptr<Romlist::Writer> docWriter = std::make_shared<Romlist::Writer>(document.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}


Lr::DocHandlingError Install::commitClifpEmulatorConfig(std::unique_ptr<Emulator> document)
{
    Q_ASSERT(document->install() == this);

    // Prepare writer
    std::shared_ptr<Emulator::Writer> docWriter = std::make_shared<Emulator::Writer>(document.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

//Public:
void Install::softReset()
{
    Lr::IInstall::softReset();
    closeDataDocument(std::move(mRomlist));
}

QList<Import::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }

bool Install::isRunning() const
{
    return Qx::processIsRunning(mMainExe.fileName()) || Qx::processIsRunning(mConsoleExe.fileName());
}

QString Install::versionString() const
{
    // Limits to first 3 segments for consistency since that's what AttractMode seems to use

    // Try executables if they exist
    if(QString exeVer = versionFromExecutable(); !exeVer.isEmpty())
        return exeVer;

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
    return Lr::IInstall::versionString();
}

QString Install::translateDocName(const QString& originalName, Lr::IDataDoc::Type type) const
{
    // Perform general kosherization
    QString translatedName = Qx::kosherizeFileName(originalName);

    // Prefix platforms/playlists
    if(type == Lr::IDataDoc::Type::Platform)
        translatedName.prepend(PLATFORM_TAG_PREFIX);
    else if(type == Lr::IDataDoc::Type::Playlist)
        translatedName.prepend(PLAYLIST_TAG_PREFIX);

    return translatedName;
}

Qx::Error Install::preImport()
{
    //-Ensure that required directories exist----------------------------------------------------------------

    // Tag dir
    if(!mFpTagDirectory.exists())
        if(!mFpTagDirectory.mkpath(u"."_s))
            return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, mFpTagDirectory);

    // Overview dir
    QDir overviewDir(mFpScraperDirectory.absoluteFilePath(OVERVIEW_FOLDER_NAME));
    if(!overviewDir.exists())
        if(!overviewDir.mkpath(u"."_s))
            return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, overviewDir);

    // Logo and screenshot dir
    auto details = Import::Details::current();
    if(details.imageMode == Import::ImageMode::Copy || details.imageMode == Import::ImageMode::Link)
    {
        QDir logoDir(mFpScraperDirectory.absoluteFilePath(LOGO_FOLDER_NAME));
        if(!logoDir.exists())
            if(!logoDir.mkpath(u"."_s))
                return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, logoDir);

        QDir ssDir(mFpScraperDirectory.absoluteFilePath(SCREENSHOT_FOLDER_NAME));
        if(!ssDir.exists())
            if(!ssDir.mkpath(u"."_s))
                return Qx::IoOpReport(Qx::IO_OP_WRITE, Qx::IO_ERR_CANT_CREATE, ssDir);
    }

    // Perform base tasks
    return Lr::IInstall::preImport();
}

Qx::Error Install::prePlatformsImport()
{
    // Checkout romlist
    return checkoutFlashpointRomlist(mRomlist);
}

Qx::Error Install::postPlatformsImport()
{
    // Commit romlist (this will also null out its pointer since it's moved)
    return commitFlashpointRomlist(std::move(mRomlist));
}

Qx::Error Install::postImport()
{
    //-Create/update emulator settings-----------------------------------

    // Checkout emulator config
    std::unique_ptr<Emulator> emulatorConfig;
    Lr::DocHandlingError emulatorConfigReadError = checkoutClifpEmulatorConfig(emulatorConfig);

    // Stop import if error occurred
    if(emulatorConfigReadError.isValid())
        return emulatorConfigReadError;

    // General emulator setup
    QString workingDir = QDir::toNativeSeparators(QFileInfo(Import::Details::current().clifpPath).absolutePath());
    emulatorConfig->setExecutable(CLIFp::EXE_NAME);
    emulatorConfig->setArgs(uR"(play -i u"[romfilename]"_s)"_s);
    emulatorConfig->setWorkDir(workingDir);
    emulatorConfig->setRomPath(u""_s);
    emulatorConfig->setRomExt(u""_s);
    emulatorConfig->setSystem(Fp::NAME);
    emulatorConfig->setInfoSource(u""_s);

    // Ensure image directories are clear
    EmulatorArtworkEntry::Builder aeb;

    // Can reuse builder since all fields are set in each entry
    aeb.wPaths({});
    aeb.wType(u"flyer"_s);
    emulatorConfig->setArtworkEntry(aeb.build());
    aeb.wType(u"snap"_s);
    emulatorConfig->setArtworkEntry(aeb.build());
    aeb.wType(u"marquee"_s);
    emulatorConfig->setArtworkEntry(aeb.build());
    aeb.wType(u"wheel"_s);
    emulatorConfig->setArtworkEntry(aeb.build());

    // Commit emulator config
    Lr::DocHandlingError emulatorConfigWriteError = commitClifpEmulatorConfig(std::move(emulatorConfig));

    // Stop import if error occurred
    if(emulatorConfigWriteError.isValid())
        return emulatorConfigWriteError;

    //-Ensure display entry exists-----------------------------------

    // Checkout main config
    std::unique_ptr<CrudeSettings> mainConfig;
    Lr::DocHandlingError mainConfigReadError = checkoutMainConfig(mainConfig);

    // Stop import if error occurred
    if(mainConfigReadError.isValid())
        return mainConfigReadError;

    // Add default display entry if not present
    if(!mainConfig->containsDisplay(Fp::NAME))
    {
        Display::Builder db;
        db.wName(Fp::NAME);
        db.wLayout(u"Attrac-Man"_s);
        db.wRomlist(Fp::NAME);
        db.wInCycle(false);
        db.wInMenu(true);

        // All filter
        DisplayFilter::Builder dfb;
        dfb.wName(u"All"_s);
        dfb.wSortBy(DisplayFilter::Sort::AltTitle); // This uses FP's u"orderTtile"_s
        db.wFilter(dfb.build());

        // Favorites filter
        dfb = DisplayFilter::Builder();
        dfb.wName(u"Favourites"_s);
        dfb.wRule(u"Favourite equals 1"_s);
        dfb.wSortBy(DisplayFilter::Sort::AltTitle);
        db.wFilter(dfb.build());

        mainConfig->addDisplay(db.build());
    }

    // Display to update with updated tags
    Display& fpDisplay = mainConfig->display(Fp::NAME);
    QList<DisplayFilter>& displayFilters = fpDisplay.filters();

    // Remove old platform/playlist based filters
    displayFilters.removeIf([](const DisplayFilter& filter){
        const QStringList rules = filter.rules();
        for(const QString& rule : rules)
        {
            if(rule.contains(u"\\[Platform\\]"_s) || rule.contains(u"\\[Playlist\\]"_s))
                return true;
        }

        return false;
    });

    // Generate filter for each current taglist
    QStringList tagFiles;
    tagFiles.append(modifiedPlatforms());
    tagFiles.append(modifiedPlaylists());

    for(const QString& tagFile : tagFiles)
    {
        // Escape brackets in name since AM uses regex for value
        QString escaped = Qx::String::mapArg(tagFile,{
            {u"["_s, u"\\["_s},
            {u"]"_s, u"\\]"_s}
        });

        DisplayFilter::Builder dfb;
        dfb = DisplayFilter::Builder();
        dfb.wName('"' + tagFile + '"');
        dfb.wSortBy(DisplayFilter::Sort::AltTitle);
        dfb.wRule(u"Tags contains "_s + escaped);

        displayFilters.append(dfb.build());
    }

    // Commit main config
    Lr::DocHandlingError configCommitError = commitMainConfig(std::move(mainConfig));

    // Stop import if error occurred
    if(configCommitError.isValid())
        return configCommitError;

    // Add/Update marquee (if it fails, not worth aborting over so ignore error status)
    QDir fpMarqueeDirectory(mFpScraperDirectory.absoluteFilePath(MARQUEE_FOLDER_NAME));
    fpMarqueeDirectory.mkpath(u"."_s);
    QFileInfo srcMarqueeInfo(MARQUEE_PATH);
    QFile::copy(MARQUEE_PATH, fpMarqueeDirectory.absoluteFilePath(Fp::NAME + '.' + srcMarqueeInfo.completeSuffix()));

    // Return success
    return Qx::Error();
}

void Install::processBulkImageSources(const Import::ImagePaths& bulkSources)
{
    Q_UNUSED(bulkSources);
    qFatal("Attract Mode does not support Reference image mode, and that option should not be available.");
}

QString Install::generateImagePath(const RomEntry& romEntry, Fp::ImageType type)
{
    return mFpScraperDirectory.absolutePath() + '/' +
           (type == Fp::ImageType::Logo ? LOGO_FOLDER_NAME : SCREENSHOT_FOLDER_NAME) + '/' +
           romEntry.id().toString(QUuid::WithoutBraces);
}

}
