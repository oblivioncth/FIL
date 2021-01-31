#include "launchbox-xml.h"

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
Xml::DataDocReader::DataDocReader(Xml::DataDoc* targetDoc) : mTargetDocument(targetDoc) {}

Qx::XmlStreamReaderError Xml::DataDocReader::readInto()
{
    // Hook reader to document handle
    mStreamReader.setDevice(mTargetDocument->mDocumentFile.get());

    // Prepare error return instance
    Qx::XmlStreamReaderError readError;

    if(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == XML_ROOT_ELEMENT)
        {
            // Return no error on success
            if(!readTargetDoc())
            {
                if(mStreamReader.error() == QXmlStreamReader::CustomError)
                    return Qx::XmlStreamReaderError(mStreamReader.errorString());
                else
                    return Qx::XmlStreamReaderError(mStreamReader.error());
            }
            else
                return Qx::XmlStreamReaderError();
        }
        else
            readError = Qx::XmlStreamReaderError(formatDataDocError(ERR_NOT_LB_DOC, mTargetDocument->mHandleTarget));
    }
    else
        readError = Qx::XmlStreamReaderError(mStreamReader.error());

    return readError;
}

//===============================================================================================================
// Xml::DataDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::DataDocWriter::DataDocWriter(DataDoc* sourceDoc)
    : mSourceDocument(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
QString Xml::DataDocWriter::writeOutOf()
{
    // Hook writer to document handle
    mStreamWriter.setDevice(mSourceDocument->mDocumentFile.get());

    // Enable auto formating
    mStreamWriter.setAutoFormatting(true);
    mStreamWriter.setAutoFormattingIndent(2);

    // Write standard XML header
    mStreamWriter.writeStartDocument("1.0", true);

    // Write main LaunchBox tag
    mStreamWriter.writeStartElement(XML_ROOT_ELEMENT);

    // Write main body
    if(!writeSourceDoc())
        return mStreamWriter.device()->errorString();

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return QString();
}

void Xml::DataDocWriter::writeEmptyCheckedTextElement(const QString& qualifiedName, const QString& text)
{
    if(text.isEmpty())
        mStreamWriter.writeEmptyElement(qualifiedName);
    else
        mStreamWriter.writeTextElement(qualifiedName, text);
}

void Xml::DataDocWriter::writeOtherFields(const QHash<QString, QString>& otherFields)
{
    for(QHash<QString, QString>::const_iterator i = otherFields.constBegin(); i != otherFields.constEnd(); ++i)
        writeEmptyCheckedTextElement(i.key(), i.value());
}

//===============================================================================================================
// Xml::ConfigDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::ConfigDoc::ConfigDoc(std::unique_ptr<QFile> xmlFile, DataDocHandle handle)
    : DataDoc(std::move(xmlFile), handle) {}

//===============================================================================================================
// Xml::PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformDoc::PlatformDoc(std::unique_ptr<QFile> xmlFile, QString docName, UpdateOptions updateOptions, const Key&)
    : DataDoc(std::move(xmlFile), DataDocHandle{TYPE_NAME, docName}), mUpdateOptions(updateOptions) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const QHash<QUuid, Game>& Xml::PlatformDoc::getFinalGames() const { return mGamesFinal; }
const QHash<QUuid, AddApp>& Xml::PlatformDoc::getFinalAddApps() const { return mAddAppsFinal; }

bool Xml::PlatformDoc::containsGame(QUuid gameID) const { return mGamesFinal.contains(gameID) || mGamesExisting.contains(gameID); }
bool Xml::PlatformDoc::containsAddApp(QUuid addAppId) const { return mAddAppsFinal.contains(addAppId) || mAddAppsExisting.contains(addAppId); }

void Xml::PlatformDoc::addGame(Game game)
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

void Xml::PlatformDoc::addAddApp(AddApp app)
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

void Xml::PlatformDoc::finalize()
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
// Xml::PlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformDocReader::PlatformDocReader(PlatformDoc* targetDoc)
    : DataDocReader(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool Xml::PlatformDocReader::readTargetDoc()
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

    // Return status
    return mStreamReader.hasError();
}

void Xml::PlatformDocReader::parseGame()
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
    static_cast<PlatformDoc*>(mTargetDocument)->mGamesExisting[existingGame.getID()] = existingGame;
}

void Xml::PlatformDocReader::parseAddApp()
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
    static_cast<PlatformDoc*>(mTargetDocument)->mAddAppsExisting[existingAddApp.getID()] = existingAddApp;

}

//===============================================================================================================
// Xml::PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformDocWriter::PlatformDocWriter(PlatformDoc* sourceDoc)
    : DataDocWriter(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool Xml::PlatformDocWriter::writeSourceDoc()
{
    // Write all games
    for(const Game& game : static_cast<PlatformDoc*>(mSourceDocument)->getFinalGames())
    {
        if(!writeGame(game))
            return false;
    }

    // Write all additional apps
    for(const AddApp& addApp : static_cast<PlatformDoc*>(mSourceDocument)->getFinalAddApps())
    {
        if(!writeAddApp(addApp))
            return false;
    }

    // Return true on success
    return true;
}

bool Xml::PlatformDocWriter::writeGame(const Game& game)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_Game::NAME);

    // Write known tags
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_ID, game.getID().toString(QUuid::WithoutBraces));
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_TITLE, game.getTitle());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_SERIES, game.getSeries());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_DEVELOPER, game.getDeveloper());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_PUBLISHER, game.getPublisher());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_PLATFORM, game.getPlatform());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_SORT_TITLE, game.getSortTitle());

    if(game.getDateAdded().isValid()) // LB is picky with dates
        writeEmptyCheckedTextElement(Element_Game::ELEMENT_DATE_ADDED, game.getDateAdded().toString(Qt::ISODateWithMs));

    if(game.getDateModified().isValid())// LB is picky with dates
        writeEmptyCheckedTextElement(Element_Game::ELEMENT_DATE_MODIFIED, game.getDateModified().toString(Qt::ISODateWithMs));

    writeEmptyCheckedTextElement(Element_Game::ELEMENT_BROKEN, game.isBroken() ? "true" : "false");
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_PLAYMODE, game.getPlayMode());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_STATUS, game.getStatus());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_REGION, game.getRegion());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_NOTES, game.getNotes());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_SOURCE, game.getSource());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_APP_PATH, game.getAppPath());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_COMMAND_LINE, game.getCommandLine());

    if(game.getReleaseDate().isValid()) // LB is picky with dates
        writeEmptyCheckedTextElement(Element_Game::ELEMENT_RELEASE_DATE, game.getReleaseDate().toString(Qt::ISODateWithMs));

    writeEmptyCheckedTextElement(Element_Game::ELEMENT_VERSION, game.getVersion());
    writeEmptyCheckedTextElement(Element_Game::ELEMENT_RELEASE_TYPE, game.getReleaseType());

    // Write other tags
    writeOtherFields(game.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
}

bool Xml::PlatformDocWriter::writeAddApp(const AddApp& addApp)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_AddApp::NAME);

    // Write known tags
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_ID, addApp.getID().toString(QUuid::WithoutBraces));
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_GAME_ID, addApp.getGameID().toString(QUuid::WithoutBraces));
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_APP_PATH, addApp.getAppPath());
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_COMMAND_LINE, addApp.getCommandLine());
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_AUTORUN_BEFORE, addApp.isAutorunBefore() ? "true" : "false");
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_NAME, addApp.getName());
    writeEmptyCheckedTextElement(Element_AddApp::ELEMENT_WAIT_FOR_EXIT, addApp.isWaitForExit() ? "true" : "false");

    // Write other tags
    writeOtherFields(addApp.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
}

//===============================================================================================================
// Xml::PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlaylistDoc::PlaylistDoc(std::unique_ptr<QFile> xmlFile, QString docName, UpdateOptions updateOptions, Qx::FreeIndexTracker<int>* lbDBFIDT, const Key&)
    : DataDoc(std::move(xmlFile), DataDocHandle{Xml::PlaylistDoc::TYPE_NAME, docName}), mUpdateOptions(updateOptions), mPlaylistGameFreeLBDBIDTracker(lbDBFIDT) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const PlaylistHeader& Xml::PlaylistDoc::getPlaylistHeader() const { return mPlaylistHeader; }
const QHash<QUuid, PlaylistGame>& Xml::PlaylistDoc::getFinalPlaylistGames() const { return mPlaylistGamesFinal; }

bool Xml::PlaylistDoc::containsPlaylistGame(QUuid gameID) const { return mPlaylistGamesFinal.contains(gameID) || mPlaylistGamesExisting.contains(gameID); }


void Xml::PlaylistDoc::setPlaylistHeader(PlaylistHeader header)
{
    header.transferOtherFields(mPlaylistHeader.getOtherFields());
    mPlaylistHeader = header;
}

void Xml::PlaylistDoc::addPlaylistGame(PlaylistGame playlistGame)
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

void Xml::PlaylistDoc::finalize()
{
    // Copy items to final list if obsolete entries are to be kept
    if(!mUpdateOptions.removeObsolete)
        mPlaylistGamesFinal.insert(mPlaylistGamesExisting);

    // Clear existing lists
    mPlaylistGamesExisting.clear();
}

//===============================================================================================================
// Xml::PlaylistDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlaylistDocReader::PlaylistDocReader(PlaylistDoc* targetDoc) : DataDocReader(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool Xml::PlaylistDocReader::readTargetDoc()
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

    // Return status
    return mStreamReader.hasError();
}

void Xml::PlaylistDocReader::parsePlaylistHeader()
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
    static_cast<PlaylistDoc*>(mTargetDocument)->mPlaylistHeader = phb.build();

}

void Xml::PlaylistDocReader::parsePlaylistGame()
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
        existingPlaylistGame.setLBDatabaseID(static_cast<PlaylistDoc*>(mTargetDocument)->mPlaylistGameFreeLBDBIDTracker->reserveFirstFree());
    else
        static_cast<PlaylistDoc*>(mTargetDocument)->mPlaylistGameFreeLBDBIDTracker->release(existingPlaylistGame.getLBDatabaseID());

    // Add to document
    static_cast<PlaylistDoc*>(mTargetDocument)->mPlaylistGamesExisting[existingPlaylistGame.getGameID()] = existingPlaylistGame;
}



//===============================================================================================================
// Xml::PlaylistDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlaylistDocWriter::PlaylistDocWriter(PlaylistDoc* sourceDoc)
    : DataDocWriter(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool Xml::PlaylistDocWriter::writeSourceDoc()
{
    // Write playlist header
    if(!writePlaylistHeader(static_cast<PlaylistDoc*>(mSourceDocument)->getPlaylistHeader()))
        return false;

    // Write all playlist games
    for(const PlaylistGame& playlistGame : static_cast<PlaylistDoc*>(mSourceDocument)->getFinalPlaylistGames())
    {
        if(!writePlaylistGame(playlistGame))
            return false;
    }

    // Return true on success
    return true;
}

bool Xml::PlaylistDocWriter::writePlaylistHeader(const PlaylistHeader& playlistHeader)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_PlaylistHeader::NAME);

    // Write known tags
    writeEmptyCheckedTextElement(Element_PlaylistHeader::ELEMENT_ID, playlistHeader.getPlaylistID().toString(QUuid::WithoutBraces));
    writeEmptyCheckedTextElement(Element_PlaylistHeader::ELEMENT_NAME, playlistHeader.getName());
    writeEmptyCheckedTextElement(Element_PlaylistHeader::ELEMENT_NESTED_NAME, playlistHeader.getNestedName());
    writeEmptyCheckedTextElement(Element_PlaylistHeader::ELEMENT_NOTES, playlistHeader.getNotes());

    // Write other tags
    writeOtherFields(playlistHeader.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
}

bool Xml::PlaylistDocWriter::writePlaylistGame(const PlaylistGame& playlistGame)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_PlaylistGame::NAME);

    // Write known tags
    writeEmptyCheckedTextElement(Element_PlaylistGame::ELEMENT_ID, playlistGame.getGameID().toString(QUuid::WithoutBraces));
    writeEmptyCheckedTextElement(Element_PlaylistGame::ELEMENT_GAME_TITLE, playlistGame.getGameTitle());
    writeEmptyCheckedTextElement(Element_PlaylistGame::ELEMENT_GAME_PLATFORM, playlistGame.getGamePlatform());
    writeEmptyCheckedTextElement(Element_PlaylistGame::ELEMENT_MANUAL_ORDER, QString::number(playlistGame.getManualOrder()));
    writeEmptyCheckedTextElement(Element_PlaylistGame::ELEMENT_LB_DB_ID, QString::number(playlistGame.getLBDatabaseID()));

    // Write other tags
    writeOtherFields(playlistGame.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
}

//===============================================================================================================
// Xml::PlatformsDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformsDoc::PlatformsDoc(std::unique_ptr<QFile> xmlFile, const Key&)
    : ConfigDoc(std::move(xmlFile), DataDocHandle{Xml::PlatformsDoc::TYPE_NAME, Xml::PlatformsDoc::STD_NAME}) {}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
const QHash<QString, Platform>& Xml::PlatformsDoc::getPlatforms() const { return mPlatforms; }
const QMap<QString, QMap<QString, QString>>& Xml::PlatformsDoc::getPlatformFolders() const { return mPlatformFolders; }
const QList<PlatformCategory>& Xml::PlatformsDoc::getPlatformCategories() const { return mPlatformCategories; }

bool Xml::PlatformsDoc::containsPlatform(QString name) { return mPlatforms.contains(name); }

void Xml::PlatformsDoc::addPlatform(Platform platform) { mPlatforms[platform.getName()] = platform; }

void Xml::PlatformsDoc::setMediaFolder(QString platform, QString mediaType, QString folderPath)
{
    mPlatformFolders[platform][mediaType] = folderPath;
}

//===============================================================================================================
// Xml::PlatformsDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformsDocReader::PlatformsDocReader(Xml::PlatformsDoc* targetDoc)
    : DataDocReader(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool Xml::PlatformsDocReader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_Platform::NAME)
            parsePlatform();
        else if(mStreamReader.name() == Element_PlatformFolder::NAME)
            parsePlatformFolder();
        else if (mStreamReader.name() == Element_PlatformCategory::NAME)
            parsePlatformCategory();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return mStreamReader.hasError();
}

void Xml::PlatformsDocReader::parsePlatform()
{
    // Platform Config Doc to Build
    PlatformBuilder pb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_Platform::ELEMENT_NAME)
            pb.wName(mStreamReader.readElementText());
        else
            pb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Platform and add to document
    LB::Platform existingPlatform = pb.build();
   static_cast<PlatformsDoc*>(mTargetDocument)->mPlatforms.insert(existingPlatform.getName(), existingPlatform);
}

void Xml::PlatformsDocReader::parsePlatformFolder()
{
    // Platform Folder to Build
    QString platform;
    QString mediaType;
    QString folderPath;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Element_PlatformFolder::ELEMENT_MEDIA_TYPE)
            mediaType = mStreamReader.readElementText();
        else if(mStreamReader.name() == Element_PlatformFolder::ELEMENT_FOLDER_PATH)
            folderPath = mStreamReader.readElementText();
        else if(mStreamReader.name() == Element_PlatformFolder::ELEMENT_PLATFORM)
            platform = mStreamReader.readElementText();
        else
            mStreamReader.raiseError(formatDataDocError(ERR_DOC_TYPE_MISMATCH, mTargetDocument->getHandleTarget()));
    }

    // Add to document
    static_cast<PlatformsDoc*>(mTargetDocument)->mPlatformFolders[platform][mediaType] = folderPath;
}

void Xml::PlatformsDocReader::parsePlatformCategory()
{
    // Platform Config Doc to Build
    PlatformCategoryBuilder pcb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        // No specific elements are of interest for now
        pcb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Header and add to document
   static_cast<PlatformsDoc*>(mTargetDocument)->mPlatformCategories.append(pcb.build());
}

//===============================================================================================================
// Xml::PlatformsDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Xml::PlatformsDocWriter::PlatformsDocWriter(PlatformsDoc* sourceDoc)
    : DataDocWriter(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool Xml::PlatformsDocWriter::writeSourceDoc()
{
    // Write all platforms
    for(const Platform& platform : static_cast<PlatformsDoc*>(mSourceDocument)->getPlatforms())
    {
        if(!writePlatform(platform))
            return false;
    }

    // Write all platform folders
    const QMap<QString, QMap<QString, QString>>& platformFolderMap = static_cast<PlatformsDoc*>(mSourceDocument)->getPlatformFolders();
    QMap<QString, QMap<QString, QString>>::const_iterator i;
    for(i = platformFolderMap.constBegin(); i != platformFolderMap.constEnd(); i++)
    {
         QMap<QString, QString>::const_iterator j;
         for(j = i.value().constBegin(); j != i.value().constEnd(); j++)
             if(!writePlatformFolder(i.key(), j.key(), j.value()))
                 return false;
    }

    // Write all platform categories
    for(const PlatformCategory& platformCategory : static_cast<PlatformsDoc*>(mSourceDocument)->getPlatformCategories())
    {
        if(!writePlatformCategory(platformCategory))
            return false;
    }

    // Return true on success
    return true;
}

bool Xml::PlatformsDocWriter::writePlatform(const Platform& platform)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_Platform::NAME);

    // Write known tags
    writeEmptyCheckedTextElement(Element_Platform::ELEMENT_NAME, platform.getName());

    // Write other tags
    writeOtherFields(platform.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
}

bool Xml::PlatformsDocWriter::writePlatformFolder(const QString& platform, const QString& mediaType, const QString& folderPath)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_PlatformFolder::NAME);

    // Write known tags
    writeEmptyCheckedTextElement(Element_PlatformFolder::ELEMENT_MEDIA_TYPE, mediaType);
    writeEmptyCheckedTextElement(Element_PlatformFolder::ELEMENT_FOLDER_PATH, folderPath);
    writeEmptyCheckedTextElement(Element_PlatformFolder::ELEMENT_PLATFORM, platform);

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
}

bool Xml::PlatformsDocWriter::writePlatformCategory(const PlatformCategory& platformCategory)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Element_PlatformCategory::NAME);

    // Write known tags
    // None for now...
    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    writeOtherFields(platformCategory.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return mStreamWriter.hasError();
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
