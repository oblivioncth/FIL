#include "launchboxinstall.h"
#include <QFileInfo>
#include <QDir>
#include <qhashfunctions.h>

namespace LB
{

//===============================================================================================================
// INSTALL::XMLHandle
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator==(const Install::XMLHandle& lhs, const Install::XMLHandle& rhs) noexcept
{
    return lhs.type == rhs.type && lhs.name == rhs.name;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const Install::XMLHandle& key, uint seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.type);
    seed = hash(seed, key.name);

    return seed;
}

//===============================================================================================================
// INSTALL::XMLDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::XMLDoc::XMLDoc(std::unique_ptr<QFile> xmlFile,  XMLHandle xmlMetaData, const Key&)
    : mDocumentFile(std::move(xmlFile)), mHandleTarget(xmlMetaData) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Install::XMLHandle Install::XMLDoc::getHandleTarget() const { return mHandleTarget; }

const QList<Game>& Install::XMLDoc::getGames() const { return mGames; }

const QList<AddApp>& Install::XMLDoc::getAddApps() const { return mAddApps; }

const PlaylistHeader& Install::XMLDoc::getPlaylistHeader() const { return mPlaylistHeader; }

const QList<PlaylistGame>& Install::XMLDoc::getPlaylistGames() const { return mPlaylistGames; }

void Install::XMLDoc::addGame(Game game)
{
    if(mHandleTarget.type == Platform)
        mGames.append(game);
}

void Install::XMLDoc::addAddApp(AddApp app)
{
    if(mHandleTarget.type == Platform)
        mAddApps.append(app);
}

void Install::XMLDoc::setPlaylistHeader(PlaylistHeader header)
{
    if(mHandleTarget.type == Playlist)
        mPlaylistHeader = header;
}

void Install::XMLDoc::addPlaylistGame(PlaylistGame playlistGame)
{
    if(mHandleTarget.type == Playlist)
        mPlaylistGames.append(playlistGame);
}

//===============================================================================================================
// INSTALL::XMLReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::XMLReader::XMLReader(XMLDoc* targetDoc)
    : mTargetDocument(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::XmlStreamReaderError Install::XMLReader::readInto()
{
    // Hook reader to document handle
    mStreamReader.setDevice(mTargetDocument->mDocumentFile.get());

    // Prepare error return instance
    Qx::XmlStreamReaderError readError;

    if(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XML_ROOT_ELEMENT)
            readError = readLaunchBoxDocument();
        else
            readError = Qx::XmlStreamReaderError(ERR_NOT_LB_DOC);
    }
    else
        readError = Qx::XmlStreamReaderError(mStreamReader.error());

    return readError;
}

//Private:
Qx::XmlStreamReaderError Install::XMLReader::readLaunchBoxDocument()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XMLMainElement_Game::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Platform)
                parseGame();
            else
                mStreamReader.raiseError(ERR_DOC_TYPE_MISMATCH);
        }
        else if(mStreamReader.name() == XMLMainElement_AddApp::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Platform)
                parseAddApp();
            else
                mStreamReader.raiseError(ERR_DOC_TYPE_MISMATCH);
        }
        else if(mStreamReader.name() == XMLMainElement_PlaylistHeader::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Playlist)
                parsePlaylistHeader();
            else
                mStreamReader.raiseError(ERR_DOC_TYPE_MISMATCH);
        }
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Playlist)
                parsePlaylistGame();
            else
                mStreamReader.raiseError(ERR_DOC_TYPE_MISMATCH);
        }
        else
            mStreamReader.skipCurrentElement();
    }

    // Return no error on success
    if(mStreamReader.hasError())
    {
        if(mStreamReader.error() == QXmlStreamReader::CustomError)
            return Qx::XmlStreamReaderError(mStreamReader.errorString());
        else
            return Qx::XmlStreamReaderError(mStreamReader.error());
    }
    else
        return Qx::XmlStreamReaderError();
}

void Install::XMLReader::parseGame()
{
    // Game to build
    GameBuilder gb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_ID)
            gb.wID(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_TITLE)
            gb.wTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_SERIES)
            gb.wSeries(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_DEVELOPER)
            gb.wDeveloper(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_PUBLISHER)
            gb.wPublisher(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_PLATFORM)
            gb.wPlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_SORT_TITLE)
            gb.wSortTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_DATE_ADDED)
            gb.wDateAdded(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_DATE_MODIFIED)
            gb.wDateModified(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_BROKEN)
            gb.wBroken(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_PLAYMODE)
            gb.wPlayMode(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_STATUS)
            gb.wStatus(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_REGION)
            gb.wRegion(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_NOTES)
            gb.wNotes(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_SOURCE)
            gb.wSource(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_APP_PATH)
            gb.wAppPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_COMMAND_LINE)
            gb.wCommandLine(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_RELEASE_DATE)
            gb.wReleaseDate(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_Game::ELEMENT_VERSION)
            gb.wVersion(mStreamReader.readElementText());
        else
            gb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Game and add to document
    mTargetDocument->addGame(gb.build());
}

void Install::XMLReader::parseAddApp()
{
    // Additional App to Build
    AddAppBuilder aab;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_ID)
            aab.wID(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_GAME_ID)
            aab.wGameID(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_APP_PATH)
            aab.wAppPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_COMMAND_LINE)
            aab.wCommandLine(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_AUTORUN_BEFORE)
            aab.wAutorunBefore(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_NAME)
            aab.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_AddApp::ELEMENT_WAIT_FOR_EXIT)
            aab.wWaitForExit(mStreamReader.readElementText());
        else
            aab.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Additional App and add to document
    mTargetDocument->addAddApp(aab.build());

}
void Install::XMLReader::parsePlaylistHeader()
{
    // Playlist Header to Build
    PlaylistHeaderBuilder phb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XMLMainElement_PlaylistHeader::ELEMENT_ID)
            phb.wPlaylistID(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistHeader::ELEMENT_NAME)
            phb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistHeader::ELEMENT_NESTED_NAME)
            phb.wNestedName(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistHeader::ELEMENT_NOTES)
            phb.wNotes(mStreamReader.readElementText());
        else
            phb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Header and add to document
    mTargetDocument->setPlaylistHeader(phb.build());

}

void Install::XMLReader::parsePlaylistGame()
{
    // Playlist Game to Build
    PlaylistGameBuilder pgb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_ID)
            pgb.wGameID(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_GAME_TITLE)
            pgb.wGameTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_GAME_PLATFORM)
            pgb.wGamePlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_MANUAL_ORDER)
            pgb.wManualOrder(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_LB_DB_ID)
            pgb.wLBDatabaseID(mStreamReader.readElementText());
        else
            pgb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Game and add to document
    mTargetDocument->addPlaylistGame(pgb.build());
}

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
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);
}

//-Class Functions------------------------------------------------------------------------------------------------
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
//Public:
Qx::IOOpReport Install::populateExistingItems()
{
    // Clear existing
    mExistingPlatforms.clear();
    mExistingPlaylists.clear();

    // Temp storage
    QStringList existingPlatformsList;
    QStringList existingPlaylistsList;

    Qx::IOOpReport existingCheck = Qx::getDirFileList(existingPlatformsList, mPlatformsDirectory, false, {XML_EXT});

    if(existingCheck.wasSuccessful())
        existingCheck = Qx::getDirFileList(existingPlaylistsList, mPlaylistsDirectory, false, {XML_EXT});

    // Convert lists to set and drop XML extension
    for(QString platform : existingPlatformsList)
        mExistingPlatforms.insert(platform.remove(XML_EXT));
    for(QString playlist : existingPlaylistsList)
        mExistingPlaylists.insert(playlist.remove(XML_EXT));

    return existingCheck;
}

// Get a handle to the specified XML file (do not include ".xml" extension)
Qx::XmlStreamReaderError Install::openXMLDocument(std::unique_ptr<XMLDoc>& returnBuffer, XMLHandle requestHandle)
{
    // Ensure return buffer is reset
    returnBuffer.reset();

    // Error report to return
    Qx::XmlStreamReaderError openReadError; // Defaults to no error

    // Check if existing instance is already allocated and set handle to null if so
    if(mLeasedHandles.contains(requestHandle))
    {
        returnBuffer = std::unique_ptr<Install::XMLDoc>();
        openReadError = Qx::XmlStreamReaderError(XMLReader::ERR_DOC_ALREADY_OPEN);
    }
    else
    {
        // Create unique reference to the target file for the new handle
        std::unique_ptr<QFile> xmlFile = std::make_unique<QFile>((requestHandle.type == Platform ? mPlatformsDirectory : mPlaylistsDirectory).absolutePath() +
                                                                 '/' + requestHandle.name);

        if(xmlFile->open(QFile::ReadWrite)) // Ensures that empty file is created if the target doesn't exist
        {
            // Create new handle to requested document
            std::unique_ptr<XMLDoc> returnBuffer = std::make_unique<XMLDoc>(std::move(xmlFile), requestHandle, XMLDoc::Key{});

            // Read existing file if present
            if((requestHandle.type == Platform && mExistingPlatforms.contains(requestHandle.name)) ||
                (requestHandle.type == Playlist && mExistingPlaylists.contains(requestHandle.name)))
            {
                XMLReader docReader(returnBuffer.get());
                openReadError = docReader.readInto();
            }

            // Add handle to lease set if no error occured while readding
            if(openReadError.isValid())
                returnBuffer = std::unique_ptr<Install::XMLDoc>();
            else
                mLeasedHandles.insert(requestHandle);
        }
        else
            openReadError = Qx::XmlStreamReaderError(XMLReader::ERR_DOC_IN_USE);
    }

    // Return new handle
    return openReadError;
}

bool Install::saveXMLDocument(std::unique_ptr<XMLDoc> document)
{
    // TODO: MAKE ME
    return true;
}

bool Install::revertAllChanges()
{
    // TODO: MAKE ME
    return true;
}

QSet<QString> Install::getExistingPlatforms() const { return mExistingPlatforms; }
QSet<QString> Install::getExistingPlaylists() const { return mExistingPlaylists; }


}
