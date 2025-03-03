// Unit Include
#include "lr-items-interface.h"

namespace Lr
{

//===============================================================================================================
// ITEM
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Item::Item() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
bool Item::hasOtherFields() const { return !mOtherFields.isEmpty(); }
QHash<QString, QString>& Item::otherFields() { return mOtherFields; }
const QHash<QString, QString>& Item::otherFields() const { return mOtherFields; }

void Item::transferOtherFields(QHash<QString, QString>& otherFields) { mOtherFields = std::move(otherFields); }

//===============================================================================================================
// BASIC ITEM
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
BasicItem::BasicItem() {}

BasicItem::BasicItem(QUuid id, QString name) :
    mId(id),
    mName(name)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid BasicItem::id() const { return mId; }
QString BasicItem::name() const { return mName; }

//===============================================================================================================
// GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}
Game::Game(QUuid id, QString name, QString platform) :
    BasicItem(id, name),
    mPlatform(platform)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Game::platform() const { return mPlatform; }

//===============================================================================================================
// ADD APP
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
AddApp::AddApp() {}

AddApp::AddApp(QUuid id, QString name, QUuid gameId) :
    BasicItem(id, name),
    mGameId(gameId)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid AddApp::gameId() const { return mGameId; }

//===============================================================================================================
// PLAYLIST HEADER
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::PlaylistHeader() {}

PlaylistHeader::PlaylistHeader(QUuid id, QString name) :
    BasicItem(id, name)
{}

//===============================================================================================================
// PLAYLIST GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::PlaylistGame() {}

PlaylistGame::PlaylistGame(QUuid id, QString name) :
    BasicItem(id, name)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistGame::gameId() const { return mId; } // Proxy for id()

}
