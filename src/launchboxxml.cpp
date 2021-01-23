#include "launchboxxml.h"

namespace LB
{

//===============================================================================================================
// Xml::DataDocHandle
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const Xml::DataDocHandle& lhs, const Xml::DataDocHandle& rhs) noexcept
{
    return lhs.docType == rhs.docType && lhs.docName == rhs.docName;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const Xml::DataDocHandle& key, uint seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.docType);
    seed = hash(seed, key.docName);

    return seed;
}

//===============================================================================================================
// Xml::DataDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::DataDoc::DataDoc(std::unique_ptr<QFile> xmlFile, DataDocHandle handle)
    : mDocumentFile(std::move(xmlFile)), mHandleTarget(handle) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Xml::DataDocHandle Xml::DataDoc::getHandleTarget() const { return mHandleTarget; }
void Xml::DataDoc::clearFile() { mDocumentFile->resize(0); }

//===============================================================================================================
// Xml::DataDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::DataDocReader::DataDocReader() {}

//===============================================================================================================
// Xml::DataDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::DataDocWriter::DataDocWriter() {}

//===============================================================================================================
// Xml::Platform
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::Platform::Platform(std::unique_ptr<QFile> xmlFile, QString docName, UpdateOptions updateOptions, const Key&)
    : DataDoc(std::move(xmlFile), DataDocHandle{TYPE_NAME, docName}), mUpdateOptions(updateOptions) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const QHash<QUuid, Game>& Xml::Platform::getFinalGames() const { return mGamesFinal; }
const QHash<QUuid, AddApp>& Xml::Platform::getFinalAddApps() const { return mAddAppsFinal; }

bool Xml::Platform::containsGame(QUuid gameID) const { return mGamesFinal.contains(gameID) || mGamesExisting.contains(gameID); }
bool Xml::Platform::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

void Xml::Platform::addGame(Game game)
{
    QUuid key = game.getID();

    // Check if game exists
    if(mGamesExisting.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
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

void Xml::Platform::addAddApp(AddApp app)
{
    QUuid key = app.getID();

    // Check if add app exists
    if(mAddAppsExisting.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
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

void Xml::Platform::finalize()
{
    // Copy items to final list if obsolete entries are to be kept
    if(!mUpdateOptions.removeObsolete)
    {
        mGamesFinal.insert(mGamesExisting);
        mAddAppsFinal.insert(mAddAppsExisting);
    }

    // Clear existing lists
    mGamesExisting.clear();
    mAddAppsExisting.clear();
}

//===============================================================================================================
// Xml::PlatformReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformReader::PlatformReader(Platform* targetDoc)
    : mTargetDocument(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:

Qx::XmlStreamReaderError Xml::PlatformReader::readInto()
{
    // Hook reader to document handle
    mStreamReader.setDevice(mTargetDocument->mDocumentFile.get());

    // Prepare error return instance
    Qx::XmlStreamReaderError readError;

    if(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XML_ROOT_ELEMENT)
            readError = readPlatformDoc();
        else
            readError = Qx::XmlStreamReaderError(formatDataDocError(ERR_NOT_LB_DOC, mTargetDocument->mHandleTarget));
    }
    else
        readError = Qx::XmlStreamReaderError(mStreamReader.error());

    return readError;
}

//Private:
Qx::XmlStreamReaderError Xml::PlatformReader::readPlatformDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_Game::NAME)
            parseGame();
        else if(mStreamReader.name() == Element_AddApp::NAME)
            parseAddApp();
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

void Xml::PlatformReader::parseGame()
{
    // Game to build
    GameBuilder gb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_Game::ELEMENT_ID)
            gb.wID(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_TITLE)
            gb.wTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_SERIES)
            gb.wSeries(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_DEVELOPER)
            gb.wDeveloper(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_PUBLISHER)
            gb.wPublisher(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_PLATFORM)
            gb.wPlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_SORT_TITLE)
            gb.wSortTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_DATE_ADDED)
            gb.wDateAdded(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_DATE_MODIFIED)
            gb.wDateModified(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_BROKEN)
            gb.wBroken(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_PLAYMODE)
            gb.wPlayMode(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_STATUS)
            gb.wStatus(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_REGION)
            gb.wRegion(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_NOTES)
            gb.wNotes(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_SOURCE)
            gb.wSource(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_APP_PATH)
            gb.wAppPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_COMMAND_LINE)
            gb.wCommandLine(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_RELEASE_DATE)
            gb.wReleaseDate(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_VERSION)
            gb.wVersion(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_Game::ELEMENT_RELEASE_TYPE)
            gb.wReleaseType(mStreamReader.readElementText());
        else
            gb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Game and add to document
    LB::Game existingGame = gb.build();
    mTargetDocument->mGamesExisting[existingGame.getID()] = existingGame;
}

void Xml::PlatformReader::parseAddApp()
{
    // Additional App to Build
    AddAppBuilder aab;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_AddApp::ELEMENT_ID)
            aab.wID(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_AddApp::ELEMENT_GAME_ID)
            aab.wGameID(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_AddApp::ELEMENT_APP_PATH)
            aab.wAppPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_AddApp::ELEMENT_COMMAND_LINE)
            aab.wCommandLine(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_AddApp::ELEMENT_AUTORUN_BEFORE)
            aab.wAutorunBefore(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_AddApp::ELEMENT_NAME)
            aab.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_AddApp::ELEMENT_WAIT_FOR_EXIT)
            aab.wWaitForExit(mStreamReader.readElementText());
        else
            aab.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Additional App and add to document
    LB::AddApp existingAddApp = aab.build();
    mTargetDocument->mAddAppsExisting[existingAddApp.getID()] = existingAddApp;

}

//===============================================================================================================
// Xml::PlatformWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformWriter::PlatformWriter(Platform* sourceDoc)
    : mSourceDocument(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
QString Xml::PlatformWriter::writeOutOf()
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
    if(!writePlatformDoc())
        return mStreamWriter.device()->errorString();

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return QString();
}

//Private:
bool Xml::PlatformWriter::writePlatformDoc()
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

    // Return true on success
    return true;
}

bool Xml::PlatformWriter::writeGame(const Game& game)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_Game::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_ID, game.getID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_TITLE, game.getTitle());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_SERIES, game.getSeries());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_DEVELOPER, game.getDeveloper());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_PUBLISHER, game.getPublisher());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_PLATFORM, game.getPlatform());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_SORT_TITLE, game.getSortTitle());

    if(game.getDateAdded().isValid()) // LB is picky with dates
        mStreamWriter.writeTextElement(Element_Game::ELEMENT_DATE_ADDED, game.getDateAdded().toString(Qt::ISODateWithMs));

    if(game.getDateModified().isValid())// LB is picky with dates
        mStreamWriter.writeTextElement(Element_Game::ELEMENT_DATE_MODIFIED, game.getDateModified().toString(Qt::ISODateWithMs));

    mStreamWriter.writeTextElement(Element_Game::ELEMENT_BROKEN, game.isBroken() ? "true" : "false");
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_PLAYMODE, game.getPlayMode());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_STATUS, game.getStatus());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_REGION, game.getRegion());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_NOTES, game.getNotes());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_SOURCE, game.getSource());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_APP_PATH, game.getAppPath());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_COMMAND_LINE, game.getCommandLine());

    if(game.getReleaseDate().isValid()) // LB is picky with dates
        mStreamWriter.writeTextElement(Element_Game::ELEMENT_RELEASE_DATE, game.getReleaseDate().toString(Qt::ISODateWithMs));

    mStreamWriter.writeTextElement(Element_Game::ELEMENT_VERSION, game.getVersion());
    mStreamWriter.writeTextElement(Element_Game::ELEMENT_RELEASE_TYPE, game.getReleaseType());

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

bool Xml::PlatformWriter::writeAddApp(const AddApp& addApp)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_AddApp::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_ID, addApp.getID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_GAME_ID, addApp.getGameID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_APP_PATH, addApp.getAppPath());
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_COMMAND_LINE, addApp.getCommandLine());
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_AUTORUN_BEFORE, addApp.isAutorunBefore() ? "true" : "false");
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_NAME, addApp.getName());
    mStreamWriter.writeTextElement(Element_AddApp::ELEMENT_WAIT_FOR_EXIT, addApp.isWaitForExit() ? "true" : "false");

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

//===============================================================================================================
// Xml::Playlist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::Playlist::Playlist(std::unique_ptr<QFile> xmlFile, QString docName, UpdateOptions updateOptions, Qx::FreeIndexTracker<int>* lbDBFIDT, const Key&)
    : DataDoc(std::move(xmlFile), DataDocHandle{Xml::Playlist::TYPE_NAME, docName}), mUpdateOptions(updateOptions), mPlaylistGameFreeLBDBIDTracker(lbDBFIDT) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const PlaylistHeader& Xml::Playlist::getPlaylistHeader() const { return mPlaylistHeader; }
const QHash<QUuid, PlaylistGame>& Xml::Playlist::getFinalPlaylistGames() const { return mPlaylistGamesFinal; }

bool Xml::Playlist::containsPlaylistGame(QUuid gameID) const { return mPlaylistGamesFinal.contains(gameID) || mPlaylistGamesExisting.contains(gameID); }


void Xml::Playlist::setPlaylistHeader(PlaylistHeader header)
{
    header.transferOtherFields(mPlaylistHeader.getOtherFields());
    mPlaylistHeader = header;
}

void Xml::Playlist::addPlaylistGame(PlaylistGame playlistGame)
{
    QUuid key = playlistGame.getGameID();

    // Check if playlist game exists
    if(mPlaylistGamesExisting.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
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

void Xml::Playlist::finalize()
{
    // Copy items to final list if obsolete entries are to be kept
    if(!mUpdateOptions.removeObsolete)
        mPlaylistGamesFinal.insert(mPlaylistGamesExisting);

    // Clear existing lists
    mPlaylistGamesExisting.clear();
}

//===============================================================================================================
// Xml::PlaylistReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlaylistReader::PlaylistReader(Playlist* targetDoc)
    : mTargetDocument(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::XmlStreamReaderError Xml::PlaylistReader::readInto()
{
    // Hook reader to document handle
    mStreamReader.setDevice(mTargetDocument->mDocumentFile.get());

    // Prepare error return instance
    Qx::XmlStreamReaderError readError;

    if(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XML_ROOT_ELEMENT)
            readError = readPlaylistDoc();
        else
            readError = Qx::XmlStreamReaderError(formatDataDocError(ERR_NOT_LB_DOC, mTargetDocument->mHandleTarget));
    }
    else
        readError = Qx::XmlStreamReaderError(mStreamReader.error());

    return readError;
}

//Private:
Qx::XmlStreamReaderError Xml::PlaylistReader::readPlaylistDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_PlaylistHeader::NAME)
            parsePlaylistHeader();
        else if(mStreamReader.name() == Element_PlaylistGame::NAME)
            parsePlaylistGame();
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

void Xml::PlaylistReader::parsePlaylistHeader()
{
    // Playlist Header to Build
    PlaylistHeaderBuilder phb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_PlaylistHeader::ELEMENT_ID)
            phb.wPlaylistID(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistHeader::ELEMENT_NAME)
            phb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistHeader::ELEMENT_NESTED_NAME)
            phb.wNestedName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistHeader::ELEMENT_NOTES)
            phb.wNotes(mStreamReader.readElementText());
        else
            phb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Header and add to document
    mTargetDocument->mPlaylistHeader = phb.build();

}

void Xml::PlaylistReader::parsePlaylistGame()
{
    // Playlist Game to Build
    PlaylistGameBuilder pgb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_PlaylistGame::ELEMENT_ID)
            pgb.wGameID(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistGame::ELEMENT_GAME_TITLE)
            pgb.wGameTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistGame::ELEMENT_GAME_FILE_NAME)
            pgb.wGameFileName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistGame::ELEMENT_GAME_PLATFORM)
            pgb.wGamePlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistGame::ELEMENT_MANUAL_ORDER)
            pgb.wManualOrder(mStreamReader.readElementText());
        else if(mStreamReader.name() == Element_PlaylistGame::ELEMENT_LB_DB_ID)
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
// Xml::PlaylistWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlaylistWriter::PlaylistWriter(Playlist* sourceDoc)
    : mSourceDocument(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
QString Xml::PlaylistWriter::writeOutOf()
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
    if(!writePlaylistDoc())
        return mStreamWriter.device()->errorString();

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return QString();
}

//Private:
bool Xml::PlaylistWriter::writePlaylistDoc()
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

    // Return true on success
    return true;
}

bool Xml::PlaylistWriter::writePlaylistHeader(const PlaylistHeader& playlistHeader)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_PlaylistHeader::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(Element_PlaylistHeader::ELEMENT_ID, playlistHeader.getPlaylistID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(Element_PlaylistHeader::ELEMENT_NAME, playlistHeader.getName());
    mStreamWriter.writeTextElement(Element_PlaylistHeader::ELEMENT_NESTED_NAME, playlistHeader.getNestedName());
    mStreamWriter.writeTextElement(Element_PlaylistHeader::ELEMENT_NOTES, playlistHeader.getNotes());

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

bool Xml::PlaylistWriter::writePlaylistGame(const PlaylistGame& playlistGame)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_PlaylistGame::NAME);

    // Write known tags
    mStreamWriter.writeTextElement(Element_PlaylistGame::ELEMENT_ID, playlistGame.getGameID().toString(QUuid::WithoutBraces));
    mStreamWriter.writeTextElement(Element_PlaylistGame::ELEMENT_GAME_TITLE, playlistGame.getGameTitle());
    mStreamWriter.writeTextElement(Element_PlaylistGame::ELEMENT_GAME_PLATFORM, playlistGame.getGamePlatform());
    mStreamWriter.writeTextElement(Element_PlaylistGame::ELEMENT_MANUAL_ORDER, QString::number(playlistGame.getManualOrder()));
    mStreamWriter.writeTextElement(Element_PlaylistGame::ELEMENT_LB_DB_ID, QString::number(playlistGame.getLBDatabaseID()));

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
// Xml
//===============================================================================================================

//-Class Functions----------------------------------------------------------------------------------------------------
//Public:
QString Xml::formatDataDocError(QString errorTemplate, DataDocHandle docHandle)
{
    return errorTemplate.arg(docHandle.docType).arg(docHandle.docName);
}


}
