#include "launchboxinstall.h"
#include <QFileInfo>
#include <QDir>
#include <qhashfunctions.h>

namespace LB
{

//===============================================================================================================
// LAUNCHBOX INSTALL::XMLHandle
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator==(const LaunchBoxInstall::XMLHandle& lhs, const LaunchBoxInstall::XMLHandle& rhs) noexcept
{
    return lhs.type == rhs.type && lhs.name == rhs.name;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const LaunchBoxInstall::XMLHandle& key, uint seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.type);
    seed = hash(seed, key.name);

    return seed;
}

//===============================================================================================================
// LAUNCHBOX INSTALL::LBXMLDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
LaunchBoxInstall::LBXMLDoc::LBXMLDoc(std::unique_ptr<QFile> xmlFile,  XMLHandle xmlMetaData, const Key&) : mDocumentFile(std::move(xmlFile)), mHandleTarget(xmlMetaData)
{
    // TODO: MAKE ME
    mDocumentFile = std::move(xmlFile);
    mHandleTarget = xmlMetaData;


}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
QXmlStreamReader::Error LaunchBoxInstall::LBXMLDoc::readAll()
{
    // Attached File
    QXmlStreamReader itemReader(mDocumentFile.get());

    bool isLaunchboxXml = false;

    // Read file
    while(!itemReader.atEnd())
    {
        itemReader.readNextStartElement();

        if(itemReader.read)




    }


    // TODO: MAKE ME
    return QXmlStreamReader::Error::NoError;

}
void LaunchBoxInstall::LBXMLDoc::close(bool flushData)
{
    mDocumentFile->close();
    mDocumentFile.reset();

    if(flushData)
    {
        mGames.clear();
        mAdditionalApps.clear();
        mPlaylistHeader = LB::LaunchBoxPlaylistHeader();
        mPlaylistGames.clear();
    }
}

bool LaunchBoxInstall::LBXMLDoc::isValid() const
{
    //TODO: MAKE ME
    return true;
}

LaunchBoxInstall::XMLHandle LaunchBoxInstall::LBXMLDoc::getHandleTarget() const { return mHandleTarget; }

const QList<LaunchBoxGame>& LaunchBoxInstall::LBXMLDoc::getGames() const
{
    if(mHandleTarget.type != Platform)
        return DUMMY_GAME_LIST;

    return mGames;
}

const QList<LaunchBoxAdditionalApp>& LaunchBoxInstall::LBXMLDoc::getAdditionalApps() const
{
    if(mHandleTarget.type != Platform)
        return DUMMY_ADDITIONAL_APP_LIST;

    return mAdditionalApps;
}

const LaunchBoxPlaylistHeader& LaunchBoxInstall::LBXMLDoc::getPlaylistHeader() const
{
    if(mHandleTarget.type != Playlist)
        return DUMMY_PLAYLIST_HEADER;

    return mPlaylistHeader;
}

const QList<LaunchBoxPlaylistGame>& LaunchBoxInstall::LBXMLDoc::getPlaylistGames() const
{
    if(mHandleTarget.type != Platform)
        return DUMMY_PLAYLIST_GAME_LIST;

    return mPlaylistGames;
}

void LaunchBoxInstall::LBXMLDoc::addGame(LaunchBoxGame game){ mGames.append(game); }

void LaunchBoxInstall::LBXMLDoc::addAdditionalApp(LaunchBoxAdditionalApp app) { mAdditionalApps.append(app); }

void LaunchBoxInstall::LBXMLDoc::setPlaylistHeader(LaunchBoxPlaylistHeader header) { mPlaylistHeader = header; }

void LaunchBoxInstall::LBXMLDoc::addPlaylistGame(LaunchBoxPlaylistGame playlistGame) { mPlaylistGames.append(playlistGame); }

//===============================================================================================================
// LAUNCHBOX INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxInstall::LaunchBoxInstall(QString installPath)
{
    // Ensure instance will be valid
    if(!pathIsValidLaunchBoxInstall(installPath))
        assert("Cannot create a LaunchBoxInstall instance with an invalid installPath. Check first with LaunchBoxInstall::pathIsValidLaunchBoxInstall(QString).");

    // Initialize files and directories;
    mRootDirectory = QDir(installPath);
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
bool LaunchBoxInstall::pathIsValidLaunchBoxInstall(QString installPath)
{
    QFileInfo platformsFolder(installPath + "/" + PLATFORMS_PATH);
    QFileInfo playlistsFolder(installPath + "/" + PLATFORMS_PATH);
    QFileInfo mainEXE(installPath + "/" + MAIN_EXE_PATH);

    return platformsFolder.exists() && platformsFolder.isDir() &&
           playlistsFolder.exists() && playlistsFolder.isDir() &&
           mainEXE.exists() && mainEXE.isExecutable();
}

//-Instance Functions----------------------------------------------------------------------------------------------
//Public:
Qx::IO::IOOpReport LaunchBoxInstall::populateExistingItems()
{
    // Clear existing
    mExistingPlatforms.clear();
    mExistingPlaylists.clear();

    // Temp storage
    QStringList existingPlatformsList;
    QStringList existingPlaylistsList;

    Qx::IO::IOOpReport existingCheck = Qx::IO::getDirFileList(existingPlatformsList, mPlatformsDirectory, false, {XML_EXT});

    if(existingCheck.wasSuccessful())
        existingCheck = Qx::IO::getDirFileList(existingPlaylistsList, mPlaylistsDirectory, false, {XML_EXT});

    // Convert lists to set and drop XML extension
    for(QString platform : existingPlatformsList)
        mExistingPlatforms.insert(platform.remove(XML_EXT));
    for(QString playlist : existingPlaylistsList)
        mExistingPlaylists.insert(playlist.remove(XML_EXT));

    return existingCheck;
}

// Get a handle to the specified XML file (do not include ".xml" extension)
std::shared_ptr<LaunchBoxInstall::LBXMLDoc> LaunchBoxInstall::openXMLDocument(XMLHandle requestHandle)
{
    // Check if existing instance is already allocated and return a handle to it if so
    if(mLeasedHandles.contains(requestHandle))
        return mLeasedHandles.value(requestHandle);

    // Create unique reference to the target file for the new handle
    std::unique_ptr<QFile> xmlFile = std::make_unique<QFile>((requestHandle.type == Platform ? mPlatformsDirectory : mPlaylistsDirectory).absolutePath() +
                                                             '/' + requestHandle.name);

    xmlFile->open(QFile::ReadWrite); // Ensures that empty file is created if the target doesn't exist

    // Create new handle to requested document
    std::shared_ptr<LBXMLDoc> newHandle = std::make_shared<LBXMLDoc>(std::move(xmlFile), requestHandle, LBXMLDoc::Key{});

    // Add handle to lease map
    mLeasedHandles.insert(requestHandle, newHandle);

    // Return new handle
    return newHandle;
}

bool LaunchBoxInstall::saveXMLDocument(std::shared_ptr<LBXMLDoc> document)
{
    // TODO: MAKE ME
    return true;
}

bool LaunchBoxInstall::revertAllChanges()
{
    // TODO: MAKE ME
    return true;
}

QSet<QString> LaunchBoxInstall::getExistingPlatforms() const { return mExistingPlatforms; }
QSet<QString> LaunchBoxInstall::getExistingPlaylists() const { return mExistingPlaylists; }


}
