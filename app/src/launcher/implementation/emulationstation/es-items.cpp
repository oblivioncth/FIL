// Unit Include
#include "es-items.h"

namespace Es
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}
Game::Game(const Fp::Game& flashpointGame) :
    Lr::Game(flashpointGame.id(), flashpointGame.title(), flashpointGame.platformName())
{}

Game::Game(const Fp::AddApp& flashpointAddApp, const Fp::Game& parentGame) :
    Lr::Game(flashpointAddApp.id(), "FIXME", parentGame.platformName())
{}

//-Class Functions------------------------------------------------------------------------------------------------------
//Public:
QString Game::pathFromId(const QUuid& id) { return id.toString(QUuid::WithoutBraces); }
QUuid Game::idFromPath(const QString& path) { return QUuid::fromString(path); }

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Game::path() const { return mPath; }

//===============================================================================================================
// Game::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Game::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Game::Builder& Game::Builder::wPath(const QString& path) { mItemBlueprint.mPath = path; return *this; }

//===============================================================================================================
// Folder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Folder::Folder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Folder::path() const { return mPath; }

//===============================================================================================================
// Folder::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Folder::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Folder::Builder& Folder::Builder::wPath(const QString& path) { mItemBlueprint.mPath = path; return *this; }

//===============================================================================================================
// System
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
System::System() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString System::name() const { return mName; }
QString System::fullName() const { return mName; }
QString System::systemSortName() const { return mName; }
const QHash<QString, QString>& System::commands() const { return mCommands; }

//===============================================================================================================
// System::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
System::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
System::Builder& System::Builder::wName(const QString& name) { mItemBlueprint.mName = name; return *this; }
System::Builder& System::Builder::wFullName(const QString& name) { mItemBlueprint.mName = name; return *this; }
System::Builder& System::Builder::wSystemSortName(const QString& name) { mItemBlueprint.mName = name; return *this; }
System::Builder& System::Builder::wCommand(const QString& label, const QString& command) { mItemBlueprint.mCommands[label] = command; return *this; }

}
