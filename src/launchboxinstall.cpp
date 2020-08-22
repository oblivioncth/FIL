#include "launchboxinstall.h"
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
// INSTALL::XMLHandle
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const Install::XMLHandle& lhs, const Install::XMLHandle& rhs) noexcept
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
Install::XMLDoc::XMLDoc(std::unique_ptr<QFile> xmlFile, XMLHandle xmlMetaData, UpdateOptions updateOptions, Qx::FreeIndexTracker<int>* lbDBFIDT, const Key&)
    : mDocumentFile(std::move(xmlFile)), mHandleTarget(xmlMetaData), mUpdateOptions(updateOptions), mPlaylistGameFreeLBDBIDTracker(lbDBFIDT) {}

//-Class Functions-----------------------------------------------------------------------------------------------------
QString Install::XMLDoc::makeFileNameLBKosher(QString fileName)
{
    // Perform general kosherization
    fileName = Qx::kosherizeFileName(fileName);

    // LB specific changes
    fileName.replace('#','_');
    fileName.replace('\'','_');

    return fileName;
}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Install::XMLHandle Install::XMLDoc::getHandleTarget() const { return mHandleTarget; }

const QHash<QUuid, Game>& Install::XMLDoc::getFinalGames() const { return mGamesFinal; }
const QHash<QUuid, AddApp>& Install::XMLDoc::getFinalAddApps() const { return mAddAppsFinal; }
const PlaylistHeader& Install::XMLDoc::getPlaylistHeader() const { return mPlaylistHeader; }
const QHash<QUuid, PlaylistGame>& Install::XMLDoc::getFinalPlaylistGames() const { return mPlaylistGamesFinal; }

bool Install::XMLDoc::containsGame(QUuid gameID) const { return mGamesFinal.contains(gameID) || mGamesExisting.contains(gameID); }
bool Install::XMLDoc::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }
bool Install::XMLDoc::containsPlaylistGame(QUuid gameID) const { return mPlaylistGamesFinal.contains(gameID) || mPlaylistGamesExisting.contains(gameID); }

void Install::XMLDoc::addGame(Game game)
{
    // Only act if this doc is the appropriate type
    if(mHandleTarget.type == Platform)
    {
        QUuid key = game.getID();

        // Check if game exists
        if(mGamesExisting.contains(key))
        {
            // Replace if existing update is on, move existing otherwise
            if(mUpdateOptions.importMode == NewAndExisting)
            {
                game.transferOtherFields(mGamesExisting[key].getOtherFields());
                mGamesFinal[key] = game;
                mGamesExisting.remove(key);
            }
            else
            {
                mGamesFinal[key] = std::move(mGamesExisting[key]);
                mGamesExisting.remove(key);
            }

        }
        else
            mGamesFinal[key] = game;
    }
}

void Install::XMLDoc::addAddApp(AddApp app)
{
    // Only act if this doc is the appropriate type
    if(mHandleTarget.type == Platform)
    {
        QUuid key = app.getID();

        // Check if add app exists
        if(mAddAppsExisting.contains(key))
        {
            // Replace if existing update is on, move existing otherwise
            if(mUpdateOptions.importMode == NewAndExisting)
            {
                app.transferOtherFields(mAddAppsExisting[key].getOtherFields());
                mAddAppsFinal[key] = app;
                mAddAppsExisting.remove(key);
            }
            else
            {
                mAddAppsFinal[key] = std::move(mAddAppsExisting[key]);
                mAddAppsExisting.remove(key);
            }

        }
        else
            mAddAppsFinal[key] = app;
    }
}

void Install::XMLDoc::setPlaylistHeader(PlaylistHeader header)
{
    // Only act if this doc is the appropriate type
    if(mHandleTarget.type == Playlist)
    {
        header.transferOtherFields(mPlaylistHeader.getOtherFields());
        mPlaylistHeader = header;
    }
}

void Install::XMLDoc::addPlaylistGame(PlaylistGame playlistGame)
{
    // Only act if this doc is the appropriate type
    if(mHandleTarget.type == Playlist)
    {
        QUuid key = playlistGame.getGameID();

        // Check if playlist game exists
        if(mPlaylistGamesExisting.contains(key))
        {
            // Replace if existing update is on, move existing otherwise
            if(mUpdateOptions.importMode == NewAndExisting)
            {
                playlistGame.transferOtherFields(mPlaylistGamesExisting[key].getOtherFields());
                playlistGame.setLBDatabaseID(mPlaylistGamesExisting[key].getLBDatabaseID());
                mPlaylistGamesFinal[key] = playlistGame;
                mPlaylistGamesExisting.remove(key);
            }
            else
            {
                mPlaylistGamesFinal[key] = std::move(mPlaylistGamesExisting[key]);
                mPlaylistGamesExisting.remove(key);
            }

        }
        else
        {
            playlistGame.setLBDatabaseID(mPlaylistGameFreeLBDBIDTracker->reserveFirstFree());
            mPlaylistGamesFinal[key] = playlistGame;
        }
    }
}

void Install::XMLDoc::clearFile() { mDocumentFile->resize(0); }

void Install::XMLDoc::finalize()
{
    // Copy items to final list if obsolete entries are to be kept
    if(!mUpdateOptions.removeObsolete)
    {
        mGamesFinal.insert(mGamesExisting);
        mAddAppsFinal.insert(mAddAppsExisting);
        mPlaylistGamesFinal.insert(mPlaylistGamesExisting);
    }

    // Clear existing lists
    mGamesExisting.clear();
    mAddAppsExisting.clear();
    mPlaylistGamesExisting.clear();
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
            readError = Qx::XmlStreamReaderError(populateErrorWithTarget(ERR_NOT_LB_DOC, mTargetDocument->getHandleTarget()));
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
                mStreamReader.raiseError(populateErrorWithTarget(ERR_DOC_TYPE_MISMATCH, mTargetDocument->getHandleTarget()));
        }
        else if(mStreamReader.name() == XMLMainElement_AddApp::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Platform)
                parseAddApp();
            else
                mStreamReader.raiseError(populateErrorWithTarget(ERR_DOC_TYPE_MISMATCH, mTargetDocument->getHandleTarget()));
        }
        else if(mStreamReader.name() == XMLMainElement_PlaylistHeader::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Playlist)
                parsePlaylistHeader();
            else
                mStreamReader.raiseError(populateErrorWithTarget(ERR_DOC_TYPE_MISMATCH, mTargetDocument->getHandleTarget()));
        }
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::NAME)
        {
            if(mTargetDocument->getHandleTarget().type == Playlist)
                parsePlaylistGame();
            else
                mStreamReader.raiseError(populateErrorWithTarget(ERR_DOC_TYPE_MISMATCH, mTargetDocument->getHandleTarget()));
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
    LB::Game existingGame = gb.build();
    mTargetDocument->mGamesExisting[existingGame.getID()] = existingGame;
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
    LB::AddApp existingAddApp = aab.build();
    mTargetDocument->mAddAppsExisting[existingAddApp.getID()] = existingAddApp;

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
    mTargetDocument->mPlaylistHeader = phb.build();

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
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_GAME_FILE_NAME)
            pgb.wGameFileName(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_GAME_PLATFORM)
            pgb.wGamePlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_MANUAL_ORDER)
            pgb.wManualOrder(mStreamReader.readElementText());
        else if(mStreamReader.name() == XMLMainElement_PlaylistGame::ELEMENT_LB_DB_ID)
            pgb.wLBDatabaseID(mStreamReader.readElementText());
        else
            pgb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Game
    LB::PlaylistGame existingPlaylistGame = pgb.build();

    // Correct LB ID if it is invalid and then add it to tracker
    if(existingPlaylistGame.getLBDatabaseID() < 0)
        existingPlaylistGame.setLBDatabaseID(mTargetDocument->mPlaylistGameFreeLBDBIDTracker->reserveFirstFree());
    else
        mTargetDocument->mPlaylistGameFreeLBDBIDTracker->release(existingPlaylistGame.getLBDatabaseID());

    // Add to document
    mTargetDocument->mPlaylistGamesExisting[existingPlaylistGame.getGameID()] = existingPlaylistGame;
}

//===============================================================================================================
// INSTALL::XMLWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::XMLWriter::XMLWriter(XMLDoc* sourceDoc)
    : mSourceDocument(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
QString Install::XMLWriter::writeOutOf()
{
    // Hook writer to document handle
    mStreamWriter.setDevice(mSourceDocument->mDocumentFile.get());

    // Enable auto formating
    mStreamWriter.setAutoFormatting(true);

    // Write standard XML header
    mStreamWriter.writeStartDocument();

    // Write main LaunchBox tag
    mStreamWriter.writeStartElement(XML_ROOT_ELEMENT);

    // Write main body
    if(!writeLaunchBoxDocument())
        return mStreamWriter.device()->errorString();

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return QString();
}

//Private:
bool Install::XMLWriter::writeLaunchBoxDocument()
{
    // Platform procedure
    if(mSourceDocument->getHandleTarget().type == Platform)
    {
        // Write all games
        for(const Game& game : mSourceDocument->getFinalGames())
        {
            if(!writeGame(game))
                return false;
        }

        // Write all additional apps
        for(const AddApp& addApp : mSourceDocument->getFinalAddApps())
        {
            if(!writeAddApp(addApp))
                return false;
        }
    }
    else // Playlist procedure
    {
        // Write playlist header
        if(!writePlaylistHeader(mSourceDocument->getPlaylistHeader()))
            return false;

        // Write all playlist games
        for(const PlaylistGame& playlistGame : mSourceDocument->getFinalPlaylistGames())
        {
            if(!writePlaylistGame(playlistGame))
                return false;
        }
    }

    // Return true on success
    return true;
}

bool Install::XMLWriter::writeGame(const Game& game)
{
    // Write opening tag
    mStreamWriter.writeStartElement(XMLMainElement_Game::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_ID, game.getID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_TITLE, game.getTitle());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_SERIES, game.getSeries());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_DEVELOPER, game.getDeveloper());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_PUBLISHER, game.getPublisher());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_PLATFORM, game.getPlatform());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_SORT_TITLE, game.getSortTitle());

    if(game.getDateAdded().isValid()) // LB is picky with dates
        mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_DATE_ADDED, game.getDateAdded().toString(Qt::ISODateWithMs));

    if(game.getDateModified().isValid())// LB is picky with dates
        mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_DATE_MODIFIED, game.getDateModified().toString(Qt::ISODateWithMs));

    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_BROKEN, game.isBroken() ? "true" : "false");
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_PLAYMODE, game.getPlayMode());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_STATUS, game.getStatus());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_REGION, game.getRegion());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_NOTES, game.getNotes());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_SOURCE, game.getSource());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_APP_PATH, game.getAppPath());
    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_COMMAND_LINE, game.getCommandLine());

    if(game.getReleaseDate().isValid()) // LB is picky with dates
        mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_RELEASE_DATE, game.getReleaseDate().toString(Qt::ISODateWithMs));

    mStreamWriter.writeTextElement(XMLMainElement_Game::ELEMENT_VERSION, game.getVersion());

    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    for(QHash<QString, QString>::const_iterator i = game.getOtherFields().constBegin(); i != game.getOtherFields().constEnd(); ++i)
    {
        mStreamWriter.writeTextElement(i.key(), i.value());

        if(mStreamWriter.hasError())
            return false;
    }

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return true on success
    return true;
}

bool Install::XMLWriter::writeAddApp(const AddApp& addApp)
{
    // Write opening tag
    mStreamWriter.writeStartElement(XMLMainElement_AddApp::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_ID, addApp.getID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_GAME_ID, addApp.getGameID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_APP_PATH, addApp.getAppPath());
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_COMMAND_LINE, addApp.getCommandLine());
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_AUTORUN_BEFORE, addApp.isAutorunBefore() ? "true" : "false");
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_NAME, addApp.getName());
    mStreamWriter.writeTextElement(XMLMainElement_AddApp::ELEMENT_WAIT_FOR_EXIT, addApp.isWaitForExit() ? "true" : "false");

    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    for(QHash<QString, QString>::const_iterator i = addApp.getOtherFields().constBegin(); i != addApp.getOtherFields().constEnd(); ++i)
    {
        mStreamWriter.writeTextElement(i.key(), i.value());

        if(mStreamWriter.hasError())
            return false;
    }

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return true on success
    return true;
}

bool Install::XMLWriter::writePlaylistHeader(const PlaylistHeader& playlistHeader)
{
    // Write opening tag
    mStreamWriter.writeStartElement(XMLMainElement_PlaylistHeader::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistHeader::ELEMENT_ID, playlistHeader.getPlaylistID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistHeader::ELEMENT_NAME, playlistHeader.getName());
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistHeader::ELEMENT_NESTED_NAME, playlistHeader.getNestedName());
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistHeader::ELEMENT_NOTES, playlistHeader.getNotes());

    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    for(QHash<QString, QString>::const_iterator i = playlistHeader.getOtherFields().constBegin(); i != playlistHeader.getOtherFields().constEnd(); ++i)
    {
        mStreamWriter.writeTextElement(i.key(), i.value());

        if(mStreamWriter.hasError())
            return false;
    }

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return true on success
    return true;
}

bool Install::XMLWriter::writePlaylistGame(const PlaylistGame& playlistGame)
{
    // Write opening tag
    mStreamWriter.writeStartElement(XMLMainElement_PlaylistGame::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistGame::ELEMENT_ID, playlistGame.getGameID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistGame::ELEMENT_GAME_TITLE, playlistGame.getGameTitle());
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistGame::ELEMENT_GAME_PLATFORM, playlistGame.getGamePlatform());
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistGame::ELEMENT_MANUAL_ORDER, QString::number(playlistGame.getManualOrder()));
    mStreamWriter.writeTextElement(XMLMainElement_PlaylistGame::ELEMENT_LB_DB_ID, QString::number(playlistGame.getLBDatabaseID()));

    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    for(QHash<QString, QString>::const_iterator i = playlistGame.getOtherFields().constBegin(); i != playlistGame.getOtherFields().constEnd(); ++i)
    {
        mStreamWriter.writeTextElement(i.key(), i.value());

        if(mStreamWriter.hasError())
            return false;
    }

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return true on success
    return true;
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
    mPlatformImagesDirectory = QDir(installPath + '/' + PLATFORM_IMAGES_PATH);
}

//-Class Functions------------------------------------------------------------------------------------------------
//Private:
void Install::allowUserWriteOnXML(QString xmlPath)
{
    PACL pDacl,pNewDACL;
    EXPLICIT_ACCESS ExplicitAccess;
    PSECURITY_DESCRIPTOR ppSecurityDescriptor;
    PSID psid;

    CString xmlPathC = xmlPath.toStdWString().c_str();
    LPTSTR lpStr = xmlPathC.GetBuffer();

    GetNamedSecurityInfo(lpStr, SE_FILE_OBJECT,DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, &ppSecurityDescriptor);
    ConvertStringSidToSid(L"S-1-1-0", &psid);

    ExplicitAccess.grfAccessMode = SET_ACCESS;
    ExplicitAccess.grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.ptstrName = (LPTSTR) psid;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;

    SetEntriesInAcl(1, &ExplicitAccess, pDacl, &pNewDACL);
    SetNamedSecurityInfo(lpStr,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL);

    LocalFree(pNewDACL);
    LocalFree(psid);

    xmlPathC.ReleaseBuffer();
}

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
//Private:
QString Install::transferImage(ImageMode imageOption, QDir sourceDir, QString destinationSubPath, const LB::Game& game)
{
    // Parse to paths
    QString gameIDString = game.getID().toString(QUuid::WithoutBraces);
    QString sourcePath = sourceDir.absolutePath() + '/' + gameIDString.left(2) + '/' + gameIDString.mid(2, 2) + '/' + gameIDString + IMAGE_EXT;
    QString destinationPath = mPlatformImagesDirectory.absolutePath() + '/' + game.getPlatform() + '/' + destinationSubPath + '/' + gameIDString + IMAGE_EXT;

    // Image Info
    QFileInfo sourceInfo(sourcePath);
    QFileInfo destinationInfo(destinationPath);
    bool sourceAvailable = sourceInfo.exists() && !sourceInfo.isSymLink();
    bool destinationOccupied = destinationInfo.exists() && (destinationInfo.isFile() || destinationInfo.isSymLink());

    // Determine backup path
    QString backupPath = destinationInfo.absolutePath() + '/' + destinationInfo.baseName() + MODIFIED_FILE_EXT;

    // Temporarily backup image if it already exists
    if(destinationOccupied && sourceAvailable)
        if(!QFile::rename(destinationPath, backupPath)) // Temp backup
            return ERR_IMAGE_WONT_BACKUP.arg(destinationPath);

    // Linking error tracker
    std::error_code linkError;

    // Handle transfer if source is available
    if(sourceAvailable)
    {
        switch(imageOption)
        {
            case LB_Copy:
                if(!QFile::copy(sourcePath, destinationPath))
                {
                    QFile::rename(backupPath, destinationPath); // Restore Backup
                    return ERR_IMAGE_WONT_COPY.arg(sourcePath, destinationPath);
                }
                else if(QFile::exists(backupPath))
                    QFile::remove(backupPath);
                else
                    mPurgableImages.append(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
                break;

            case LB_Link:
                std::filesystem::create_symlink(sourcePath.toStdString(), destinationPath.toStdString(), linkError);
                if(linkError)
                {
                    QFile::rename(backupPath, destinationPath); // Restore Backup
                    return ERR_IMAGE_WONT_LINK.arg(sourcePath, destinationPath);
                }
                else if(QFile::exists(backupPath))
                    QFile::remove(backupPath);
                else
                    mPurgableImages.append(destinationPath); // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
                break;

            case FP_Link:
                if(!QFile::rename(sourcePath, destinationPath))
                {
                    QFile::rename(backupPath, destinationPath); // Restore Backup
                    return ERR_IMAGE_WONT_MOVE.arg(sourcePath, destinationPath);
                }
                else
                {
                    std::filesystem::create_symlink(destinationPath.toStdString(), sourcePath.toStdString(), linkError);
                    if(linkError)
                    {
                        QFile::rename(destinationPath, sourcePath); // Revert move
                        QFile::rename(backupPath, destinationPath); // Restore Backup
                        return ERR_IMAGE_WONT_LINK.arg(destinationPath, sourcePath);
                    }
                    else if(QFile::exists(backupPath))
                        QFile::remove(backupPath);
                    else
                        mLinksToReverse[sourcePath] = destinationPath; // Only queue image to be removed on failure if its new, so existing images arent deleted on revert
                }
                break;
        }
    }

    // Return null string on success
    return QString();
}

//Public:
QString Install::populateErrorWithTarget(QString error, XMLHandle target)
{
    return error.arg(target.type == Platform ? "Platform" : "Playlist").arg(target.name);
}

Qx::IOOpReport Install::populateExistingItems()
{
    // Clear existing
    mExistingPlatforms.clear();
    mExistingPlaylists.clear();

    // Temp storage
    QStringList existingPlatformsList;
    QStringList existingPlaylistsList;

    Qx::IOOpReport existingCheck = Qx::getDirFileList(existingPlatformsList, mPlatformsDirectory, QDirIterator::Subdirectories, {XML_EXT}, false);

    if(existingCheck.wasSuccessful())
        existingCheck = Qx::getDirFileList(existingPlaylistsList, mPlaylistsDirectory, QDirIterator::Subdirectories, {XML_EXT}, false);

    // Convert lists to set and drop XML extension
    for(QString platform : existingPlatformsList)
        mExistingPlatforms.insert(QFileInfo(platform).baseName());
    for(QString playlist : existingPlaylistsList)
        mExistingPlaylists.insert(QFileInfo(playlist).baseName());

    return existingCheck;
}

// Get a handle to the specified XML file (do not include ".xml" extension)
Qx::XmlStreamReaderError Install::openXMLDocument(std::unique_ptr<XMLDoc>& returnBuffer, XMLHandle requestHandle, UpdateOptions updateOptions)
{
    // Ensure return buffer is reset
    returnBuffer.reset();

    // Error report to return
    Qx::XmlStreamReaderError openReadError; // Defaults to no error

    // Check if existing instance is already allocated and set handle to null if so
    if(mLeasedHandles.contains(requestHandle))
    {
        returnBuffer = std::unique_ptr<Install::XMLDoc>();
        openReadError = Qx::XmlStreamReaderError(populateErrorWithTarget(XMLReader::ERR_DOC_ALREADY_OPEN, requestHandle));
    }
    else
    {
        // Get full path to target file
        QString targetPath = (requestHandle.type == Platform ? mPlatformsDirectory : mPlaylistsDirectory).absolutePath() + '/' +
                              XMLDoc::makeFileNameLBKosher(requestHandle.name) + XML_EXT;

        // Create unique reference to the target file for the new handle
        std::unique_ptr<QFile> xmlFile = std::make_unique<QFile>(targetPath);

        // Create backup if required
        QFileInfo targetInfo(targetPath);

        if(targetInfo.exists() && targetInfo.isFile())
        {
            QString backupPath = targetInfo.absolutePath() + '/' + targetInfo.baseName() + MODIFIED_FILE_EXT;

            if(QFile::exists(backupPath) && QFileInfo(backupPath).isFile())
            {
                if(!QFile::remove(backupPath))
                    return Qx::XmlStreamReaderError(populateErrorWithTarget(XMLReader::ERR_BAK_WONT_DEL, requestHandle));
            }

            if(!QFile::copy(targetPath, backupPath))
                return Qx::XmlStreamReaderError(populateErrorWithTarget(XMLReader::ERR_CANT_MAKE_BAK, requestHandle));
        }

        // Add file to modified list
        mModifiedXMLDocuments.append(targetPath);

        // Open File
        if(xmlFile->open(QFile::ReadWrite)) // Ensures that empty file is created if the target doesn't exist
        {
            // Create new handle to requested document
            returnBuffer = std::make_unique<XMLDoc>(std::move(xmlFile), requestHandle, updateOptions, &mLBDatabaseIDTracker, XMLDoc::Key{});

            // Read existing file if present
            if((requestHandle.type == Platform && mExistingPlatforms.contains(requestHandle.name)) ||
                (requestHandle.type == Playlist && mExistingPlaylists.contains(requestHandle.name)))
            {
                XMLReader docReader(returnBuffer.get());
                openReadError = docReader.readInto();

                // Clear file to prepare for writing
                returnBuffer->clearFile();
            }

            // Add handle to lease set if no error occured while readding
            if(openReadError.isValid())
                returnBuffer = std::unique_ptr<Install::XMLDoc>();
            else
                mLeasedHandles.insert(requestHandle);
        }
        else
            openReadError = Qx::XmlStreamReaderError(populateErrorWithTarget(XMLReader::ERR_DOC_CANT_OPEN, requestHandle).arg(xmlFile->errorString()));
    }

    // Return new handle
    return openReadError;
}

bool Install::saveXMLDocument(QString& errorMessage, std::unique_ptr<XMLDoc> document)
{
    // Prepare writer
    XMLWriter docWriter(document.get());

    // Write to file
    errorMessage = docWriter.writeOutOf();

    // Close document file
    document->mDocumentFile->close();

    // Set document perfmissions
    allowUserWriteOnXML(document->mDocumentFile->fileName());

    // Remove handle reservation
    mLeasedHandles.remove(document->getHandleTarget());

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return errorMessage.isNull();
}

bool Install::ensureImageDirectories(QString& errorMessage,QString platform)
{
    // Ensure error message is null
    errorMessage = QString();

    QDir logoDir(mPlatformImagesDirectory.absolutePath() + '/' + platform + '/' + LOGO_PATH);
    QDir screenshotDir(mPlatformImagesDirectory.absolutePath() + '/' + platform + '/' + SCREENSHOT_PATH);

    if(!logoDir.mkpath(".")) // "." -> Make directory at its current path (no extra sub-folders)
    {
        errorMessage = ERR_CANT_MAKE_DIR.arg(logoDir.absolutePath());
        return false;
    }

    if(!screenshotDir.mkpath("."))
    {
        errorMessage = ERR_CANT_MAKE_DIR.arg(screenshotDir.absolutePath());
        return false;
    }

    // Directories are present
    return true;
}

bool Install::transferLogo(QString& errorMessage, ImageMode imageOption, QDir logoSourceDir, const LB::Game& game)
{
    errorMessage = transferImage(imageOption, logoSourceDir, LOGO_PATH, game);
    return errorMessage.isNull();
}

bool Install::transferScreenshot(QString& errorMessage, ImageMode imageOption, QDir screenshotSourceDir, const LB::Game& game)
{
    errorMessage = transferImage(imageOption, screenshotSourceDir, SCREENSHOT_PATH, game);
    return errorMessage.isNull();
}

int Install::revertNextChange(QString& errorMessage, bool skipOnFail)
{
    // Ensure error message is null
    errorMessage = QString();

    // Get operation count for return
    int operationsLeft = mModifiedXMLDocuments.size() + mPurgableImages.size() + mLinksToReverse.size();

    // Delete new XML files and restore backups if present
    if(!mModifiedXMLDocuments.isEmpty())
    {
        QString currentDoc = mModifiedXMLDocuments.front();

        QFileInfo currentDocInfo(currentDoc);
        QString backupPath = currentDocInfo.absolutePath() + '/' + currentDocInfo.baseName() + MODIFIED_FILE_EXT;

        if(currentDocInfo.exists() && !QFile::remove(currentDoc) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_REMOVE_XML.arg(currentDoc);
            return operationsLeft;
        }

        if(!QFile::exists(currentDoc) && QFile::exists(backupPath) && !QFile::rename(backupPath, currentDoc) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_RESTORE_EXML.arg(backupPath);
            return operationsLeft;
        }

        // Remove entry on success
        mModifiedXMLDocuments.removeFirst();
        return operationsLeft - 1;
    }

    // Revert regular image changes
    if(!mPurgableImages.isEmpty())
    {
        QString currentImage = mPurgableImages.front();

        if(!QFile::remove(currentImage) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_REMOVE_IMAGE.arg(currentImage);
            return operationsLeft;
        }

        // Remove entry on success
        mPurgableImages.removeFirst();
        return operationsLeft - 1;
    }

    // Revert FP links
    if(!mLinksToReverse.isEmpty())
    {
        QString currentLink = mLinksToReverse.firstKey();
        QString curerntOriginal = mLinksToReverse.first();

        if(QFile::exists(currentLink) && !QFile::remove(currentLink) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_REMOVE_IMAGE.arg(currentLink);
            return operationsLeft;
        }

        if(!QFile::exists(currentLink) && !QFile::rename(curerntOriginal, currentLink) && !skipOnFail)
        {
            errorMessage = ERR_REVERT_CANT_MOVE_IMAGE.arg(curerntOriginal);
            return operationsLeft;
        }

        // Remove entry on success
        mLinksToReverse.remove(mLinksToReverse.firstKey());
        return operationsLeft - 1;
    }

    // Return 0 if all empty (shouldn't be reached if function is used correctly)
    return 0;
}

void Install::softReset()
{
    mModifiedXMLDocuments.clear();
    mPurgableImages.clear();
    mLinksToReverse.clear();
    mLBDatabaseIDTracker = Qx::FreeIndexTracker<int>(0, -1);
}

QString Install::getPath() const { return mRootDirectory.absolutePath(); }
QSet<QString> Install::getExistingPlatforms() const { return mExistingPlatforms; }
QSet<QString> Install::getExistingPlaylists() const { return mExistingPlaylists; }
int Install::getRevertQueueCount() const { return mModifiedXMLDocuments.size() + mPurgableImages.size() + mLinksToReverse.size(); }

}
