#include "launchbox.h"
#include "flashpoint-install.h"
#include "qx-io.h"

namespace LB
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

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}

Game::Game(FP::Game flashpointGame, QString fullCLIFpPath)
    : mID(flashpointGame.getID()),
      mTitle(flashpointGame.getTitle()),
      mSeries(flashpointGame.getSeries()),
      mDeveloper(flashpointGame.getDeveloper()),
      mPublisher(flashpointGame.getPublisher()),
      mPlatform(flashpointGame.getPlatform()),
      mSortTitle(flashpointGame.getOrderTitle()),
      mDateAdded(flashpointGame.getDateAdded()),
      mDateModified(flashpointGame.getDateModified()),
      mBroken(flashpointGame.isBroken()),
      mPlayMode(flashpointGame.getPlayMode()),
      mStatus(flashpointGame.getStatus()),
      mRegion(Qx::kosherizeFileName(flashpointGame.getLanguage().replace(':',';'))),
      // Some entries have a typo and since mRegion is used in folder creation the field must be kosher
      mNotes(flashpointGame.getOriginalDescription() +
             (!flashpointGame.getNotes().isEmpty() ? "\n\n" + flashpointGame.getNotes() : "")),
      mSource(flashpointGame.getSource()),
      mAppPath(QDir::toNativeSeparators(fullCLIFpPath)),
      mCommandLine(FP::Install::CLIFp::parametersFromStandard(flashpointGame.getID())),
      mReleaseDate(flashpointGame.getReleaseDate()),
      mVersion(flashpointGame.getVersion()),
      mReleaseType(flashpointGame.getLibrary() == FP::Install::DBTable_Game::ENTRY_GAME_LIBRARY ? RELEASE_TYPE_GAME : RELEASE_TYPE_ANIM) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid Game::getID() const { return mID; }
QString Game::getTitle() const { return mTitle; }
QString Game::getSeries() const { return mSeries; }
QString Game::getDeveloper() const { return mDeveloper; }
QString Game::getPublisher() const { return mPublisher; }
QString Game::getPlatform() const { return mPlatform; }
QString Game::getSortTitle() const { return mSortTitle; }
QDateTime Game::getDateAdded() const { return mDateAdded; }
QDateTime Game::getDateModified() const { return mDateModified; }
bool Game::isBroken() const { return mBroken; }
QString Game::getPlayMode() const { return mPlayMode; }
QString Game::getStatus() const { return mStatus; }
QString Game::getRegion() const { return mRegion; }
QString Game::getNotes() const { return mNotes; }
QString Game::getSource() const { return mSource; }
QString Game::getAppPath() const { return mAppPath; }
QString Game::getCommandLine() const { return mCommandLine; }
QDateTime Game::getReleaseDate() const { return mReleaseDate; }
QString Game::getVersion() const { return mVersion; }
QString Game::getReleaseType() const { return mReleaseType; }

//===============================================================================================================
// GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
GameBuilder::GameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
GameBuilder& GameBuilder::wID(QString rawID) { mItemBlueprint.mID = QUuid(rawID); return *this; }
GameBuilder& GameBuilder::wTitle(QString title) { mItemBlueprint.mTitle = title; return *this; }
GameBuilder& GameBuilder::wSeries(QString series) { mItemBlueprint.mSeries = series; return *this; }
GameBuilder& GameBuilder::wDeveloper(QString developer) { mItemBlueprint.mDeveloper = developer; return *this; }
GameBuilder& GameBuilder::wPublisher(QString publisher) { mItemBlueprint.mPublisher = publisher; return *this; }
GameBuilder& GameBuilder::wPlatform(QString platform) { mItemBlueprint.mPlatform = platform; return *this; }
GameBuilder& GameBuilder::wSortTitle(QString sortTitle) { mItemBlueprint.mSortTitle = sortTitle; return *this; }

GameBuilder& GameBuilder::wDateAdded(QString rawDateAdded)
{
    mItemBlueprint.mDateAdded = QDateTime::fromString(rawDateAdded, Qt::ISODateWithMs);
    return *this;
}

GameBuilder& GameBuilder::wDateModified(QString rawDateModified)
{
    mItemBlueprint.mDateModified = QDateTime::fromString(rawDateModified, Qt::ISODateWithMs);
    return *this;
}

GameBuilder& GameBuilder::wBroken(QString rawBroken) { mItemBlueprint.mBroken = rawBroken.toInt() != 0; return *this; }
GameBuilder& GameBuilder::wPlayMode(QString playMode) { mItemBlueprint.mPlayMode = playMode; return *this; }
GameBuilder& GameBuilder::wStatus(QString status) { mItemBlueprint.mStatus = status; return *this; }
GameBuilder& GameBuilder::wRegion(QString region) { mItemBlueprint.mRegion = region; return *this; }
GameBuilder& GameBuilder::wNotes(QString notes) { mItemBlueprint.mNotes = notes; return *this; }
GameBuilder& GameBuilder::wSource(QString source) { mItemBlueprint.mSource = source; return *this; }
GameBuilder& GameBuilder::wAppPath(QString appPath) { mItemBlueprint.mAppPath = appPath; return *this; }
GameBuilder& GameBuilder::wCommandLine(QString commandLine) { mItemBlueprint.mCommandLine = commandLine; return *this; }

GameBuilder& GameBuilder::wReleaseDate(QString rawReleaseDate)
{
    mItemBlueprint.mReleaseDate = QDateTime::fromString(rawReleaseDate, Qt::ISODateWithMs);
    return *this;
}

GameBuilder& GameBuilder::wVersion(QString version) { mItemBlueprint.mVersion = version; return *this; }
GameBuilder& GameBuilder::wReleaseType(QString releaseType) { mItemBlueprint.mReleaseType = releaseType; return *this; }

//===============================================================================================================
// ADD APP
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
AddApp::AddApp(FP::AddApp flashpointAddApp, QString fullCLIFpPath)
    : mID(flashpointAddApp.getID()),
      mGameID(flashpointAddApp.getParentID()),
      mAppPath(QDir::toNativeSeparators(fullCLIFpPath)),
      mCommandLine(flashpointAddApp.isPlayable() ? FP::Install::CLIFp::parametersFromStandard(mID)
                     : FP::Install::CLIFp::parametersFromStandard(flashpointAddApp.getAppPath(), flashpointAddApp.getLaunchCommand())),
      mAutorunBefore(false),
      mName(flashpointAddApp.getName()),
      mWaitForExit(flashpointAddApp.isWaitExit()) {}

AddApp::AddApp() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid AddApp::getID() const { return mID; }
QUuid AddApp::getGameID() const { return mGameID; }
QString AddApp::getAppPath() const { return mAppPath; }
QString AddApp::getCommandLine() const { return mCommandLine; }
bool AddApp::isAutorunBefore() const { return mAutorunBefore; }
QString AddApp::getName() const { return mName; }
bool AddApp::isWaitForExit() const { return mWaitForExit; }

//===============================================================================================================
// ADD APP BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
AddAppBuilder::AddAppBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
AddAppBuilder& AddAppBuilder::wID(QString rawID) { mItemBlueprint.mID = QUuid(rawID); return *this; }
AddAppBuilder& AddAppBuilder::wGameID(QString rawGameID) { mItemBlueprint.mGameID = QUuid(rawGameID); return *this; }
AddAppBuilder& AddAppBuilder::wAppPath(QString appPath) { mItemBlueprint.mAppPath = appPath; return *this; }
AddAppBuilder& AddAppBuilder::wCommandLine(QString commandLine) { mItemBlueprint.mCommandLine = commandLine; return *this; }
AddAppBuilder& AddAppBuilder::wAutorunBefore(QString rawAutorunBefore) { mItemBlueprint.mAutorunBefore = rawAutorunBefore != "0"; return *this; }
AddAppBuilder& AddAppBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this; }
AddAppBuilder& AddAppBuilder::wWaitForExit(QString rawWaitForExit) { mItemBlueprint.mWaitForExit = rawWaitForExit != "0"; return *this; }

//===============================================================================================================
// PLAYLIST HEADER
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::PlaylistHeader() {}
PlaylistHeader::PlaylistHeader(FP::Playlist flashpointPlaylist)
    : mPlaylistID(flashpointPlaylist.getID()),
      mName(flashpointPlaylist.getTitle()),
      mNestedName(flashpointPlaylist.getTitle()),
      mNotes(flashpointPlaylist.getDescription()) {}


//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistHeader::getPlaylistID() const { return mPlaylistID; }
QString PlaylistHeader::getName() const { return mName; }
QString PlaylistHeader::getNestedName() const { return mNestedName; }
QString PlaylistHeader::getNotes() const { return mNotes; }

//===============================================================================================================
// PLAYLIST HEADER BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder::PlaylistHeaderBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wPlaylistID(QString rawPlaylistID) { mItemBlueprint.mPlaylistID = QUuid(rawPlaylistID); return *this; }
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this;}
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wNestedName(QString nestedName) { mItemBlueprint.mNestedName = nestedName; return *this;}
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wNotes(QString notes) { mItemBlueprint.mNotes = notes; return *this;}

//===============================================================================================================
// PLAYLIST GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::PlaylistGame(FP::PlaylistGame flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap)
    : mGameID(flashpointPlaylistGame.getGameID()),
      mLBDatabaseID(-1),
      mGameTitle(playlistGameDetailsMap.value(mGameID).title),
      mGameFileName(playlistGameDetailsMap.value(mGameID).fileName),
      mGamePlatform(playlistGameDetailsMap.value(mGameID).platform),
      mManualOrder(flashpointPlaylistGame.getOrder()) {}

PlaylistGame::PlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistGame::getGameID() const { return mGameID; };
int PlaylistGame::getLBDatabaseID() const { return mLBDatabaseID; }
QString PlaylistGame::getGameTitle() const { return mGameTitle; }
QString PlaylistGame::getGameFileName() const { return mGameFileName; }
QString PlaylistGame::getGamePlatform() const { return mGamePlatform; }
int PlaylistGame::getManualOrder() const { return mManualOrder; }

void PlaylistGame::setLBDatabaseID(int lbDBID) { mLBDatabaseID = lbDBID; }

//===============================================================================================================
// PLAYLIST GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder::PlaylistGameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder& PlaylistGameBuilder::wGameID(QString rawGameID) { mItemBlueprint.mGameID = QUuid(rawGameID); return *this; }

PlaylistGameBuilder& PlaylistGameBuilder::wLBDatabaseID(QString rawLBDatabaseID)
{
    bool validInt = false;
    mItemBlueprint.mLBDatabaseID = rawLBDatabaseID.toInt(&validInt);
    if(!validInt)
        mItemBlueprint.mLBDatabaseID = -1;
    return *this;
}

PlaylistGameBuilder& PlaylistGameBuilder::wGameTitle(QString gameTitle) { mItemBlueprint.mGameTitle = gameTitle; return *this; }
PlaylistGameBuilder& PlaylistGameBuilder::wGameFileName(QString gameFileName) { mItemBlueprint.mGameFileName = gameFileName; return *this; }
PlaylistGameBuilder& PlaylistGameBuilder::wGamePlatform(QString gamePlatform) { mItemBlueprint.mGamePlatform = gamePlatform; return *this; }

PlaylistGameBuilder& PlaylistGameBuilder::wManualOrder(QString rawManualOrder)
{
    bool validInt = false;
    mItemBlueprint.mManualOrder = rawManualOrder.toInt(&validInt);
    if(!validInt)
        mItemBlueprint.mManualOrder = -1;
    return *this;
}

//===============================================================================================================
// PLATFORM
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Platform::Platform() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
QString Platform::getName() const { return mName; }

//===============================================================================================================
// PLATFORM BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformBuilder::PlatformBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlatformBuilder& PlatformBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this; }

//===============================================================================================================
// PLATFORM FOLDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
//PlatformFolder::PlatformFolder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
//QString PlatformFolder::getMediaType() { return mMediaType; }
//QString PlatformFolder::getFolderPath() { return mFolderPath; }
//QString PlatformFolder::getPlatform() { return mPlatform; }

//===============================================================================================================
// PLATFORM FOLDER BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
//PlatformFolderBuilder::PlatformFolderBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
//PlatformFolderBuilder& PlatformFolderBuilder::wMediaType(QString mediaType) { mItemBlueprint.mMediaType = mediaType; return *this; }
//PlatformFolderBuilder& PlatformFolderBuilder::wFolderPath(QString folderPath) { mItemBlueprint.mFolderPath = folderPath; return *this; }
//PlatformFolderBuilder& PlatformFolderBuilder::wPlatform(QString platform) { mItemBlueprint.mMediaType = platform; return *this; }

//===============================================================================================================
// PLATFORM CATEGORY
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformCategory::PlatformCategory() {}

//===============================================================================================================
// PLATFORM CATEGORY BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformCategoryBuilder::PlatformCategoryBuilder() {}

}
