// Unit Include
#include "es-data.h"

// Project Includes
#include "launcher/implementation/emulationstation/es-install.h"

namespace Xml
{

const QString GAMELIST_ROOT = u"gameList"_s;
const QString SYSTEMLIST_ROOT = u"systemList"_s;

namespace Game
{
    const QString ELEMENT = u"game"_s;

    const QString PATH = u"path"_s;
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
    const QString COMMAND = u"command"_s;
    const QString COMMAND_ATTR_LABEL = u"label"_s;
}

}

namespace Es
{
//===============================================================================================================
// Gamelist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Gamelist::Gamelist(Install* install, const QString& xmlPath, QString docName, const Import::UpdateOptions& updateOptions) :
    Lr::PlatformDoc<LauncherId>(install, xmlPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
std::shared_ptr<Game> Gamelist::processSet(const Fp::Set& set)
{
    // Convert game to local entry
    std::shared_ptr<Game> mainGameEntry = std::make_shared<Game>(set.game());

    // Add entry
    addUpdateableItem(mGamesExisting, mGamesFinal, mainGameEntry);

    // Handle additional apps
    for(const Fp::AddApp& addApp : set.addApps())
    {
        // Ignore if not playable
        if(addApp.isPlayable())
        {
            // Convert to game
            std::shared_ptr<Game> subGameEntry = std::make_shared<Game>(addApp, set.game());

            // Add entry
            addUpdateableItem(mGamesExisting, mGamesFinal, subGameEntry);
        }
    }

    return mainGameEntry;
}

//Public:
bool Gamelist::isEmpty() const { return mGamesExisting.isEmpty() && mGamesFinal.isEmpty(); }

void Gamelist::finalize()
{
    finalizeUpdateableItems(mGamesExisting, mGamesFinal);
    Lr::IUpdateableDoc::finalize();
}

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
        if(mStreamReader.name() == Xml::Folder::PATH)
        {
            QString path = mStreamReader.readElementText();
            gb.wPath(path);
            gb.wId(Game::idFromPath(path));
        }
        else
            gb.wOtherField({mStreamReader.name().toString(), mStreamReader.readElementText()});
    }

    // Build Game and add to document
    std::shared_ptr<Game> existingGame = gb.buildShared();
    target()->mGamesExisting[existingGame->id()] = existingGame;
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
    Folder existingFolder = fb.build();
    target()->mFolders.append(existingFolder);
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
    for(auto game : std::as_const(source()->mGamesFinal))
    {
        if(!writeGame(*game))
            return false;
    }

    // Write all folders
    for(const auto& folder : std::as_const(source()->mFolders))
    {
        if(!writeFolder(folder))
            return false;
    }

    // Return true on success
    return true;
}

bool GamelistWriter::writeGame(const Game& game)
{
    // Write opening tag
    mStreamWriter.writeStartElement(Xml::Game::ELEMENT);

    // Write known tags
    // TODO: Store path as ID only and convert during write and read
    writeCleanTextElement(Xml::Game::PATH, game.path());

    // Write other tags
    writeOtherFields(game.otherFields());

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
    writeOtherFields(folder.otherFields());

    // Close additional app tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// Collection
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Collection::Collection(Install* install, const QString& path, QString docName, const Import::UpdateOptions& updateOptions) :
    Lr::PlaylistDoc<LauncherId>(install, path, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Collection::isEmpty() const { return mEntriesExisting.isEmpty() && mEntriesFinal.isEmpty(); };

void Collection::finalize()
{
    finalizeUpdateableData(mEntriesExisting, mEntriesFinal);
    Lr::IUpdateableDoc::finalize();
};

void Collection::setPlaylistData(const Fp::Playlist& playlist)
{
    for(auto& pg : playlist.playlistGames())
        addUpdateableData(mEntriesExisting, mEntriesFinal, Game::pathFromId(pg.gameId()));
}

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
        // TODO: Detection of corrupted ID's (would need to store as ID)
        target()->mEntriesExisting.insert(mReader.readLine());
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
    mWriter(sourceDoc->path())
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError CollectionWriter::writeOutOf()
{
    // Open File
    if(auto res = mWriter.openFile())
        return Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocCantOpen, res.outcomeInfo());

    // Write paths
    for(const auto& p : std::as_const(source()->mEntriesFinal))
        mWriter.writeLine(p);

    return mWriter.hasError() ? Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocWriteFailed, mWriter.status().outcomeInfo()) :
           Lr::DocHandlingError();
}

//===============================================================================================================
// Systemlist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Systemlist::Systemlist(Install* install, const QString& path, QString docName) :
    Lr::DataDoc<LauncherId>(install, path, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Systemlist::isEmpty() const { return mSystems.isEmpty(); };
Lr::IDataDoc::Type Systemlist::type() const { return Lr::IDataDoc::Type::Config; }

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
        else if(mStreamReader.name() == Xml::System::COMMAND)
            sb.wCommand(mStreamReader.attributes().value(Xml::System::COMMAND_ATTR_LABEL).toString(), mStreamReader.readElementText());
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

    // Write other tags
    writeOtherFields(system.otherFields());

    // Close game tag
    mStreamWriter.writeEndElement();

    // Return error status
    return !mStreamWriter.hasError();
}
}
