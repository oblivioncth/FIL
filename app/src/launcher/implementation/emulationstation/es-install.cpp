// Unit Include
#include "es-install.h"

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/core/qx-versionnumber.h>

// Project Includes
#include "kernel/clifp.h"
#include "import/details.h"

namespace Es
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(const QString& installPath) :
    Lr::Install<LauncherId>(installPath)
{
    // Determine theoretical data root, and base everything off of it
    mDataRoot = determineDataRoot(installPath);

    // Setup data
    mPortableFile.setFileName(mDataRoot.absoluteFilePath(u"../"_s + PORTABLE_TXT_PATH));
    mExeFile.setFileName(mDataRoot.absoluteFilePath(u"../"_s + EXE_NAME));
    mGameListsDir = QDir(mDataRoot.absoluteFilePath(GAMELISTS_PATH));
    mCollectionsDir = QDir(mDataRoot.absoluteFilePath(COLLECTIONS_PATH));
    mCustomSystemsDir = QDir(mDataRoot.absoluteFilePath(CUSTOM_SYSTEMS_PATH));
    mThemesDir = QDir(mDataRoot.absoluteFilePath(THEMES_PATH));
    mDownloadedMediaDir = QDir(mDataRoot.absoluteFilePath(DOWNLOADED_MEDIA_PATH));
    mSettingsDir = QDir(mDataRoot.absoluteFilePath(SETTINGS_PATH));
    mCustomSystemsFile.setFileName(mCustomSystemsDir.absoluteFilePath(CUSTOM_SYSTEMS_FILE_PATH));
    mSettingsFile.setFileName(mSettingsDir.absoluteFilePath(SETTINGS_FILE_PATH));

    // Setup fake theme data
    mFakeThemeDir = QDir(mThemesDir.absoluteFilePath(FAKE_THEME_NAME));
    mPlatformIconsDir = QDir(mFakeThemeDir.absoluteFilePath(PLATFORM_ICONS_FOLDER_NAME));
    mPlaylistIconsDir = QDir(mFakeThemeDir.absoluteFilePath(PLAYLIST_ICONS_FOLDER_NAME));

    // Check validity
    if(!mGameListsDir.exists() ||
       !mCollectionsDir.exists() ||
       !mCustomSystemsDir.exists() ||
       !mDownloadedMediaDir.exists() ||
       !mSettingsDir.exists())
    {
        declareValid(false);
        return;
    }

    // Give the OK
    declareValid(true);
}

//-Class Functions--------------------------------------------------------------------------------------------------------
//Private:
QDir Install::determineDataRoot(const QDir& providedDir)
{
    auto& d = providedDir;

    // Check if path is above data root, otherwise assume in data root a ctor will handle remainder of validity checks
    if(d.exists(PORTABLE_TXT_PATH) || d.exists(DATA_ROOT_PATH) || d.exists(EXE_NAME))
        return providedDir.absoluteFilePath(DATA_ROOT_PATH);
    else
        return providedDir;
}

//-Instance Functions----------------------------------------------------------------------------------------------
//Private:
Qx::Error Install::populateExistingDocs(QSet<Lr::IDataDoc::Identifier>& existingDocs)
{
    // Record config docs first, since they are potentially needed later in this function
    bool hasCustomSystems = mCustomSystemsFile.exists() && mCustomSystemsFile.size() != 0;
    if(hasCustomSystems)
        existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Config, Systemlist::STD_NAME));

    // Get systems from custom systems file, if possible
    if(hasCustomSystems)
    {
        // Checkout if not already
        if(!mCustomSystemsDoc)
            if(auto err = checkoutCustomSystemsDoc(); err.isValid())
                return err;

        // Enumerate
        for(const System& s : mCustomSystemsDoc->systems())
            existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Platform, s.fullName()));
    }

    // Enumerate collections
    QFileInfoList existingList;
    if(auto res = Qx::dirContentInfoList(existingList, mCollectionsDir, {CUSTOM_PREFIX + u"*."_s + CFG_EXT}); res.isFailure())
        return res;

    for(const QFileInfo& playlistFile : std::as_const(existingList))
        existingDocs.insert(Lr::IDataDoc::Identifier(Lr::IDataDoc::Type::Playlist, playlistFile.baseName().remove(CUSTOM_PREFIX)));

    // Return success
    return Qx::Error();
}

QString Install::dataDocPath(Lr::IDataDoc::Identifier identifier) const
{
    switch(identifier.docType())
    {
    case Lr::IDataDoc::Type::Platform:
    {
        const QString gamelist(System::fullNameToName(identifier.docName()) + '/' + GAMELIST_FILENAME);
        return mGameListsDir.absoluteFilePath(gamelist);
        break;
    }
    case Lr::IDataDoc::Type::Playlist:
    {
        const QString collection(CUSTOM_PREFIX + identifier.docName() + '.' + CFG_EXT);
        return mCollectionsDir.absoluteFilePath(collection);
        break;
    }

    case Lr::IDataDoc::Type::Config:
        qCritical("Config type not used for ES-DE.");
        return {};
        break;
    default:
        throw new std::invalid_argument("Function argument was not of type Lr::IDataDoc::Identifier");
    }
}

std::unique_ptr<Gamelist> Install::preparePlatformDocCheckout(const QString& translatedName)
{
    // Create doc file reference
    Lr::IDataDoc::Identifier docId(Lr::IDataDoc::Type::Platform, translatedName);

    // Construct unopened document and return
    return std::make_unique<Gamelist>(this, dataDocPath(docId), translatedName, Import::Details::current().updateOptions);
}

void Install::preparePlatformDocCommit(const Gamelist& document)
{
    // Update corresponding custom system entry
    if(document.isEmpty())
        mCustomSystemsDoc->removeSystem(document.fullSystemName());
    else
    {
        System::Builder sb;
        sb.wName(document.shortSystemName());
        sb.wFullName(document.fullSystemName());
        sb.wSystemSortName(document.fullSystemName());
        sb.wPath(ROMPATH_PLACEHOLDER + '/' + document.shortSystemName());
        sb.wExtension(u'.' + DUMMY_EXT);
        sb.wCommand(QString(), Import::Details::current().clifpPath + u' ' + CLIFp::parametersFromStandard(BASENAME_PLACEHOLDER));
        sb.wPlatform(document.shortSystemName());
        sb.wTheme(document.shortSystemName());
        mCustomSystemsDoc->insertSystem(sb.build());
    }
}

void Install::preparePlaylistDocCommit(const Collection& collection)
{
    // Make sure playlist is enabled in setting
    if(!mSettingsDoc->containsCustomCollection(collection.name()))
        mSettingsDoc->addCustomCollection(collection.name());
}

std::unique_ptr<Collection> Install::preparePlaylistDocCheckout(const QString& translatedName)
{
    // Create doc file reference
    Lr::IDataDoc::Identifier docId(Lr::IDataDoc::Type::Playlist, translatedName);

    // Construct unopened document
    return std::make_unique<Collection>(this, dataDocPath(docId), translatedName, Import::Details::current().updateOptions);
}

Lr::DocHandlingError Install::checkoutCustomSystemsDoc()
{
    mCustomSystemsDoc = std::make_unique<Systemlist>(this, mCustomSystemsFile.fileName());

    // Construct doc reader
    std::shared_ptr<SystemlistReader> docReader = std::make_shared<SystemlistReader>(mCustomSystemsDoc.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        mCustomSystemsDoc.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::commitCustomSystemsDoc()
{
    // Prepare writer
    std::shared_ptr<SystemlistWriter> docWriter = std::make_shared<SystemlistWriter>(mCustomSystemsDoc.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Ensure document is cleared
    mCustomSystemsDoc.reset();

    // Return write status
    return writeErrorStatus;
}

Lr::DocHandlingError Install::checkoutSettingsDoc()
{
    mSettingsDoc = std::make_unique<Settings>(this, mSettingsFile.fileName());

    // Construct doc reader
    std::shared_ptr<SettingsReader> docReader = std::make_shared<SettingsReader>(mSettingsDoc.get());

    // Open document
    Lr::DocHandlingError readErrorStatus = checkoutDataDocument(docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        mSettingsDoc.reset();

    // Return status
    return readErrorStatus;
}

Lr::DocHandlingError Install::commitSettingsDoc()
{
    // Prepare writer
    std::shared_ptr<SettingsWriter> docWriter = std::make_shared<SettingsWriter>(mSettingsDoc.get());

    // Write
    Lr::DocHandlingError writeErrorStatus = commitDataDocument(docWriter);

    // Ensure document is cleared
    mSettingsDoc.reset();

    // Return write status
    return writeErrorStatus;
}

//Public:
void Install::softReset()
{
    closeDataDocument(std::move(mCustomSystemsDoc));
    closeDataDocument(std::move(mSettingsDoc));
    mPlaylistGameSystemNameCache.clear();
}

QList<Import::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }
bool Install::isRunning() const { return Qx::processIsRunning(EXE_NAME); }

QString Install::versionString() const
{
    // Limits to first 3 segments for consistency since that's what AttractMode seems to use

    // Try executable first if it's known
    if(QString exeVer = versionFromExecutable(); !exeVer.isEmpty())
        return exeVer;

    // Try log
    if(mLogFile.exists())
    {
        QFile log(mLogFile.fileName());
        if(log.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString verLine = QString::fromLatin1(log.readLine());
            if(!verLine.isEmpty())
            {
                QRegularExpressionMatch sv = LOG_VERSION_REGEX.matchView(verLine);
                if(sv.hasMatch())
                {
                    Qx::VersionNumber ver = Qx::VersionNumber::fromString(sv.captured(u"ver"_s));
                    if(!ver.isNull())
                        return ver.toString();
                }
            }
        }
    }

    // Can't determine version
    return Lr::IInstall::versionString();
}

QString Install::translateDocName(const QString& originalName, Lr::IDataDoc::Type type) const
{
    using enum Lr::IDataDoc::Type;
    switch(type)
    {
        case Platform:
            return System::originalNameToFullName(originalName);
        case Playlist:
            return Collection::originalNameToName(originalName);
        case Config:
            return Qx::kosherizeFileName(originalName);
    }

    qFatal("Invalid enumeration value");
    return {};
}

QDir Install::romsDirectory() const { return mRomsDir; }

Qx::Error Install::preImport()
{
    if(auto err = checkoutSettingsDoc(); err.isValid())
        return err;

    // Determine ROMs directory
    if(mPortableFile.exists())
        mRomsDir.setPath(mDataRoot.absoluteFilePath(u"../ROMs"_s));
    else
        mRomsDir.setPath(mSettingsDoc->romDirectory());

    return Qx::Error();
}

Qx::Error Install::postImport()
{
    // Save settings doc
    return commitSettingsDoc();
}

Qx::Error Install::prePlatformsImport()
{
    // Checkout custom systems doc if not already
    Qx::Error err;
    if(!mCustomSystemsDoc)
        err = checkoutCustomSystemsDoc();

    return err;
}

Qx::Error Install::postPlatformsImport()
{
    // Save custom systems doc
    return commitCustomSystemsDoc();
}

QString Install::generateImagePath(const Game& game, Fp::ImageType type)
{
    return mDownloadedMediaDir.absoluteFilePath(
        game.systemName() + '/' +
        (type == Fp::ImageType::Logo ? LOGO_PATH : SCREENSHOT_PATH) + '/' +
        QFileInfo(game.path()).baseName()
    );
}

void Install::processBulkImageSources(const Import::ImagePaths& bulkSources)
{
    Q_UNUSED(bulkSources);
    qFatal("ES-DE does not support Reference image mode, and that option should not be available.");
}

QString Install::platformCategoryIconPath() const { return mFakeThemeDir.absoluteFilePath(FAKE_THEME_ICON_NAME); }
std::optional<QDir> Install::platformIconsDirectory() const { return mPlatformIconsDir; }
std::optional<QDir> Install::playlistIconsDirectory() const { return mPlaylistIconsDir; }

}
