// Unit Include
#include "lb-data.h"

// Standard Library Includes
#include <memory>

// Project Includes
#include "import/details.h"
#include "launcher/implementation/launchbox/lb-install.h"

namespace Xml
{

namespace Element_Game
{
    const QString NAME = u"Game"_s;

    const QString ELEMENT_ID = u"ID"_s;
    const QString ELEMENT_TITLE = u"Title"_s;
    const QString ELEMENT_SERIES = u"Series"_s;
    const QString ELEMENT_DEVELOPER = u"Developer"_s;
    const QString ELEMENT_PUBLISHER = u"Publisher"_s;
    const QString ELEMENT_PLATFORM = u"Platform"_s;
    const QString ELEMENT_SORT_TITLE = u"SortTitle"_s;
    const QString ELEMENT_DATE_ADDED = u"DateAdded"_s;
    const QString ELEMENT_DATE_MODIFIED = u"DateModified"_s;
    const QString ELEMENT_BROKEN = u"Broken"_s;
    const QString ELEMENT_PLAYMODE = u"PlayMode"_s;
    const QString ELEMENT_STATUS = u"Status"_s;
    const QString ELEMENT_REGION = u"Region"_s;
    const QString ELEMENT_NOTES = u"Notes"_s;
    const QString ELEMENT_SOURCE = u"Source"_s;
    const QString ELEMENT_APP_PATH = u"ApplicationPath"_s;
    const QString ELEMENT_COMMAND_LINE = u"CommandLine"_s;
    const QString ELEMENT_RELEASE_DATE = u"ReleaseDate"_s;
    const QString ELEMENT_VERSION = u"Version"_s;
    const QString ELEMENT_RELEASE_TYPE = u"ReleaseType"_s;
}

namespace Element_AddApp
{
    const QString NAME = u"AdditionalApplication"_s;

    const QString ELEMENT_ID = u"Id"_s;
    const QString ELEMENT_GAME_ID = u"GameID"_s;
    const QString ELEMENT_APP_PATH = u"ApplicationPath"_s;
    const QString ELEMENT_COMMAND_LINE = u"CommandLine"_s;
    const QString ELEMENT_AUTORUN_BEFORE = u"AutoRunBefore"_s;
    const QString ELEMENT_NAME = u"Name"_s;
    const QString ELEMENT_WAIT_FOR_EXIT = u"WaitForExit"_s;
}

namespace Element_CustomField
{
    const QString NAME = u"CustomField"_s;

    const QString ELEMENT_GAME_ID = u"GameID"_s;
    const QString ELEMENT_NAME = u"Name"_s;
    const QString ELEMENT_VALUE = u"Value"_s;
}

namespace Element_PlaylistHeader
{
    const QString NAME = u"Playlist"_s;

    const QString ELEMENT_ID = u"PlaylistId"_s;
    const QString ELEMENT_NAME = u"Name"_s;
    const QString ELEMENT_NESTED_NAME = u"NestedName"_s;
    const QString ELEMENT_NOTES = u"Notes"_s;
}

namespace Element_PlaylistGame
{
    const QString NAME = u"PlaylistGame"_s;

    const QString ELEMENT_ID = u"GameId"_s;
    const QString ELEMENT_GAME_TITLE = u"GameTitle"_s;
    const QString ELEMENT_GAME_FILE_NAME = u"GameFileName"_s;
    const QString ELEMENT_GAME_PLATFORM = u"GamePlatform"_s;
    const QString ELEMENT_MANUAL_ORDER = u"ManualOrder"_s;
    const QString ELEMENT_LB_DB_ID = u"LaunchBoxDbId"_s;
}

namespace Element_Platform
{
    const QString NAME = u"Platform"_s;

    const QString ELEMENT_NAME = u"Name"_s;
    const QString ELEMENT_CATEGORY = u"Category"_s;
}

namespace Element_PlatformFolder
{
    const QString NAME = u"PlatformFolder"_s;

    const QString ELEMENT_MEDIA_TYPE = u"MediaType"_s;
    const QString ELEMENT_FOLDER_PATH = u"FolderPath"_s;
    const QString ELEMENT_PLATFORM = u"Platform"_s;
}

namespace Element_PlatformCategory
{
    const QString NAME = u"PlatformCategory"_s;

    const QString ELEMENT_NAME = u"Name"_s;
    const QString ELEMENT_NESTED_NAME = u"NestedName"_s;
}

 namespace Element_Parent
 {
    const QString NAME = u"Parent"_s;

    const QString ELEMENT_PLATFORM_CATEGORY_NAME = u"PlatformCategoryName"_s;
    const QString ELEMENT_PLATFORM_NAME = u"PlatformName"_s;
    const QString ELEMENT_PARENT_PLATFORM_CATEGORY_NAME = u"ParentPlatformCategoryName"_s;
    const QString ELEMENT_PLAYLIST_ID = u"PlaylistId"_s;
 }

const QString ROOT_ELEMENT = u"LaunchBox"_s;
}

namespace Lb
{
//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::PlatformDoc(Install* install, const QString& xmlPath, QString docName, const Import::UpdateOptions& updateOptions) :
    Lr::BasicPlatformDoc<LauncherId>(install, xmlPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Game PlatformDoc::prepareGame(const Fp::Game& game)
{
    // Convert to LaunchBox game
    const QString& clifpPath = Import::Details::current().clifpPath;
    Game lbGame(game, clifpPath);

    // Add details to cache
    install()->mPlaylistGameDetailsCache.insert(game.id(), PlaylistGame::EntryDetails(lbGame));

    // Add language as custom field
    CustomField::Builder cfb;
    cfb.wGameId(game.id());
    cfb.wName(CustomField::LANGUAGE);
    cfb.wValue(game.language());
    addCustomField(cfb.build());

    // Return converted game
    return lbGame;
}

AddApp PlatformDoc::prepareAddApp(const Fp::AddApp& addApp)
{
    // Convert to LaunchBox add app
    const QString& clifpPath = Import::Details::current().clifpPath;
    AddApp lbAddApp(addApp, clifpPath);

    // Return converted game
    return lbAddApp;
}

void PlatformDoc::addCustomField(CustomField&& customField)
{
    QString key = customField.gameId().toString() + customField.name();
    addUpdateableItem(mCustomFieldsExisting, mCustomFieldsFinal, key, customField);
}

//Public:
bool PlatformDoc::isEmpty() const
{
    return mCustomFieldsFinal.isEmpty() && mCustomFieldsExisting.isEmpty() && Lr::BasicPlatformDoc<LauncherId>::isEmpty();
}

void PlatformDoc::finalize()
{
    // Finalize custom fields
    finalizeUpdateableItems(mCustomFieldsExisting, mCustomFieldsFinal);

    // Ensure that custom fields for removed games are deleted
    QHash<QString, CustomField>::iterator i = mCustomFieldsFinal.begin();
    while (i != mCustomFieldsFinal.end())
    {
        if(!finalGames().contains(i.value().gameId()))
            i = mCustomFieldsFinal.erase(i);
        else
            ++i;
    }

    Lr::BasicPlatformDoc<LauncherId>::finalize();
}

//===============================================================================================================
// PlatformDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDocReader::PlatformDocReader(PlatformDoc* targetDoc) :
    Lr::XmlDocReader<PlatformDoc>(targetDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Lr::DocHandlingError PlatformDocReader::readTargetDoc()
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
    return streamStatus();
}

void PlatformDocReader::parseGame()
{
    // Game to build
    Game::Builder gb;

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
    Game existingGame = gb.build();
    target()->mGamesExisting[existingGame.id()] = existingGame;
}

void PlatformDocReader::parseAddApp()
{
    // Additional App to Build
    AddApp::Builder aab;

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
    AddApp existingAddApp = aab.build();
    target()->mAddAppsExisting[existingAddApp.id()] = existingAddApp;
}

void PlatformDocReader::parseCustomField()
{
    // Custom Field to Build
    CustomField::Builder cfb;

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
    CustomField existingCustomField = cfb.build();
    QString key = existingCustomField.gameId().toString() + existingCustomField.name();
    target()->mCustomFieldsExisting[key] = existingCustomField;
}

//===============================================================================================================
// PlatformDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDocWriter::PlatformDocWriter(PlatformDoc* sourceDoc) :
    Lr::XmlDocWriter<PlatformDoc>(sourceDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformDocWriter::writeSourceDoc()
{
    // Write all games
    for(const Game& game : source()->finalGames())
    {
        if(!writeGame(game))
            return false;
    }

    // Write all additional apps
    for(const AddApp& addApp : source()->finalAddApps())
    {
        if(!writeAddApp(addApp))
            return false;
    }

    // Write all custom fields
    for(const CustomField& customField : std::as_const(source()->mCustomFieldsFinal))
    {
        if(!writeCustomField(customField))
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
    writeCleanTextElement(Xml::Element_Game::ELEMENT_ID, game.id().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_Game::ELEMENT_TITLE, game.title());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_SERIES, game.series());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_DEVELOPER, game.developer());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_PUBLISHER, game.publisher());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_PLATFORM, game.platform());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_SORT_TITLE, game.sortTitle());

    if(game.dateAdded().isValid()) // LB is picky with dates
        writeCleanTextElement(Xml::Element_Game::ELEMENT_DATE_ADDED, game.dateAdded().toString(Qt::ISODateWithMs));

    if(game.dateModified().isValid())// LB is picky with dates
        writeCleanTextElement(Xml::Element_Game::ELEMENT_DATE_MODIFIED, game.dateModified().toString(Qt::ISODateWithMs));

    writeCleanTextElement(Xml::Element_Game::ELEMENT_BROKEN, game.isBroken() ? u"true"_s : u"false"_s);
    writeCleanTextElement(Xml::Element_Game::ELEMENT_PLAYMODE, game.playMode());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_STATUS, game.status());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_REGION, game.region());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_NOTES, game.notes()); // Some titles have had notes with illegal xml
    writeCleanTextElement(Xml::Element_Game::ELEMENT_SOURCE, game.source());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_APP_PATH, game.appPath());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_COMMAND_LINE, game.commandLine());

    if(game.releaseDate().isValid()) // LB is picky with dates
        writeCleanTextElement(Xml::Element_Game::ELEMENT_RELEASE_DATE, game.releaseDate().toString(Qt::ISODateWithMs));

    writeCleanTextElement(Xml::Element_Game::ELEMENT_VERSION, game.version());
    writeCleanTextElement(Xml::Element_Game::ELEMENT_RELEASE_TYPE, game.releaseType());

    // Write other tags
    writeOtherFields(game.otherFields());

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
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_ID, addApp.id().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_GAME_ID, addApp.gameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_APP_PATH, addApp.appPath());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_COMMAND_LINE, addApp.commandLine());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_AUTORUN_BEFORE, addApp.isAutorunBefore() ? u"true"_s : u"false"_s);
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_NAME, addApp.name());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_WAIT_FOR_EXIT, addApp.isWaitForExit() ? u"true"_s : u"false"_s);

    // Write other tags
    writeOtherFields(addApp.otherFields());

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
    writeCleanTextElement(Xml::Element_CustomField::ELEMENT_GAME_ID, customField.gameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_CustomField::ELEMENT_NAME, customField.name());
    writeCleanTextElement(Xml::Element_CustomField::ELEMENT_VALUE, customField.value());

    // Write other tags
    writeOtherFields(customField.otherFields());

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
PlaylistDoc::PlaylistDoc(Install* install, const QString& xmlPath, QString docName, const Import::UpdateOptions& updateOptions) :
    Lr::BasicPlaylistDoc<LauncherId>(install, xmlPath, docName, updateOptions),
    mLaunchBoxDatabaseIdTracker(&install->mLbDatabaseIdTracker)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
PlaylistHeader PlaylistDoc::preparePlaylistHeader(const Fp::Playlist& playlist)
{
    // Convert to LaunchBox playlist header
    return PlaylistHeader(playlist);
}

PlaylistGame PlaylistDoc::preparePlaylistGame(const Fp::PlaylistGame& game)
{
    // Convert to LaunchBox playlist game
    PlaylistGame lbPlaylistGame(game, install()->mPlaylistGameDetailsCache);

    // Set LB Database ID appropriately before hand-off
    QUuid key = lbPlaylistGame.id();
    if(mPlaylistGamesExisting.contains(key))
    {
        // Move LB playlist ID if applicable
        if(mUpdateOptions.importMode == Import::UpdateMode::NewAndExisting)
            lbPlaylistGame.setLBDatabaseId(mPlaylistGamesExisting[key].lbDatabaseId());
    }
    else
    {
        auto optIdx = mLaunchBoxDatabaseIdTracker->reserveFirstFree();
        lbPlaylistGame.setLBDatabaseId(optIdx.value_or(0));
    }

    // Return converted playlist game
    return lbPlaylistGame;
}

//===============================================================================================================
// PlaylistDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDocReader::PlaylistDocReader(PlaylistDoc* targetDoc) :
    Lr::XmlDocReader<PlaylistDoc>(targetDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Lr::DocHandlingError PlaylistDocReader::readTargetDoc()
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
    return streamStatus();
}

void PlaylistDocReader::parsePlaylistHeader()
{
    // Playlist Header to Build
    PlaylistHeader::Builder phb;

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
    target()->mPlaylistHeader = phb.build();
}

void PlaylistDocReader::parsePlaylistGame()
{
    // Playlist Game to Build
    PlaylistGame::Builder pgb;

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
    PlaylistGame existingPlaylistGame = pgb.build();

    // Correct LB ID if it is invalid and then add it to tracker
    if(existingPlaylistGame.lbDatabaseId() < 0)
    {
        auto optIdx = target()->mLaunchBoxDatabaseIdTracker->reserveFirstFree();
        existingPlaylistGame.setLBDatabaseId(optIdx.value_or(0));
    }
    else
        target()->mLaunchBoxDatabaseIdTracker->reserve(existingPlaylistGame.lbDatabaseId());

    // Add to document
    target()->mPlaylistGamesExisting[existingPlaylistGame.gameId()] = existingPlaylistGame;
}

//===============================================================================================================
// PlaylistDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDocWriter::PlaylistDocWriter(PlaylistDoc* sourceDoc) :
    Lr::XmlDocWriter<PlaylistDoc>(sourceDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlaylistDocWriter::writeSourceDoc()
{
    // Write playlist header
    PlaylistHeader playlistHeader = source()->playlistHeader();
    if(!writePlaylistHeader(playlistHeader))
        return false;

    // Write all playlist games
    for(const PlaylistGame& playlistGame : source()->finalPlaylistGames())
    {
        if(!writePlaylistGame(playlistGame))
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
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_ID, playlistHeader.playlistId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_NAME, playlistHeader.name());
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_NESTED_NAME, playlistHeader.nestedName());
    writeCleanTextElement(Xml::Element_PlaylistHeader::ELEMENT_NOTES, playlistHeader.notes());

    // Write other tags
    writeOtherFields(playlistHeader.otherFields());

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
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_ID, playlistGame.gameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_GAME_TITLE, playlistGame.gameTitle());
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_GAME_PLATFORM, playlistGame.gamePlatform());
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_MANUAL_ORDER, QString::number(playlistGame.manualOrder()));
    writeCleanTextElement(Xml::Element_PlaylistGame::ELEMENT_LB_DB_ID, QString::number(playlistGame.lbDatabaseId()));

    // Write other tags
    writeOtherFields(playlistGame.otherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// PlatformsConfigDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsConfigDoc::PlatformsConfigDoc(Install* install, const QString& xmlPath, const Import::UpdateOptions& updateOptions) :
    Lr::UpdateableDoc<LauncherId>(install, xmlPath, STD_NAME, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Lr::IDataDoc::Type PlatformsConfigDoc::type() const { return Lr::IDataDoc::Type::Config; }

//Public:
bool PlatformsConfigDoc::isEmpty() const
{
    return mPlatformsFinal.isEmpty() && mPlatformsExisting.isEmpty() &&
           mPlatformFoldersFinal.isEmpty() && mPlatformFoldersExisting.isEmpty() &&
           mPlatformCategoriesFinal.isEmpty() && mPlatformCategoriesExisting.isEmpty();
}

const QHash<QString, Platform>& PlatformsConfigDoc::finalPlatforms() const { return mPlatformsFinal; }
const QMap<QString, PlatformFolder>& PlatformsConfigDoc::finalPlatformFolders() const { return mPlatformFoldersFinal; }
const QMap<QString, PlatformCategory>& PlatformsConfigDoc::finalPlatformCategories() const { return mPlatformCategoriesFinal; }

void PlatformsConfigDoc::addPlatform(const Platform& platform)
{
    // Add platform, don't need to add media folders as LB will automatically set them to the defaults
    addUpdateableItem(mPlatformsExisting, mPlatformsFinal, platform.name(), platform);
}

void PlatformsConfigDoc::removePlatform(const QString& name)
{
    // Remove platform and any of its media folders (so LB will reset them to default)
    mPlatformsFinal.remove(name);
    mPlatformsExisting.remove(name);
    removePlatformFolders(name);
}

void PlatformsConfigDoc::addPlatformFolder(const PlatformFolder& platformFolder)
{
    addUpdateableItem(mPlatformFoldersExisting, mPlatformFoldersFinal, platformFolder.identifier(), platformFolder);
}

void PlatformsConfigDoc::removePlatformFolders(const QString& platformName)
{
    auto culler = [&platformName](QMap<QString, PlatformFolder>::iterator itr){
        return itr.value().platform() == platformName;
    };
    mPlatformFoldersExisting.removeIf(culler);
    mPlatformFoldersFinal.removeIf(culler);
}

void PlatformsConfigDoc::addPlatformCategory(const PlatformCategory& platformCategory)
{
    addUpdateableItem(mPlatformCategoriesExisting, mPlatformCategoriesFinal, platformCategory.name(), platformCategory);
}

void PlatformsConfigDoc::removePlatformCategory(const QString& categoryName)
{
    mPlatformCategoriesFinal.remove(categoryName);
    mPlatformCategoriesExisting.remove(categoryName);
}

void PlatformsConfigDoc::finalize()
{
    // Finalize derived
    finalizeUpdateableItems(mPlatformsExisting, mPlatformsFinal);
    finalizeUpdateableItems(mPlatformFoldersExisting, mPlatformFoldersFinal);
    finalizeUpdateableItems(mPlatformCategoriesExisting, mPlatformCategoriesFinal);

    // Finalize base
    Lr::IUpdateableDoc::finalize();
}

//===============================================================================================================
// PlatformsConfigDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsConfigDoc::Reader::Reader(PlatformsConfigDoc* targetDoc) :
    Lr::XmlDocReader<PlatformsConfigDoc>(targetDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Lr::DocHandlingError PlatformsConfigDoc::Reader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Platform::NAME)
            parsePlatform();
        else if(mStreamReader.name() == Xml::Element_PlatformFolder::NAME)
        {
            if(Lr::DocHandlingError dhe = parsePlatformFolder(); dhe.isValid())
                return dhe;
        }
        else if (mStreamReader.name() == Xml::Element_PlatformCategory::NAME)
            parsePlatformCategory();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return streamStatus();
}

void PlatformsConfigDoc::Reader::parsePlatform()
{
    // Platform to Build
    Platform::Builder pb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Platform::ELEMENT_NAME)
            pb.wName(mStreamReader.readElementText());
//        else if(mStreamReader.name() == Xml::Element_Platform::ELEMENT_CATEGORY)
//            pb.wCategory(mStreamReader.readElementText());
        else
            pb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Platform and add to document
    Platform existingPlatform = pb.build();
    target()->mPlatformsExisting[existingPlatform.name()] = existingPlatform;
}

Lr::DocHandlingError PlatformsConfigDoc::Reader::parsePlatformFolder()
{
    // Platform Folder to Build
    PlatformFolder::Builder pfb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_PlatformFolder::ELEMENT_MEDIA_TYPE)
            pfb.wMediaType(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlatformFolder::ELEMENT_FOLDER_PATH)
            pfb.wFolderPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlatformFolder::ELEMENT_PLATFORM)
            pfb.wPlatform(mStreamReader.readElementText());
        else
            return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocInvalidType);
    }

    // Build PlatformFolder and add to document
    PlatformFolder existingPlatformFolder = pfb.build();
    target()->mPlatformFoldersExisting[existingPlatformFolder.identifier()] = existingPlatformFolder;

    return Lr::DocHandlingError();
}

void PlatformsConfigDoc::Reader::parsePlatformCategory()
{
    // Platform Config Doc to Build
    PlatformCategory::Builder pcb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_PlatformCategory::ELEMENT_NAME)
            pcb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_PlatformCategory::ELEMENT_NESTED_NAME)
            pcb.wNestedName(mStreamReader.readElementText());
        else
            pcb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build
    PlatformCategory pc = pcb.build();

    // Build Playlist Header and add to document
   target()->mPlatformCategoriesExisting[pc.name()] = pc;
}

//===============================================================================================================
// PlatformsConfigDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsConfigDoc::Writer::Writer(PlatformsConfigDoc* sourceDoc) :
    Lr::XmlDocWriter<PlatformsConfigDoc>(sourceDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformsConfigDoc::Writer::writeSourceDoc()
{
    // Write all platforms
    for(const Platform& platform : source()->finalPlatforms())
    {
        if(!writePlatform(platform))
            return false;
    }

    // Write all platform folders
    for(const PlatformFolder& platformFolder : source()->finalPlatformFolders())
    {
        if(!writePlatformFolder(platformFolder))
            return false;
    }

    // Write all platform categories
    for(const PlatformCategory& platformCategory : source()->finalPlatformCategories())
    {
        if(!writePlatformCategory(platformCategory))
            return false;
    }

    // Return true on success
    return true;
}

bool PlatformsConfigDoc::Writer::writePlatform(const Platform& platform)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_Platform::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_Platform::ELEMENT_NAME, platform.name());
//    writeCleanTextElement(Xml::Element_Platform::ELEMENT_CATEGORY, platform.category());

    // Write other tags
    writeOtherFields(platform.otherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformsConfigDoc::Writer::writePlatformFolder(const PlatformFolder& platformFoler)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlatformFolder::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_PlatformFolder::ELEMENT_MEDIA_TYPE, platformFoler.mediaType());
    writeCleanTextElement(Xml::Element_PlatformFolder::ELEMENT_FOLDER_PATH, platformFoler.folderPath());
    writeCleanTextElement(Xml::Element_PlatformFolder::ELEMENT_PLATFORM, platformFoler.platform());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformsConfigDoc::Writer::writePlatformCategory(const PlatformCategory& platformCategory)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlatformCategory::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_PlatformCategory::ELEMENT_NAME, platformCategory.name());
    writeCleanTextElement(Xml::Element_PlatformCategory::ELEMENT_NESTED_NAME, platformCategory.nestedName());

    // Write other tags
    writeOtherFields(platformCategory.otherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// ParentsDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ParentsDoc::ParentsDoc(Install* install, const QString& xmlPath) :
    Lr::DataDoc<LauncherId>(install, xmlPath, STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Lr::IDataDoc::Type ParentsDoc::type() const { return Lr::IDataDoc::Type::Config; }

bool ParentsDoc::removeIfPresent(qsizetype idx)
{
    if(idx != -1)
    {
        mParents.remove(idx);
        return true;
    }

    return false;
}

qsizetype ParentsDoc::findPlatformCategory(QStringView platformCategory, QStringView parentCategory) const
{
    for(auto i = 0; i < mParents.size(); i++)
    {
        auto p = mParents.at(i);
        if(p.platformCategoryName() == platformCategory && p.parentPlatformCategoryName() == parentCategory)
            return i;
    }

    return -1;
}

qsizetype ParentsDoc::findPlatform(QStringView platform, QStringView parentCategory) const
{
    for(auto i = 0; i < mParents.size(); i++)
    {
        auto p = mParents.at(i);
        if(p.platformName() == platform && p.parentPlatformCategoryName() == parentCategory)
            return i;
    }

    return -1;
}

qsizetype ParentsDoc::findPlaylist(const QUuid& playlistId, QStringView parentCategory) const
{
    for(auto i = 0; i < mParents.size(); i++)
    {
        auto p = mParents.at(i);
        if(p.playlistId() == playlistId && p.parentPlatformCategoryName() == parentCategory)
            return i;
    }

    return -1;
}

//Public:
bool ParentsDoc::isEmpty() const { return mParents.isEmpty(); }

bool ParentsDoc::containsPlatformCategory(QStringView platformCategory, QStringView parentCategory) const { return findPlatformCategory(platformCategory, parentCategory) != -1; }
bool ParentsDoc::containsPlatform(QStringView platform, QStringView parentCategory) const { return findPlatform(platform, parentCategory) != -1; }
bool ParentsDoc::containsPlaylist(const QUuid& playlistId, QStringView parentCategory) const { return findPlaylist(playlistId, parentCategory) != -1; }

bool ParentsDoc::removePlatformCategory(QStringView platformCategory, QStringView parentCategory) { return removeIfPresent(findPlatformCategory(platformCategory, parentCategory)); }
bool ParentsDoc::removePlatform(QStringView platform, QStringView parentCategory) { return removeIfPresent(findPlatform(platform, parentCategory)); }
bool ParentsDoc::removePlaylist(const QUuid& playlistId, QStringView parentCategory) { return removeIfPresent(findPlaylist(playlistId, parentCategory)); }

const QList<Parent>& ParentsDoc::parents() const { return mParents; }

void ParentsDoc::addParent(const Parent& parent) { mParents.append(parent); }

//===============================================================================================================
// ParentsDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ParentsDoc::Reader::Reader(ParentsDoc* targetDoc) :
    Lr::XmlDocReader<ParentsDoc>(targetDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Lr::DocHandlingError ParentsDoc::Reader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Parent::NAME)
            parseParent();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return streamStatus();
}

void ParentsDoc::Reader::parseParent()
{
    // Parent to build
    Parent::Builder pb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Element_Parent::ELEMENT_PLATFORM_CATEGORY_NAME)
            pb.wPlatformCategoryName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Parent::ELEMENT_PLATFORM_NAME)
            pb.wPlatformName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Parent::ELEMENT_PARENT_PLATFORM_CATEGORY_NAME)
            pb.wParentPlatformCategoryName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Element_Parent::ELEMENT_PLAYLIST_ID)
            pb.wPlaylistId(mStreamReader.readElementText());
        else
            pb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Platform and add to document
    Parent existingParent = pb.build();
    target()->mParents.append(existingParent);
}

//===============================================================================================================
// ParentsDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ParentsDoc::Writer::Writer(ParentsDoc* sourceDoc) :
    Lr::XmlDocWriter<ParentsDoc>(sourceDoc, Xml::ROOT_ELEMENT)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool ParentsDoc::Writer::writeSourceDoc()
{
    // Write all parents
    for(const Parent& parent : source()->parents())
    {
        if(!writeParent(parent))
            return false;
    }

    // Return true on success
    return true;
}

bool ParentsDoc::Writer::writeParent(const Parent& parent)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_Parent::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_Parent::ELEMENT_PLATFORM_CATEGORY_NAME, parent.platformCategoryName());
    writeCleanTextElement(Xml::Element_Parent::ELEMENT_PLATFORM_NAME, parent.platformName());
    writeCleanTextElement(Xml::Element_Parent::ELEMENT_PARENT_PLATFORM_CATEGORY_NAME, parent.parentPlatformCategoryName());
    writeCleanTextElement(Xml::Element_Parent::ELEMENT_PLAYLIST_ID, !parent.playlistId().isNull() ?  parent.playlistId().toString(QUuid::WithoutBraces): u""_s);

    // Write other tags
    writeOtherFields(parent.otherFields());

    // Close tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

}
