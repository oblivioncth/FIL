// Unit Include
#include "lb-data.h"

// Standard Library Includes
#include <memory>

// Qx Includes
#include <qx/xml/qx-xmlstreamreadererror.h>
#include <qx/xml/qx-common-xml.h>

// Project Includes
#include "lb-install.h"


namespace Lb
{
//===============================================================================================================
// XmlDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
XmlDocReader::XmlDocReader(Fe::DataDoc* targetDoc) : Fe::DataDocReader(targetDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError XmlDocReader::readInto()
{
    // Hook reader to document handle
    mStreamReader.setDevice(targetDocFile().get());

    // Prepare error return instance
    Qx::XmlStreamReaderError readError;

    if(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::ROOT_ELEMENT)
        {
            // Return no error on success
            if(!readTargetDoc())
            {
                if(mStreamReader.error() == QXmlStreamReader::CustomError)
                    readError = Qx::XmlStreamReaderError(mStreamReader.errorString());
                else
                    readError = Qx::XmlStreamReaderError(mStreamReader.error());
            }
        }
        else
            readError = Qx::XmlStreamReaderError(mTargetDocument->errorString(Fe::DataDoc::StandardError::NotParentDoc));
    }
    else
        readError = Qx::XmlStreamReaderError(mStreamReader.error());

    return readError.isValid() ? Qx::GenericError(Qx::GenericError::Critical, mPrimaryError, readError.text()) :
                                 Qx::GenericError();
}

//===============================================================================================================
// XmlDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
XmlDocWriter::XmlDocWriter(Fe::DataDoc* sourceDoc) : Fe::DataDocWriter(sourceDoc) {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
void XmlDocWriter::writeCleanTextElement(const QString& qualifiedName, const QString& text)
{
    if(text.isEmpty())
        mStreamWriter.writeEmptyElement(qualifiedName);
    else
        mStreamWriter.writeTextElement(qualifiedName, Qx::xmlSanitized(text));
}

void XmlDocWriter::writeOtherFields(const QHash<QString, QString>& otherFields)
{
    for(QHash<QString, QString>::const_iterator i = otherFields.constBegin(); i != otherFields.constEnd(); ++i)
        writeCleanTextElement(i.key(), i.value());
}

//Public:
Qx::GenericError XmlDocWriter::writeOutOf()
{
    // Hook writer to document handle
    mStreamWriter.setDevice(sourceDocFile().get());

    // Enable auto formatting
    mStreamWriter.setAutoFormatting(true);
    mStreamWriter.setAutoFormattingIndent(2);

    // Write standard XML header
    mStreamWriter.writeStartDocument("1.0", true);

    // Write main LaunchBox tag
    mStreamWriter.writeStartElement(Xml::ROOT_ELEMENT);

    // Write main body
    if(!writeSourceDoc())
        return Qx::GenericError(Qx::GenericError::Critical, mPrimaryError, mStreamWriter.device()->errorString());

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return mStreamWriter.hasError() ? Qx::GenericError(Qx::GenericError::Critical, mPrimaryError, mStreamWriter.device()->errorString()) :
                                      Qx::GenericError();
}

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::PlatformDoc(Install* const parent, std::unique_ptr<QFile> xmlFile, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&) :
    Fe::BasicPlatformDoc(parent, std::move(xmlFile), docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
std::shared_ptr<Fe::Game> PlatformDoc::prepareGame(const Fp::Game& game)
{
    // Convert to LaunchBox game
    std::shared_ptr<Game> lbGame = std::make_shared<Game>(game, parent()->linkedClifpPath());

    // Add details to cache
    static_cast<Install*>(parent())->mPlaylistGameDetailsCache.insert(game.getId(), PlaylistGame::EntryDetails(*lbGame));

    // Add language as custom field
    CustomFieldBuilder cfb;
    cfb.wGameId(game.getId());
    cfb.wName(CustomField::LANGUAGE);
    cfb.wValue(game.getLanguage());
    addCustomField(cfb.buildShared());

    // Return converted game
    return lbGame;
}

std::shared_ptr<Fe::AddApp> PlatformDoc::prepareAddApp(const Fp::AddApp& addApp)
{
    // Convert to LaunchBox add app
    std::shared_ptr<AddApp> lbAddApp = std::make_shared<AddApp>(addApp, parent()->linkedClifpPath());

    // Return converted game
    return lbAddApp;
}

//Public:

void PlatformDoc::addCustomField(std::shared_ptr<CustomField> customField)
{
    QString key = customField->getGameId().toString() + customField->getName();
    addUpdateableItem(mCustomFieldsExisting, mCustomFieldsFinal, key, customField);
}

//Public:
void PlatformDoc::finalizeDerived()
{
    // Finalize custom fields
    finalizeUpdateableItems(mCustomFieldsExisting, mCustomFieldsFinal);

    // Ensure that custom fields for removed games are deleted
    QHash<QString, std::shared_ptr<CustomField>>::iterator i = mCustomFieldsFinal.begin();
    while (i != mCustomFieldsFinal.end())
    {
        if(!getFinalGames().contains(i.value()->getGameId()))
            i = mCustomFieldsFinal.erase(i);
        else
            ++i;
    }
}

//===============================================================================================================
// PlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDocReader::PlatformDocReader(PlatformDoc* targetDoc) :
    Fe::DataDocReader(targetDoc),
    Fe::BasicPlatformDocReader(targetDoc),
    XmlDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformDocReader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Game::NAME)
            parseGame();
        else if(mStreamReader.name() == Xml::Element_AddApp::NAME)
            parseAddApp();
        else if(mStreamReader.name() == Xml::Element_CustomField::NAME)
            parseCustomField();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return !mStreamReader.hasError();
}

void PlatformDocReader::parseGame()
{
    // Game to build
    GameBuilder gb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Game::ELEMENT_ID)
            gb.wId(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_TITLE)
            gb.wTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_SERIES)
            gb.wSeries(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_DEVELOPER)
            gb.wDeveloper(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_PUBLISHER)
            gb.wPublisher(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_PLATFORM)
            gb.wPlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_SORT_TITLE)
            gb.wSortTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_DATE_ADDED)
            gb.wDateAdded(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_DATE_MODIFIED)
            gb.wDateModified(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_BROKEN)
            gb.wBroken(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_PLAYMODE)
            gb.wPlayMode(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_STATUS)
            gb.wStatus(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_REGION)
            gb.wRegion(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_NOTES)
            gb.wNotes(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_SOURCE)
            gb.wSource(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_APP_PATH)
            gb.wAppPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_COMMAND_LINE)
            gb.wCommandLine(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_RELEASE_DATE)
            gb.wReleaseDate(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_VERSION)
            gb.wVersion(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Game::ELEMENT_RELEASE_TYPE)
            gb.wReleaseType(mStreamReader.readElementText());
        else
            gb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Game and add to document
    std::shared_ptr<Game> existingGame = gb.buildShared();
    targetDocExistingGames()[existingGame->getId()] = existingGame;
}

void PlatformDocReader::parseAddApp()
{
    // Additional App to Build
    AddAppBuilder aab;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_ID)
            aab.wId(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_GAME_ID)
            aab.wGameId(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_APP_PATH)
            aab.wAppPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_COMMAND_LINE)
            aab.wCommandLine(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_AUTORUN_BEFORE)
            aab.wAutorunBefore(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_NAME)
            aab.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_AddApp::ELEMENT_WAIT_FOR_EXIT)
            aab.wWaitForExit(mStreamReader.readElementText());
        else
            aab.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Additional App and add to document
    std::shared_ptr<AddApp> existingAddApp = aab.buildShared();
    targetDocExistingAddApps()[existingAddApp->getId()] = existingAddApp;
}

void PlatformDocReader::parseCustomField()
{
    // Custom Field to Build
    CustomFieldBuilder cfb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_CustomField::ELEMENT_GAME_ID)
            cfb.wGameId(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_CustomField::ELEMENT_NAME)
            cfb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_CustomField::ELEMENT_VALUE)
            cfb.wValue(mStreamReader.readElementText());
        else
            cfb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Custom Field and add to document
    std::shared_ptr<CustomField> existingCustomField = cfb.buildShared();
    QString key = existingCustomField->getGameId().toString() + existingCustomField->getName();
    static_cast<PlatformDoc*>(mTargetDocument)->mCustomFieldsExisting[key] = existingCustomField;
}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDocWriter::PlatformDocWriter(PlatformDoc* sourceDoc) :
    Fe::DataDocWriter(sourceDoc),
    Fe::BasicPlatformDocWriter(sourceDoc),
    XmlDocWriter(sourceDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformDocWriter::writeSourceDoc()
{
    // Write all games
    for(std::shared_ptr<Fe::Game> game : static_cast<PlatformDoc*>(mSourceDocument)->getFinalGames())
    {
        if(!writeGame(*std::static_pointer_cast<Game>(game)))
            return false;
    }

    // Write all additional apps
    for(std::shared_ptr<Fe::AddApp> addApp : static_cast<PlatformDoc*>(mSourceDocument)->getFinalAddApps())
    {
        if(!writeAddApp(*std::static_pointer_cast<AddApp>(addApp)))
            return false;
    }

    // Write all custom fields
    for(std::shared_ptr<CustomField> customField : static_cast<PlatformDoc*>(mSourceDocument)->mCustomFieldsFinal)
    {
        if(!writeCustomField(*customField))
            return false;
    }

    // Return true on success
    return true;
}

bool PlatformDocWriter::writeGame(const Game& game)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_Game::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_Game::ELEMENT_ID, game.getId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_Game::ELEMENT_TITLE, game.getTitle());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_SERIES, game.getSeries());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_DEVELOPER, game.getDeveloper());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_PUBLISHER, game.getPublisher());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_PLATFORM, game.getPlatform());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_SORT_TITLE, game.getSortTitle());

    if(game.getDateAdded().isValid()) // LB is picky with dates
        writeCleanTextElement(Xml::Element_Game::ELEMENT_DATE_ADDED, game.getDateAdded().toString(Qt::ISODateWithMs));

    if(game.getDateModified().isValid())// LB is picky with dates
        writeCleanTextElement(Xml::Element_Game::ELEMENT_DATE_MODIFIED, game.getDateModified().toString(Qt::ISODateWithMs));

    writeCleanTextElement(Xml::Element_Game::ELEMENT_BROKEN, game.isBroken() ? "true" : "false");
    writeCleanTextElement(Xml::Element_Game::ELEMENT_PLAYMODE, game.getPlayMode());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_STATUS, game.getStatus());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_REGION, game.getRegion());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_NOTES, game.getNotes()); // Some titles have had notes with illegal xml
    writeCleanTextElement(Xml::Element_Game::ELEMENT_SOURCE, game.getSource());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_APP_PATH, game.getAppPath());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_COMMAND_LINE, game.getCommandLine());

    if(game.getReleaseDate().isValid()) // LB is picky with dates
        writeCleanTextElement(Xml::Element_Game::ELEMENT_RELEASE_DATE, game.getReleaseDate().toString(Qt::ISODateWithMs));

    writeCleanTextElement(Xml::Element_Game::ELEMENT_VERSION, game.getVersion());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_RELEASE_TYPE, game.getReleaseType());

    // Write other tags
    writeOtherFields(game.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformDocWriter::writeAddApp(const AddApp& addApp)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_AddApp::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_ID, addApp.getId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_GAME_ID, addApp.getGameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_APP_PATH, addApp.getAppPath());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_COMMAND_LINE, addApp.getCommandLine());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_AUTORUN_BEFORE, addApp.isAutorunBefore() ? "true" : "false");
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_NAME, addApp.getName());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_WAIT_FOR_EXIT, addApp.isWaitForExit() ? "true" : "false");

    // Write other tags
    writeOtherFields(addApp.getOtherFields());

    // Close additional app tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformDocWriter::writeCustomField(const CustomField& customField)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_CustomField::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_CustomField::ELEMENT_GAME_ID, customField.getGameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_CustomField::ELEMENT_NAME, customField.getName());
    writeCleanTextElement(Xml::Element_CustomField::ELEMENT_VALUE, customField.getValue());

    // Write other tags
    writeOtherFields(customField.getOtherFields());

    // Close custom field tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDoc::PlaylistDoc(Install* const parent, std::unique_ptr<QFile> xmlFile, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&) :
    Fe::BasicPlaylistDoc(parent, std::move(xmlFile), docName, updateOptions),
    mLaunchBoxDatabaseIdTracker(&parent->mLbDatabaseIdTracker)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
std::shared_ptr<Fe::PlaylistHeader> PlaylistDoc::preparePlaylistHeader(const Fp::Playlist& playlist)
{
    // Convert to LaunchBox playlist header
    std::shared_ptr<PlaylistHeader> lbPlaylist = std::make_shared<PlaylistHeader>(playlist);

    // Return converted playlist header
    return lbPlaylist;
}

std::shared_ptr<Fe::PlaylistGame> PlaylistDoc::preparePlaylistGame(const Fp::PlaylistGame& game)
{
    // Convert to LaunchBox playlist game
    std::shared_ptr<PlaylistGame> lbPlaylistGame = std::make_shared<PlaylistGame>(game, static_cast<Install*>(parent())->mPlaylistGameDetailsCache);

    // Set LB Database ID appropriately before hand-off
    QUuid key = lbPlaylistGame->getId();
    if(mPlaylistGamesExisting.contains(key))
    {
        // Move LB playlist ID if applicable
        if(mUpdateOptions.importMode == Fe::ImportMode::NewAndExisting)
            lbPlaylistGame->setLBDatabaseId(std::static_pointer_cast<PlaylistGame>(mPlaylistGamesExisting[key])->getLBDatabaseId());
    }
    else
        lbPlaylistGame->setLBDatabaseId(mLaunchBoxDatabaseIdTracker->reserveFirstFree());

    // Return converted playlist game
    return lbPlaylistGame;
}

//===============================================================================================================
// PlaylistDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDocReader::PlaylistDocReader(PlaylistDoc* targetDoc) :
    Fe::DataDocReader(targetDoc),
    Fe::BasicPlaylistDocReader(targetDoc),
    XmlDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlaylistDocReader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_PlaylistHeader::NAME)
            parsePlaylistHeader();
        else if(mStreamReader.name() == Xml::Element_PlaylistGame::NAME)
            parsePlaylistGame();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return !mStreamReader.hasError();
}

void PlaylistDocReader::parsePlaylistHeader()
{
    // Playlist Header to Build
    PlaylistHeaderBuilder phb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_PlaylistHeader::ELEMENT_ID)
            phb.wPlaylistId(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistHeader::ELEMENT_NAME)
            phb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistHeader::ELEMENT_NESTED_NAME)
            phb.wNestedName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistHeader::ELEMENT_NOTES)
            phb.wNotes(mStreamReader.readElementText());
        else
            phb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Header and add to document
    targetDocPlaylistHeader() = phb.buildShared();
}

void PlaylistDocReader::parsePlaylistGame()
{
    // Playlist Game to Build
    PlaylistGameBuilder pgb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_PlaylistGame::ELEMENT_ID)
            pgb.wGameId(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistGame::ELEMENT_GAME_TITLE)
            pgb.wGameTitle(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistGame::ELEMENT_GAME_FILE_NAME)
            pgb.wGameFileName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistGame::ELEMENT_GAME_PLATFORM)
            pgb.wGamePlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistGame::ELEMENT_MANUAL_ORDER)
            pgb.wManualOrder(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlaylistGame::ELEMENT_LB_DB_ID)
            pgb.wLBDatabaseId(mStreamReader.readElementText());
        else
            pgb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Game
    std::shared_ptr<Lb::PlaylistGame> existingPlaylistGame = pgb.buildShared();

    // Correct LB ID if it is invalid and then add it to tracker
    if(existingPlaylistGame->getLBDatabaseId() < 0)
        existingPlaylistGame->setLBDatabaseId(static_cast<PlaylistDoc*>(mTargetDocument)->mLaunchBoxDatabaseIdTracker->reserveFirstFree());
    else
        static_cast<PlaylistDoc*>(mTargetDocument)->mLaunchBoxDatabaseIdTracker->release(existingPlaylistGame->getLBDatabaseId());

    // Add to document
    targetDocExistingPlaylistGames()[existingPlaylistGame->getGameId()] = existingPlaylistGame;
}



//===============================================================================================================
// PlaylistDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDocWriter::PlaylistDocWriter(PlaylistDoc* sourceDoc) :
    Fe::DataDocWriter(sourceDoc),
    Fe::BasicPlaylistDocWriter(sourceDoc),
    XmlDocWriter(sourceDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlaylistDocWriter::writeSourceDoc()
{
    // Write playlist header
    std::shared_ptr<Fe::PlaylistHeader> playlistHeader = static_cast<PlaylistDoc*>(mSourceDocument)->getPlaylistHeader();
    if(!writePlaylistHeader(*std::static_pointer_cast<PlaylistHeader>(playlistHeader)))
        return false;

    // Write all playlist games
    for(std::shared_ptr<Fe::PlaylistGame> playlistGame : static_cast<PlaylistDoc*>(mSourceDocument)->getFinalPlaylistGames())
    {
        if(!writePlaylistGame(*std::static_pointer_cast<PlaylistGame>(playlistGame)))
            return false;
    }

    // Return true on success
    return true;
}

bool PlaylistDocWriter::writePlaylistHeader(const PlaylistHeader& playlistHeader)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlaylistHeader::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_ID, playlistHeader.getPlaylistId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_NAME, playlistHeader.getName());
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_NESTED_NAME, playlistHeader.getNestedName());
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_NOTES, playlistHeader.getNotes());

    // Write other tags
    writeOtherFields(playlistHeader.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlaylistDocWriter::writePlaylistGame(const PlaylistGame& playlistGame)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlaylistGame::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_ID, playlistGame.getGameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_GAME_TITLE, playlistGame.getGameTitle());
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_GAME_PLATFORM, playlistGame.getGamePlatform());
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_MANUAL_ORDER, QString::number(playlistGame.getManualOrder()));
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_LB_DB_ID, QString::number(playlistGame.getLBDatabaseId()));

    // Write other tags
    writeOtherFields(playlistGame.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// PlatformsDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsDoc::PlatformsDoc(Install* const parent, std::unique_ptr<QFile> xmlFile, const DocKey&) :
    Fe::DataDoc(parent, std::move(xmlFile), STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Fe::DataDoc::Type PlatformsDoc::type() const { return Fe::DataDoc::Type::Config; }

//Public:
const QHash<QString, Platform>& PlatformsDoc::getPlatforms() const { return mPlatforms; }
const QMap<QString, QMap<QString, QString>>& PlatformsDoc::getPlatformFolders() const { return mPlatformFolders; }
const QList<PlatformCategory>& PlatformsDoc::getPlatformCategories() const { return mPlatformCategories; }

bool PlatformsDoc::containsPlatform(QString name) { return mPlatforms.contains(name); }

void PlatformsDoc::addPlatform(Platform platform) { mPlatforms[platform.getName()] = platform; }

void PlatformsDoc::setMediaFolder(QString platform, QString mediaType, QString folderPath)
{
    mPlatformFolders[platform][mediaType] = folderPath;
}

//===============================================================================================================
// PlatformsDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsDocReader::PlatformsDocReader(PlatformsDoc* targetDoc) :
    Fe::DataDocReader(targetDoc),
    XmlDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformsDocReader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Platform::NAME)
            parsePlatform();
        else if(mStreamReader.name() == Xml::Element_PlatformFolder::NAME)
            parsePlatformFolder();
        else if (mStreamReader.name() == Xml::Element_PlatformCategory::NAME)
            parsePlatformCategory();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return !mStreamReader.hasError();
}

void PlatformsDocReader::parsePlatform()
{
    // Platform Config Doc to Build
    PlatformBuilder pb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Platform::ELEMENT_NAME)
            pb.wName(mStreamReader.readElementText());
        else
            pb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Platform and add to document
    std::shared_ptr<Platform> existingPlatform = pb.buildShared();
    static_cast<PlatformsDoc*>(mTargetDocument)->mPlatforms.insert(existingPlatform->getName(), *existingPlatform);
}

void PlatformsDocReader::parsePlatformFolder()
{
    // Platform Folder to Build
    QString platform;
    QString mediaType;
    QString folderPath;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_PlatformFolder::ELEMENT_MEDIA_TYPE)
            mediaType = mStreamReader.readElementText();
        else if(mStreamReader.name() == Xml::Element_PlatformFolder::ELEMENT_FOLDER_PATH)
            folderPath = mStreamReader.readElementText();
        else if(mStreamReader.name() == Xml::Element_PlatformFolder::ELEMENT_PLATFORM)
            platform = mStreamReader.readElementText();
        else
            mStreamReader.raiseError(mTargetDocument->errorString(Fe::DataDoc::StandardError::DocTypeMismatch));
    }

    // Add to document
    static_cast<PlatformsDoc*>(mTargetDocument)->mPlatformFolders[platform][mediaType] = folderPath;
}

void PlatformsDocReader::parsePlatformCategory()
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
// PlatformsDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsDocWriter::PlatformsDocWriter(PlatformsDoc* sourceDoc) :
    Fe::DataDocWriter(sourceDoc),
    XmlDocWriter(sourceDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformsDocWriter::writeSourceDoc()
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

bool PlatformsDocWriter::writePlatform(const Platform& platform)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_Platform::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_Platform::ELEMENT_NAME, platform.getName());

    // Write other tags
    writeOtherFields(platform.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformsDocWriter::writePlatformFolder(const QString& platform, const QString& mediaType, const QString& folderPath)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlatformFolder::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_PlatformFolder::ELEMENT_MEDIA_TYPE, mediaType);
    writeCleanTextElement(Xml::Element_PlatformFolder::ELEMENT_FOLDER_PATH, folderPath);
    writeCleanTextElement(Xml::Element_PlatformFolder::ELEMENT_PLATFORM, platform);

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformsDocWriter::writePlatformCategory(const PlatformCategory& platformCategory)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlatformCategory::NAME);

    // Write known tags
    // None for now...
    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    writeOtherFields(platformCategory.getOtherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

}
