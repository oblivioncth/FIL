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
XmlDocReader::XmlDocReader(Fe::DataDoc* targetDoc) :
    Fe::DataDoc::Reader(targetDoc),
    mXmlFile(targetDoc->path()),
    mStreamReader(&mXmlFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError XmlDocReader::readInto()
{
    // Open File
    if(!mXmlFile.open(QFile::ReadOnly))
    {
        return Qx::GenericError(Qx::GenericError::Critical,
                                Fe::docHandlingErrorString(mTargetDocument, Fe::DocHandlingError::DocCantOpen),
                                mXmlFile.errorString());
    }

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
            readError = Qx::XmlStreamReaderError(Fe::docHandlingErrorString(mTargetDocument, Fe::DocHandlingError::NotParentDoc));
    }
    else
        readError = Qx::XmlStreamReaderError(mStreamReader.error());

    return readError.isValid() ? Qx::GenericError(Qx::GenericError::Critical, mStdReadErrorStr, readError.text()) :
                                 Qx::GenericError();

    // File is automatically closed when reader is destroyed...
}

//===============================================================================================================
// XmlDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
XmlDocWriter::XmlDocWriter(Fe::DataDoc* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    mXmlFile(sourceDoc->path()),
    mStreamWriter(&mXmlFile)
{}

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
    // Open File
    if(!mXmlFile.open(QFile::WriteOnly | QFile::Truncate)) // Discard previous contents
    {
        return Qx::GenericError(Qx::GenericError::Critical,
                                Fe::docHandlingErrorString(mSourceDocument, Fe::DocHandlingError::DocCantSave),
                                mXmlFile.errorString());
    }

    // Enable auto formatting
    mStreamWriter.setAutoFormatting(true);
    mStreamWriter.setAutoFormattingIndent(2);

    // Write standard XML header
    mStreamWriter.writeStartDocument("1.0", true);

    // Write main LaunchBox tag
    mStreamWriter.writeStartElement(Xml::ROOT_ELEMENT);

    // Write main body
    if(!writeSourceDoc())
        return Qx::GenericError(Qx::GenericError::Critical, mStdWriteErrorStr, mStreamWriter.device()->errorString());

    // Close main LaunchBox tag
    mStreamWriter.writeEndElement();

    // Finish document
    mStreamWriter.writeEndDocument();

    // Return null string on success
    return mStreamWriter.hasError() ? Qx::GenericError(Qx::GenericError::Critical, mStdWriteErrorStr, mStreamWriter.device()->errorString()) :
                                      Qx::GenericError();

    // File is automatically closed when writer is destroyed...
}

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::PlatformDoc(Install* const parent, const QString& xmlPath, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&) :
    Fe::BasicPlatformDoc(parent, xmlPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
std::shared_ptr<Fe::Game> PlatformDoc::prepareGame(const Fp::Game& game, const Fe::ImageSources& images)
{
    Q_UNUSED(images); // LaunchBox doesn't store image info in its platform doc directly

    // Convert to LaunchBox game
    const QString& clifpPath = static_cast<Install*>(parent())->mImportDetails->clifpPath;
    std::shared_ptr<Game> lbGame = std::make_shared<Game>(game, clifpPath);

    // Add details to cache
    static_cast<Install*>(parent())->mPlaylistGameDetailsCache.insert(game.id(), PlaylistGame::EntryDetails(*lbGame));

    // Add language as custom field
    CustomField::Builder cfb;
    cfb.wGameId(game.id());
    cfb.wName(CustomField::LANGUAGE);
    cfb.wValue(game.language());
    addCustomField(cfb.buildShared());

    // Return converted game
    return lbGame;
}

std::shared_ptr<Fe::AddApp> PlatformDoc::prepareAddApp(const Fp::AddApp& addApp)
{
    // Convert to LaunchBox add app
    const QString& clifpPath = static_cast<Install*>(parent())->mImportDetails->clifpPath;
    std::shared_ptr<AddApp> lbAddApp = std::make_shared<AddApp>(addApp, clifpPath);

    // Return converted game
    return lbAddApp;
}

void PlatformDoc::addCustomField(std::shared_ptr<CustomField> customField)
{
    QString key = customField->gameId().toString() + customField->name();
    addUpdateableItem(mCustomFieldsExisting, mCustomFieldsFinal, key, customField);
}

//Public:
bool PlatformDoc::isEmpty() const
{
    return mCustomFieldsFinal.isEmpty() && mCustomFieldsExisting.isEmpty() && Fe::BasicPlatformDoc::isEmpty();
}

void PlatformDoc::finalize()
{
    // Finalize custom fields
    finalizeUpdateableItems(mCustomFieldsExisting, mCustomFieldsFinal);

    // Ensure that custom fields for removed games are deleted
    QHash<QString, std::shared_ptr<CustomField>>::iterator i = mCustomFieldsFinal.begin();
    while (i != mCustomFieldsFinal.end())
    {
        if(!finalGames().contains(i.value()->gameId()))
            i = mCustomFieldsFinal.erase(i);
        else
            ++i;
    }

    Fe::BasicPlatformDoc::finalize();
}

//===============================================================================================================
// PlatformDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::Reader::Reader(PlatformDoc* targetDoc) :
    Fe::DataDoc::Reader(targetDoc),
    Fe::BasicPlatformDoc::Reader(targetDoc),
    XmlDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformDoc::Reader::readTargetDoc()
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

void PlatformDoc::Reader::parseGame()
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
    std::shared_ptr<Game> existingGame = gb.buildShared();
    targetDocExistingGames()[existingGame->id()] = existingGame;
}

void PlatformDoc::Reader::parseAddApp()
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
    std::shared_ptr<AddApp> existingAddApp = aab.buildShared();
    targetDocExistingAddApps()[existingAddApp->id()] = existingAddApp;
}

void PlatformDoc::Reader::parseCustomField()
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
    std::shared_ptr<CustomField> existingCustomField = cfb.buildShared();
    QString key = existingCustomField->gameId().toString() + existingCustomField->name();
    static_cast<PlatformDoc*>(mTargetDocument)->mCustomFieldsExisting[key] = existingCustomField;
}

//===============================================================================================================
// PlatformDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformDoc::Writer::Writer(PlatformDoc* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    Fe::BasicPlatformDoc::Writer(sourceDoc),
    XmlDocWriter(sourceDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformDoc::Writer::writeSourceDoc()
{
    // Write all games
    for(const std::shared_ptr<Fe::Game>& game : static_cast<PlatformDoc*>(mSourceDocument)->finalGames())
    {
        if(!writeGame(*std::static_pointer_cast<Game>(game)))
            return false;
    }

    // Write all additional apps
    for(const std::shared_ptr<Fe::AddApp>& addApp : static_cast<PlatformDoc*>(mSourceDocument)->finalAddApps())
    {
        if(!writeAddApp(*std::static_pointer_cast<AddApp>(addApp)))
            return false;
    }

    // Write all custom fields
    for(const std::shared_ptr<CustomField>& customField : qAsConst(static_cast<PlatformDoc*>(mSourceDocument)->mCustomFieldsFinal))
    {
        if(!writeCustomField(*customField))
            return false;
    }

    // Return true on success
    return true;
}

bool PlatformDoc::Writer::writeGame(const Game& game)
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

    writeCleanTextElement(Xml::Element_Game::ELEMENT_BROKEN, game.isBroken() ? "true" : "false");
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

bool PlatformDoc::Writer::writeAddApp(const AddApp& addApp)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_AddApp::NAME);

    // Write known tags
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_ID, addApp.id().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_GAME_ID, addApp.gameId().toString(QUuid::WithoutBraces));
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_APP_PATH, addApp.appPath());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_COMMAND_LINE, addApp.commandLine());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_AUTORUN_BEFORE, addApp.isAutorunBefore() ? "true" : "false");
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_NAME, addApp.name());
    writeCleanTextElement(Xml::Element_AddApp::ELEMENT_WAIT_FOR_EXIT, addApp.isWaitForExit() ? "true" : "false");

    // Write other tags
    writeOtherFields(addApp.otherFields());

    // Close additional app tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformDoc::Writer::writeCustomField(const CustomField& customField)
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
PlaylistDoc::PlaylistDoc(Install* const parent, const QString& xmlPath, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&) :
    Fe::BasicPlaylistDoc(parent, xmlPath, docName, updateOptions),
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
    QUuid key = lbPlaylistGame->id();
    if(mPlaylistGamesExisting.contains(key))
    {
        // Move LB playlist ID if applicable
        if(mUpdateOptions.importMode == Fe::ImportMode::NewAndExisting)
            lbPlaylistGame->setLBDatabaseId(std::static_pointer_cast<PlaylistGame>(mPlaylistGamesExisting[key])->lbDatabaseId());
    }
    else
        lbPlaylistGame->setLBDatabaseId(mLaunchBoxDatabaseIdTracker->reserveFirstFree());

    // Return converted playlist game
    return lbPlaylistGame;
}

//===============================================================================================================
// PlaylistDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDoc::Reader::Reader(PlaylistDoc* targetDoc) :
    Fe::DataDoc::Reader(targetDoc),
    Fe::BasicPlaylistDoc::Reader(targetDoc),
    XmlDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlaylistDoc::Reader::readTargetDoc()
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

void PlaylistDoc::Reader::parsePlaylistHeader()
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
    targetDocPlaylistHeader() = phb.buildShared();
}

void PlaylistDoc::Reader::parsePlaylistGame()
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
    std::shared_ptr<Lb::PlaylistGame> existingPlaylistGame = pgb.buildShared();

    // Correct LB ID if it is invalid and then add it to tracker
    if(existingPlaylistGame->lbDatabaseId() < 0)
        existingPlaylistGame->setLBDatabaseId(static_cast<PlaylistDoc*>(mTargetDocument)->mLaunchBoxDatabaseIdTracker->reserveFirstFree());
    else
        static_cast<PlaylistDoc*>(mTargetDocument)->mLaunchBoxDatabaseIdTracker->release(existingPlaylistGame->lbDatabaseId());

    // Add to document
    targetDocExistingPlaylistGames()[existingPlaylistGame->gameId()] = existingPlaylistGame;
}

//===============================================================================================================
// PlaylistDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistDoc::Writer::Writer(PlaylistDoc* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    Fe::BasicPlaylistDoc::Writer(sourceDoc),
    XmlDocWriter(sourceDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlaylistDoc::Writer::writeSourceDoc()
{
    // Write playlist header
    std::shared_ptr<Fe::PlaylistHeader> playlistHeader = static_cast<PlaylistDoc*>(mSourceDocument)->playlistHeader();
    if(!writePlaylistHeader(*std::static_pointer_cast<PlaylistHeader>(playlistHeader)))
        return false;

    // Write all playlist games
    for(std::shared_ptr<Fe::PlaylistGame> playlistGame : static_cast<PlaylistDoc*>(mSourceDocument)->finalPlaylistGames())
    {
        if(!writePlaylistGame(*std::static_pointer_cast<PlaylistGame>(playlistGame)))
            return false;
    }

    // Return true on success
    return true;
}

bool PlaylistDoc::Writer::writePlaylistHeader(const PlaylistHeader& playlistHeader)
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

bool PlaylistDoc::Writer::writePlaylistGame(const PlaylistGame& playlistGame)
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
PlatformsConfigDoc::PlatformsConfigDoc(Install* const parent, const QString& xmlPath, const DocKey&) :
    Fe::DataDoc(parent, xmlPath, STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Fe::DataDoc::Type PlatformsConfigDoc::type() const { return Fe::DataDoc::Type::Config; }

//Public:
bool PlatformsConfigDoc::isEmpty() const
{
    return mPlatforms.isEmpty() && mPlatformFolders.isEmpty() && mPlatformCategories.isEmpty();
}

const QHash<QString, Platform>& PlatformsConfigDoc::platforms() const { return mPlatforms; }
const QMap<QString, QMap<QString, QString>>& PlatformsConfigDoc::platformFolders() const { return mPlatformFolders; }
const QList<PlatformCategory>& PlatformsConfigDoc::platformCategories() const { return mPlatformCategories; }

bool PlatformsConfigDoc::containsPlatform(QString name) { return mPlatforms.contains(name); }

void PlatformsConfigDoc::addPlatform(Platform platform)
{
    // Add platform, don't need to add media folders as LB will automatically set them to the defaults
    mPlatforms[platform.name()] = platform;
}

void PlatformsConfigDoc::removePlatform(QString name)
{
    // Remove platform and any of its media folders (so LB will reset them to default)
    mPlatforms.remove(name);
    mPlatformFolders.remove(name);
}

void PlatformsConfigDoc::setMediaFolder(QString platform, QString mediaType, QString folderPath)
{
    // Add platform if it's missing as LB will not add it by default just because it has media folders
    if(!containsPlatform(platform))
    {
        Platform::Builder pb;
        pb.wName(platform);
        addPlatform(pb.build());
    }

    // Set/add media folder
    mPlatformFolders[platform][mediaType] = folderPath;
}

//===============================================================================================================
// PlatformsConfigDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsConfigDoc::Reader::Reader(PlatformsConfigDoc* targetDoc) :
    Fe::DataDoc::Reader(targetDoc),
    XmlDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformsConfigDoc::Reader::readTargetDoc()
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

void PlatformsConfigDoc::Reader::parsePlatform()
{
    // Platform Config Doc to Build
    Platform::Builder pb;

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
    static_cast<PlatformsConfigDoc*>(mTargetDocument)->mPlatforms.insert(existingPlatform->name(), *existingPlatform);
}

void PlatformsConfigDoc::Reader::parsePlatformFolder()
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
            mStreamReader.raiseError(Fe::docHandlingErrorString(mTargetDocument, Fe::DocHandlingError::DocInvalidType));
    }

    // Add to document
    static_cast<PlatformsConfigDoc*>(mTargetDocument)->mPlatformFolders[platform][mediaType] = folderPath;
}

void PlatformsConfigDoc::Reader::parsePlatformCategory()
{
    // Platform Config Doc to Build
    PlatformCategory::Builder pcb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        // No specific elements are of interest for now
        pcb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Playlist Header and add to document
   static_cast<PlatformsConfigDoc*>(mTargetDocument)->mPlatformCategories.append(pcb.build());
}

//===============================================================================================================
// PlatformsConfigDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformsConfigDoc::Writer::Writer(PlatformsConfigDoc* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    XmlDocWriter(sourceDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
bool PlatformsConfigDoc::Writer::writeSourceDoc()
{
    // Write all platforms
    for(const Platform& platform : static_cast<PlatformsConfigDoc*>(mSourceDocument)->platforms())
    {
        if(!writePlatform(platform))
            return false;
    }

    // Write all platform folders
    const QMap<QString, QMap<QString, QString>>& platformFolderMap = static_cast<PlatformsConfigDoc*>(mSourceDocument)->platformFolders();
    QMap<QString, QMap<QString, QString>>::const_iterator i;
    for(i = platformFolderMap.constBegin(); i != platformFolderMap.constEnd(); i++)
    {
         QMap<QString, QString>::const_iterator j;
         for(j = i.value().constBegin(); j != i.value().constEnd(); j++)
             if(!writePlatformFolder(i.key(), j.key(), j.value()))
                 return false;
    }

    // Write all platform categories
    for(const PlatformCategory& platformCategory : static_cast<PlatformsConfigDoc*>(mSourceDocument)->platformCategories())
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

    // Write other tags
    writeOtherFields(platform.otherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool PlatformsConfigDoc::Writer::writePlatformFolder(const QString& platform, const QString& mediaType, const QString& folderPath)
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

bool PlatformsConfigDoc::Writer::writePlatformCategory(const PlatformCategory& platformCategory)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Element_PlatformCategory::NAME);

    // Write known tags
    // None for now...
    if(mStreamWriter.hasError())
        return false;

    // Write other tags
    writeOtherFields(platformCategory.otherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

}
