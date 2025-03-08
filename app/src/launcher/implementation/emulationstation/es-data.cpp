// Unit Include
#include "es-data.h"

// Project Includes
#include "launcher/implementation/emulationstation/es-install.h"
#include "import/backup.h"

namespace Xml
{

const QString GAMELIST_ROOT = u"gameList"_s;
const QString SYSTEMLIST_ROOT = u"systemList"_s;

namespace Game
{
    const QString ELEMENT = u"game"_s;

    const QString PATH = u"path"_s;
    const QString NAME = u"name"_s;
    const QString SORT_NAME = u"sortname"_s;
    const QString COLLECTION_SORT_NAME = u"collectionsortname"_s;
    const QString DESC = u"desc"_s;
    const QString RELEASE_DATE = u"releasedate"_s;
    const QString DEVELOPER = u"developer"_s;
    const QString PUBLISHER = u"publisher"_s;
    const QString GENRE = u"genre"_s;
    const QString PLAYERS = u"players"_s;
    const QString KID_GAME = u"kidgame"_s;
}

namespace Folder
{
    const QString ELEMENT = u"folder"_s;

    const QString PATH = u"path"_s;
}

namespace System
{
    const QString ELEMENT = u"system"_s;

    const QString NAME = u"name"_s;
    const QString FULL_NAME = u"fullname"_s;
    const QString SYSTEM_SORT_NAME = u"systemsortname"_s;
    const QString PATH = u"path"_s;
    const QString EXTENSION = u"extension"_s;
    const QString COMMAND = u"command"_s;
    const QString COMMAND_ATTR_LABEL = u"label"_s;
    const QString PLATFORM = u"platform"_s;
    const QString THEME = u"theme"_s;
}

namespace Setting
{
    const QString ATTR_NAME = u"name"_s;
    const QString ATTR_VALUE = u"value"_s;

    const QString ROM_DIRECTORY = u"ROMDirectory"_s;
    const QString COLLECTION_SYSTEMS_CUSTOM = u"CollectionSystemsCustom"_s;
}

}

namespace Es
{
//===============================================================================================================
// Gamelist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Gamelist::Gamelist(Install* install, const QString& xmlPath, const QString& fullSystemName, const Import::UpdateOptions& updateOptions) :
    Lr::PlatformDoc<LauncherId>(install, xmlPath, fullSystemName, updateOptions),
    mSystemName(System::fullNameToName(fullSystemName)),
    mGames(this)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
const Game* Gamelist::processSet(const Fp::Set& set)
{
    // Convert game to local entry and add
    const Game* addedGame = mGames.insert(Game(set.game(), set.tags(), mSystemName));

    // Cache system for ID
    install()->mPlaylistGameSystemNameCache[addedGame->id()] = mSystemName;

    // Handle additional apps
    for(const Fp::AddApp& addApp : set.addApps())
    {
        // Ignore if not playable
        if(addApp.isPlayable())
        {
            // Convert to game and add
            mGames.insert(Game(addApp, set.game(), set.tags(), mSystemName));

            // Cache system for ID
            install()->mPlaylistGameSystemNameCache[addApp.id()] = mSystemName;
        }
    }

    return addedGame;
}

//Public:
bool Gamelist::isEmpty() const { return mGames.isEmpty(); }
bool Gamelist::containsGame(const QUuid& gameId) const { return mGames.contains(gameId); }
bool Gamelist::containsAddApp(const QUuid& addAppId) const { return mGames.contains(addAppId); }

QString Gamelist::fullSystemName() const { return identifier().docName(); }
QString Gamelist::shortSystemName() const { return mSystemName; }

//===============================================================================================================
// GamelistReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
GamelistReader::GamelistReader(Gamelist* targetDoc) :
    Lr::XmlDocReader<Gamelist>(targetDoc, Xml::GAMELIST_ROOT)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Lr::DocHandlingError GamelistReader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Game::ELEMENT)
            parseGame();
        else if(mStreamReader.name() == Xml::Folder::ELEMENT)
            parseFolder();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return streamStatus();
}

void GamelistReader::parseGame()
{
    // Game to build
    Game::Builder gb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Game::PATH)
        {
            QString path = mStreamReader.readElementText();
            gb.wPath(path);
            gb.wId(Game::idFromFilename(QFileInfo(path).fileName()));
        }
        else if(mStreamReader.name() == Xml::Game::NAME)
            gb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::SORT_NAME)
            gb.wSortName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::COLLECTION_SORT_NAME)
            gb.wCollectionSortName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::DESC)
            gb.wDesc(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::RELEASE_DATE)
            gb.wReleaseDate(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::DEVELOPER)
            gb.wDeveloper(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::PUBLISHER)
            gb.wPublisher(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::GENRE)
            gb.wGenre(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::PLAYERS)
            gb.wPlayers(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::Game::KID_GAME)
            gb.wKidGame(mStreamReader.readElementText());
        else
            gb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Game and add to document
    target()->mGames.insert(gb.build());
}

void GamelistReader::parseFolder()
{
    // Additional App to Build
    Folder::Builder fb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::Folder::PATH)
            fb.wPath(mStreamReader.readElementText());
        else
            fb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build folder and add to document
    target()->mFolders.append(fb.build());
}

//===============================================================================================================
// GamelistWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
GamelistWriter::GamelistWriter(Gamelist* sourceDoc) :
    Lr::XmlDocWriter<Gamelist>(sourceDoc, Xml::GAMELIST_ROOT)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool GamelistWriter::writeSourceDoc()
{
    // Write all games
    if(!source()->mGames.forEachFinal([this](const Game& g){ return writeGame(g); }))
        return false;

    // Write all folders
    for(const auto& folder : std::as_const(source()->mFolders))
    {
        if(!writeFolder(folder))
            return false;
    }

    // Update dummy files
    if(!updateDummyFiles())
        return false;

    // Return true on success
    return true;
}

bool GamelistWriter::writeGame(const Game& game)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Game::ELEMENT);

    // Write known tags
    writeCleanTextElement(Xml::Game::PATH, game.path());
    writeCleanTextElement(Xml::Game::NAME, game.name());
    writeCleanTextElement(Xml::Game::SORT_NAME, game.sortName());
    writeCleanTextElement(Xml::Game::COLLECTION_SORT_NAME, game.collectionSortName());
    writeCleanTextElement(Xml::Game::DESC, game.desc());
    writeCleanTextElement(Xml::Game::RELEASE_DATE, game.releaseDate().toString(Qt::ISODate));
    writeCleanTextElement(Xml::Game::DEVELOPER, game.developer());
    writeCleanTextElement(Xml::Game::PUBLISHER, game.publisher());
    writeCleanTextElement(Xml::Game::GENRE, game.genre());
    writeCleanTextElement(Xml::Game::PLAYERS, game.players());
    writeCleanTextElement(Xml::Game::KID_GAME, game.kidGame() ? u"true"_s : u"false"_s);

    // Write other tags
    writeOtherFields(game);

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool GamelistWriter::writeFolder(const Folder& folder)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Folder::ELEMENT);

    // Write known tags
    writeCleanTextElement(Xml::Folder::PATH, folder.path());

    // Write other tags
    writeOtherFields(folder);

    // Close additional app tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

bool GamelistWriter::updateDummyFiles()
{
    const QDir romDir = source()->install()->romsDirectory();
    const QString systemFolder = source()->shortSystemName();
    const QDir systemRomDir = romDir.absoluteFilePath(systemFolder);
    auto bm = Import::BackupManager::instance();

    // Ensure system path exists
    if(!systemRomDir.mkpath(u"."_s))
        return false;

    // Remove obsolete dummy files
    if(!source()->mGames.forEachObsolete([&](const Game& obsGame){ return !bm->revertableRemove(systemRomDir.absoluteFilePath(obsGame.path())).isValid(); }))
        return false;

    // Add new dummy files
    if(!source()->mGames.forEachNew([&](const Game& newGame){ return !bm->revertableTouch(systemRomDir.absoluteFilePath(newGame.path())).isValid(); }))
        return false;

    return true;
}

//===============================================================================================================
// Collection
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Collection::Collection(Install* install, const QString& path, const QString& docName, const Import::UpdateOptions& updateOptions) :
    Lr::PlaylistDoc<LauncherId>(install, path, docName, updateOptions),
    mEntries(this)
{}

//-Class Functions------------------------------------------------------------------------------------------------------
//Public:
QString Collection::originalNameToName(const QString& original) { return NAME_PREFIX + ' ' + Qx::kosherizeFileName(original); }

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Collection::isEmpty() const { return mEntries.isEmpty(); };

void Collection::setPlaylistData(const Fp::Playlist& playlist)
{
    const auto& pgSnCache = install()->mPlaylistGameSystemNameCache;
    for(auto& pg : playlist.playlistGames())
    {
        auto id = pg.gameId();
        if(!pgSnCache.contains(id))
            qWarning("Added playlist game ID was not in the short-system-name cache! Skipping...");
        else
            mEntries.insert(CollectionEntry(id, pgSnCache.value(id)));
    }
}

bool Collection::containsPlaylistGame(const QUuid& gameId) const { return mEntries.contains(gameId); }
QString Collection::name() const { return identifier().docName(); }

//===============================================================================================================
// CollectionReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CollectionReader::CollectionReader(Collection* targetDoc) :
    Lr::DataDocReader<Collection>(targetDoc),
    mReader(targetDoc->path())
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError CollectionReader::readInto()
{
    // Open File
    if(auto res = mReader.openFile())
        return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocCantOpen, res.outcomeInfo());

    // Read paths
    while(!mReader.atEnd())
    {
        QStringList splitRawEntry = mReader.readLine().split(u'/');
        if(splitRawEntry.size() == 3)
        {
            QString systemShortName = splitRawEntry.at(1);
            QUuid id = Game::idFromFilename(splitRawEntry.at(2));
            if(!id.isNull())
            {
                target()->mEntries.insert(CollectionEntry(id, systemShortName));
                continue;
            }
        }

        return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocReadFailed, ERR_INVALID_ENTRY);
    }

    return mReader.hasError() ? Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocReadFailed, mReader.status().outcomeInfo()) :
                                Lr::DocHandlingError();
}

//===============================================================================================================
// CollectionWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CollectionWriter::CollectionWriter(Collection* sourceDoc) :
    Lr::DataDocWriter<Collection>(sourceDoc),
    mWriter(sourceDoc->path(), Qx::WriteMode::Truncate)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError CollectionWriter::writeOutOf()
{
    // Open File
    if(auto res = mWriter.openFile())
        return Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocCantOpen, res.outcomeInfo());

    // Write paths
    source()->mEntries.forEachFinal([this](const CollectionEntry& ce){
        const QString raw = Install::ROMPATH_PLACEHOLDER + '/' + ce.systemName() + '/' + Game::filenameFromId(ce.id());
        mWriter.writeLine(raw);
    });

    return mWriter.hasError() ? Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocWriteFailed, mWriter.status().outcomeInfo()) :
           Lr::DocHandlingError();
}

//===============================================================================================================
// Systemlist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Systemlist::Systemlist(Install* install, const QString& path) :
    Lr::DataDoc<LauncherId>(install, path, STD_NAME) // Technically, there are other system lists but currently we only need to open one
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Systemlist::isEmpty() const { return mSystems.isEmpty(); };
Lr::IDataDoc::Type Systemlist::type() const { return Lr::IDataDoc::Type::Config; }
const QHash<QString, System>& Systemlist::systems() const { return mSystems; }
void Systemlist::removeSystem(const QString& fullName) { mSystems.remove(fullName); }
void Systemlist::insertSystem(const System& system) { mSystems[system.fullName()] = system; }

//===============================================================================================================
// SystemlistReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
SystemlistReader::SystemlistReader(Systemlist* targetDoc) :
    Lr::XmlDocReader<Systemlist>(targetDoc, Xml::SYSTEMLIST_ROOT)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError SystemlistReader::readTargetDoc()
{
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::System::ELEMENT)
            parseSystem();
        else
            mStreamReader.skipCurrentElement();
    }

    // Return status
    return streamStatus();
}

void SystemlistReader::parseSystem()
{
    // Game to build
    System::Builder sb;

    // Cover all children
    while(mStreamReader.readNextStartElement())
    {
        if(mStreamReader.name() == Xml::System::NAME)
            sb.wName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::FULL_NAME)
            sb.wFullName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::SYSTEM_SORT_NAME)
            sb.wSystemSortName(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::PATH)
            sb.wPath(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::EXTENSION)
            sb.wExtension(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::COMMAND)
            sb.wCommand(mStreamReader.attributes().value(Xml::System::COMMAND_ATTR_LABEL).toString(), mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::PLATFORM)
            sb.wPlatform(mStreamReader.readElementText());
        else if(mStreamReader.name() == Xml::System::THEME)
            sb.wTheme(mStreamReader.readElementText());
        else
            sb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build System and add to document
    System existingSystem = sb.build();
    target()->mSystems[existingSystem.fullName()] = existingSystem;
}

//===============================================================================================================
// SystemlistWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
SystemlistWriter::SystemlistWriter(Systemlist* sourceDoc) :
    Lr::XmlDocWriter<Systemlist>(sourceDoc, Xml::SYSTEMLIST_ROOT)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool SystemlistWriter::writeSourceDoc()
{
    // Write all systems
    for(const auto& system : std::as_const(source()->mSystems))
    {
        if(!writeSystem(system))
            return false;
    }

    // Return true on success
    return true;
}

bool SystemlistWriter::writeSystem(const System& system)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::System::ELEMENT);

    // Write known tags
    writeCleanTextElement(Xml::System::NAME, system.name());
    writeCleanTextElement(Xml::System::FULL_NAME, system.fullName());
    writeCleanTextElement(Xml::System::SYSTEM_SORT_NAME, system.systemSortName());
    writeCleanTextElement(Xml::System::PATH, system.path());
    writeCleanTextElement(Xml::System::EXTENSION, system.extension());
    for(auto [label, text] : system.commands().asKeyValueRange())
    {
        if(!label.isEmpty())
        {
            QXmlStreamAttributes atrs;
            atrs.append(Xml::System::COMMAND_ATTR_LABEL, label);
            writeCleanTextElement(Xml::System::COMMAND, text, atrs);
        }
        else
            writeCleanTextElement(Xml::System::COMMAND, text);
    }
    writeCleanTextElement(Xml::System::PLATFORM, system.platform());
    writeCleanTextElement(Xml::System::THEME, system.theme());

    // Write other tags
    writeOtherFields(system);

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// Settings
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Settings::Settings(Install* install, const QString& path) :
    Lr::DataDoc<LauncherId>(install, path, STD_NAME) // Technically, there are other system lists but currently we only need to open one
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Settings::isEmpty() const { return false; };
Lr::IDataDoc::Type Settings::type() const { return Lr::IDataDoc::Type::Config; }
QString Settings::romDirectory() const { return mRomDirectory; }
bool Settings::containsCustomCollection(const QString& collectionName) const { return mCollectionSystemsCustom.contains(collectionName); }
void Settings::addCustomCollection(const QString& collectionName) { mCollectionSystemsCustom.append(collectionName); }

//===============================================================================================================
// SettingsReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
SettingsReader::SettingsReader(Settings* targetDoc) :
    Lr::XmlDocReader<Settings>(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError SettingsReader::readTargetDoc()
{
    // We only do a partial parse of this document for what we need, but that's OK
    while(mStreamReader.readNextStartElement())
        parseSetting();

    // Return status
    return streamStatus();
}

void SettingsReader::parseSetting()
{
    // Ensure null string (vs empty) if attr DNE
    const QString type = mStreamReader.name().toString();
    const QXmlStreamAttributes attrs = mStreamReader.attributes();
    const QString name = attrs.hasAttribute(Xml::Setting::ATTR_NAME) ? attrs.value(Xml::Setting::ATTR_NAME).toString() : QString();
    const QString value = attrs.hasAttribute(Xml::Setting::ATTR_VALUE) ? attrs.value(Xml::Setting::ATTR_VALUE).toString() : QString();

    Setting s(type == u"string"_s ? u"QString"_s : type, name, value);
    if(!s.isValid())
    {
        mStreamReader.raiseError(ERR_INVALID.arg(type, name, value));
        return;
    }

    // Check for known settings (make helper function or macro if more known values)
    if(s.name() == Xml::Setting::ROM_DIRECTORY)
    {
        if(!s.get(target()->mRomDirectory))
            mStreamReader.raiseError(ERR_KNOWN_CONVERT.arg(type, name, value));

        return;
    }
    else if(s.name() == Xml::Setting::COLLECTION_SYSTEMS_CUSTOM)
    {
        QString list;
        if(!s.get(list))
            mStreamReader.raiseError(ERR_KNOWN_CONVERT.arg(type, name, value));
        else
            target()->mCollectionSystemsCustom = list.split(u',');

        return;
    }

    // Record other setting
    target()->mOtherSettings.append(s);
}

//===============================================================================================================
// SettingsWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
SettingsWriter::SettingsWriter(Settings* sourceDoc) :
    Lr::XmlDocWriter<Settings>(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool SettingsWriter::writeSourceDoc()
{
    // Write known settings
    writeSetting(Setting::make<QString>(Xml::Setting::ROM_DIRECTORY, source()->mRomDirectory));
    writeSetting(Setting::make<QString>(Xml::Setting::COLLECTION_SYSTEMS_CUSTOM, source()->mCollectionSystemsCustom.join(u',')));

    // Write other settings
    for(const auto& setting : std::as_const(source()->mOtherSettings))
    {
        if(!writeSetting(setting))
            return false;
    }

    // Return true on success
    return true;
}

bool SettingsWriter::writeSetting(const Setting& setting)
{
    // Write type, name, and value
    QString type = setting.type();
    mStreamWriter.writeStartElement(type == u"QString"_s ? u"string"_s : type);
    mStreamWriter.writeAttribute(Xml::Setting::ATTR_NAME, setting.name());
    mStreamWriter.writeAttribute(Xml::Setting::ATTR_VALUE, setting.value());
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

}
