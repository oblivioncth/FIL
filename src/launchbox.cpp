#include "launchbox.h"
#include "flashpointinstall.h"
#include "qx-io.h"

namespace LB
{

//===============================================================================================================
// GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}

Game::Game(FP::Game flashpointGame, QString fullOFLIbPath)
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
      mNotes(flashpointGame.getNotes()),
      mSource(flashpointGame.getSource()),
      mAppPath(fullOFLIbPath),
      mCommandLine(FP::Install::CLIFp::parametersFromStandard(flashpointGame.getAppPath(), flashpointGame.getLaunchCommand())),
      mReleaseDate(flashpointGame.getReleaseDate()),
      mVersion(flashpointGame.getVersion()) {}

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
QHash<QString, QString>& Game::getOtherFields() { return mOtherFields; }
const QHash<QString, QString>& Game::getOtherFields() const { return mOtherFields; }

void Game::transferOtherFields(QHash<QString, QString>& otherFields) { mOtherFields = std::move(otherFields); }

//===============================================================================================================
// GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
GameBuilder::GameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
GameBuilder& GameBuilder::wID(QString rawID) { mGameBlueprint.mID = QUuid(rawID); return *this; }
GameBuilder& GameBuilder::wTitle(QString title) { mGameBlueprint.mTitle = title; return *this; }
GameBuilder& GameBuilder::wSeries(QString series) { mGameBlueprint.mSeries = series; return *this; }
GameBuilder& GameBuilder::wDeveloper(QString developer) { mGameBlueprint.mDeveloper = developer; return *this; }
GameBuilder& GameBuilder::wPublisher(QString publisher) { mGameBlueprint.mPublisher = publisher; return *this; }
GameBuilder& GameBuilder::wPlatform(QString platform) { mGameBlueprint.mPlatform = platform; return *this; }
GameBuilder& GameBuilder::wSortTitle(QString sortTitle) { mGameBlueprint.mSortTitle = sortTitle; return *this; }

GameBuilder& GameBuilder::wDateAdded(QString rawDateAdded)
{
    mGameBlueprint.mDateAdded = QDateTime::fromString(rawDateAdded, Qt::ISODateWithMs);
    return *this;
}

GameBuilder& GameBuilder::wDateModified(QString rawDateModified)
{
    mGameBlueprint.mDateModified = QDateTime::fromString(rawDateModified, Qt::ISODateWithMs);
    return *this;
}

GameBuilder& GameBuilder::wBroken(QString rawBroken) { mGameBlueprint.mBroken = rawBroken.toInt() != 0; return *this; }
GameBuilder& GameBuilder::wPlayMode(QString playMode) { mGameBlueprint.mPlayMode = playMode; return *this; }
GameBuilder& GameBuilder::wStatus(QString status) { mGameBlueprint.mStatus = status; return *this; }
GameBuilder& GameBuilder::wRegion(QString region) { mGameBlueprint.mRegion = region; return *this; }
GameBuilder& GameBuilder::wNotes(QString notes) { mGameBlueprint.mNotes = notes; return *this; }
GameBuilder& GameBuilder::wSource(QString source) { mGameBlueprint.mSource = source; return *this; }
GameBuilder& GameBuilder::wAppPath(QString appPath) { mGameBlueprint.mAppPath = appPath; return *this; }
GameBuilder& GameBuilder::wCommandLine(QString commandLine) { mGameBlueprint.mCommandLine = commandLine; return *this; }

GameBuilder& GameBuilder::wReleaseDate(QString rawReleaseDate)
{
    mGameBlueprint.mReleaseDate = QDateTime::fromString(rawReleaseDate, Qt::ISODateWithMs);
    return *this;
}

GameBuilder& GameBuilder::wVersion(QString version) { mGameBlueprint.mVersion = version; return *this; }

GameBuilder& GameBuilder::wOtherField(QPair<QString, QString> otherField)
{
    mGameBlueprint.mOtherFields[otherField.first] = otherField.second;
    return *this;
}

Game GameBuilder::build() { return mGameBlueprint; }

//===============================================================================================================
// ADD APP
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
AddApp::AddApp(FP::AddApp flashpointAddApp, QString fullOFLIbPath)
    : mID(flashpointAddApp.getID()),
      mGameID(flashpointAddApp.getParentID()),
      mAppPath(fullOFLIbPath),
      mCommandLine(FP::Install::CLIFp::parametersFromStandard(flashpointAddApp.getAppPath(), flashpointAddApp.getLaunchCommand())),
      mAutorunBefore(flashpointAddApp.isAutorunBefore()),
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
QHash<QString, QString>& AddApp::getOtherFields() { return mOtherFields; }
const QHash<QString, QString>& AddApp::getOtherFields() const { return mOtherFields; }
void AddApp::transferOtherFields(QHash<QString, QString>& otherFields) { mOtherFields = std::move(otherFields); }

//===============================================================================================================
// ADD APP BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
AddAppBuilder::AddAppBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
AddAppBuilder& AddAppBuilder::wID(QString rawID) { mAddAppBlueprint.mID = QUuid(rawID); return *this; }
AddAppBuilder& AddAppBuilder::wGameID(QString rawGameID) { mAddAppBlueprint.mGameID = QUuid(rawGameID); return *this; }
AddAppBuilder& AddAppBuilder::wAppPath(QString appPath) { mAddAppBlueprint.mAppPath = appPath; return *this; }
AddAppBuilder& AddAppBuilder::wCommandLine(QString commandLine) { mAddAppBlueprint.mCommandLine = commandLine; return *this; }
AddAppBuilder& AddAppBuilder::wAutorunBefore(QString rawAutorunBefore) { mAddAppBlueprint.mAutorunBefore = rawAutorunBefore != 0; return *this; }
AddAppBuilder& AddAppBuilder::wName(QString name) { mAddAppBlueprint.mName = name; return *this; }
AddAppBuilder& AddAppBuilder::wWaitForExit(QString rawWaitForExit) { mAddAppBlueprint.mWaitForExit = rawWaitForExit != 0; return *this; }
AddAppBuilder& AddAppBuilder::wOtherField(QPair<QString, QString> otherField)
{
    mAddAppBlueprint.mOtherFields[otherField.first] = otherField.second;
    return *this;
}

AddApp AddAppBuilder::build() { return mAddAppBlueprint; }

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
QHash<QString, QString>& PlaylistHeader::getOtherFields() { return mOtherFields; }
const QHash<QString, QString>& PlaylistHeader::getOtherFields() const { return mOtherFields; }
void PlaylistHeader::transferOtherFields(QHash<QString, QString>& otherFields) { mOtherFields = std::move(otherFields); }

//===============================================================================================================
// PLAYLIST HEADER BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder::PlaylistHeaderBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wPlaylistID(QString rawPlaylistID) { mPlaylistHeaderBlueprint.mPlaylistID = QUuid(rawPlaylistID); return *this; }
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wName(QString name) { mPlaylistHeaderBlueprint.mName = name; return *this;}
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wNestedName(QString nestedName) { mPlaylistHeaderBlueprint.mNestedName = nestedName; return *this;}
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wNotes(QString notes) { mPlaylistHeaderBlueprint.mNotes = notes; return *this;}

PlaylistHeaderBuilder& PlaylistHeaderBuilder::wOtherField(QPair<QString, QString> otherField)
{
    mPlaylistHeaderBlueprint.mOtherFields[otherField.first] = otherField.second;
    return *this;
}

PlaylistHeader PlaylistHeaderBuilder::build() { return mPlaylistHeaderBlueprint; }


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
QHash<QString, QString>& PlaylistGame::getOtherFields() { return mOtherFields; }
const QHash<QString, QString>& PlaylistGame::getOtherFields() const { return mOtherFields; }

void PlaylistGame::transferOtherFields(QHash<QString, QString>& otherFields) { mOtherFields = std::move(otherFields); }
void PlaylistGame::setLBDatabaseID(int lbDBID) { mLBDatabaseID = lbDBID; }

//===============================================================================================================
// PLAYLIST GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder::PlaylistGameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder& PlaylistGameBuilder::wGameID(QString rawGameID) { mPlaylistGameBlueprint.mGameID = QUuid(rawGameID); return *this; }

PlaylistGameBuilder& PlaylistGameBuilder::wLBDatabaseID(QString rawLBDatabaseID)
{
    bool validInt = false;
    mPlaylistGameBlueprint.mLBDatabaseID = rawLBDatabaseID.toInt(&validInt);
    if(!validInt)
        mPlaylistGameBlueprint.mLBDatabaseID = -1;
    return *this;
}

PlaylistGameBuilder& PlaylistGameBuilder::wGameTitle(QString gameTitle) { mPlaylistGameBlueprint.mGameTitle = gameTitle; return *this; }
PlaylistGameBuilder& PlaylistGameBuilder::wGameFileName(QString gameFileName) { mPlaylistGameBlueprint.mGameFileName = gameFileName; return *this; }
PlaylistGameBuilder& PlaylistGameBuilder::wGamePlatform(QString gamePlatform) { mPlaylistGameBlueprint.mGamePlatform = gamePlatform; return *this; }

PlaylistGameBuilder& PlaylistGameBuilder::wManualOrder(QString rawManualOrder)
{
    bool validInt = false;
    mPlaylistGameBlueprint.mManualOrder = rawManualOrder.toInt(&validInt);
    if(!validInt)
        mPlaylistGameBlueprint.mManualOrder = -1;
    return *this;
}

PlaylistGameBuilder& PlaylistGameBuilder::wOtherField(QPair<QString, QString> otherField)
{
    mPlaylistGameBlueprint.mOtherFields[otherField.first] = otherField.second;
    return *this;
}

PlaylistGame PlaylistGameBuilder::build() { return mPlaylistGameBlueprint; }
}
