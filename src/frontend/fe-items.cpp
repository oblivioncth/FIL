#include "fe-items.h"

namespace Fe
{

//===============================================================================================================
// ITEM
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Item::Item() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QHash<QString, QString>& Item::getOtherFields() { return mOtherFields; }
const QHash<QString, QString>& Item::getOtherFields() const { return mOtherFields; }

void Item::transferOtherFields(QHash<QString, QString>& otherFields) { mOtherFields = std::move(otherFields); }

//===============================================================================================================
// ITEM BUILDER
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
//ItemBuilder::ItemBuilder() { defined in .h }

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
//B& wOtherField(QPair<QString, QString> otherField) { defined in .h }
//T build() { defined in .h }

//===============================================================================================================
// GAME
//===============================================================================================================
//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid Game::getID() const { return mID; }
QString Game::getTitle() const { return mTitle; }
QString Game::getPlatform() const { return mPlatform; }

//===============================================================================================================
// GAME BUILDER
//===============================================================================================================

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
GameBuilder& GameBuilder::wID(QString rawID) { mItemBlueprint.mID = QUuid(rawID); return *this; }
GameBuilder& GameBuilder::wTitle(QString title) { mItemBlueprint.mTitle = title; return *this; }
GameBuilder& GameBuilder::wPlatform(QString platform) { mItemBlueprint.mPlatform = platform; return *this; }

//===============================================================================================================
// ADD APP
//===============================================================================================================

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid AddApp::getID() const { return mID; }
QUuid AddApp::getGameID() const { return mGameID; }
QString AddApp::getName() const { return mName; }

//===============================================================================================================
// ADD APP BUILDER
//===============================================================================================================

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
AddAppBuilder& AddAppBuilder::wID(QString rawID) { mItemBlueprint.mID = QUuid(rawID); return *this; }
AddAppBuilder& AddAppBuilder::wGameID(QString rawGameID) { mItemBlueprint.mGameID = QUuid(rawGameID); return *this; }
AddAppBuilder& AddAppBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this; }

//===============================================================================================================
// PLAYLIST HEADER
//===============================================================================================================

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistHeader::getPlaylistID() const { return mPlaylistID; }
QString PlaylistHeader::getName() const { return mName; }

//===============================================================================================================
// PLAYLIST HEADER BUILDER
//===============================================================================================================

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wPlaylistID(QString rawPlaylistID) { mItemBlueprint.mPlaylistID = QUuid(rawPlaylistID); return *this; }
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this;}

//===============================================================================================================
// PLAYLIST GAME
//===============================================================================================================

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistGame::getGameID() const { return mGameID; };

//===============================================================================================================
// PLAYLIST GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder::PlaylistGameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder& PlaylistGameBuilder::wGameID(QString rawGameID) { mItemBlueprint.mGameID = QUuid(rawGameID); return *this; }

}
