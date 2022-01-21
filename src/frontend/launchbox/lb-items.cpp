#include "lb-items.h"
#include "../../flashpoint/fp-install.h"
#include "../../clifp.h"
#include "qx-io.h"

namespace LB
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}

Game::Game(FP::Game flashpointGame, QString fullCLIFpPath) :
    Fe::Game(flashpointGame.getId(), flashpointGame.getTitle(), flashpointGame.getPlatform()),
    mSeries(flashpointGame.getSeries()),
    mDeveloper(flashpointGame.getDeveloper()),
    mPublisher(flashpointGame.getPublisher()),
    mSortTitle(flashpointGame.getOrderTitle()),
    mDateAdded(flashpointGame.getDateAdded()),
    mDateModified(flashpointGame.getDateModified()),
    mBroken(flashpointGame.isBroken()),
    mPlayMode(flashpointGame.getPlayMode()),
    mStatus(flashpointGame.getStatus()),
    mRegion(), // Ensures this field is cleared because of older tool versions
    mNotes(flashpointGame.getOriginalDescription() +
           (!flashpointGame.getNotes().isEmpty() ? "\n\n" + flashpointGame.getNotes() : "")),
    mSource(flashpointGame.getSource()),
    mAppPath(QDir::toNativeSeparators(fullCLIFpPath)),
    mCommandLine(CLIFp::parametersFromStandard(flashpointGame.getId())),
    mReleaseDate(flashpointGame.getReleaseDate()),
    mVersion(flashpointGame.getVersion()),
    mReleaseType(flashpointGame.getLibrary() == FP::DB::Table_Game::ENTRY_GAME_LIBRARY ? RELEASE_TYPE_GAME : RELEASE_TYPE_ANIM)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Game::getTitle() const { return mName; } // Proxy for name
QString Game::getSeries() const { return mSeries; }
QString Game::getDeveloper() const { return mDeveloper; }
QString Game::getPublisher() const { return mPublisher; }
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
// GameBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
GameBuilder::GameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
GameBuilder& GameBuilder::wTitle(QString title) { mItemBlueprint.mName = title; return *this; } // Proxy for name
GameBuilder& GameBuilder::wSeries(QString series) { mItemBlueprint.mSeries = series; return *this; }
GameBuilder& GameBuilder::wDeveloper(QString developer) { mItemBlueprint.mDeveloper = developer; return *this; }
GameBuilder& GameBuilder::wPublisher(QString publisher) { mItemBlueprint.mPublisher = publisher; return *this; }
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
// AddApp
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
AddApp::AddApp() {}

AddApp::AddApp(FP::AddApp flashpointAddApp, QString fullCLIFpPath) :
    Fe::AddApp(flashpointAddApp.getID(), flashpointAddApp.getName(), flashpointAddApp.getParentID()),
    mAppPath(QDir::toNativeSeparators(fullCLIFpPath)),
    mCommandLine(flashpointAddApp.isPlayable() ? CLIFp::parametersFromStandard(mId)
                   : CLIFp::parametersFromStandard(flashpointAddApp.getAppPath(), flashpointAddApp.getLaunchCommand())),
    mAutorunBefore(false),
    mWaitForExit(flashpointAddApp.isWaitExit())
{}



//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString AddApp::getAppPath() const { return mAppPath; }
QString AddApp::getCommandLine() const { return mCommandLine; }
bool AddApp::isAutorunBefore() const { return mAutorunBefore; }
bool AddApp::isWaitForExit() const { return mWaitForExit; }

//===============================================================================================================
// AddAppBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
AddAppBuilder::AddAppBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
AddAppBuilder& AddAppBuilder::wAppPath(QString appPath) { mItemBlueprint.mAppPath = appPath; return *this; }
AddAppBuilder& AddAppBuilder::wCommandLine(QString commandLine) { mItemBlueprint.mCommandLine = commandLine; return *this; }
AddAppBuilder& AddAppBuilder::wAutorunBefore(QString rawAutorunBefore) { mItemBlueprint.mAutorunBefore = rawAutorunBefore != "0"; return *this; }
AddAppBuilder& AddAppBuilder::wWaitForExit(QString rawWaitForExit) { mItemBlueprint.mWaitForExit = rawWaitForExit != "0"; return *this; }

//===============================================================================================================
// CustomField
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
CustomField::CustomField() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid CustomField::getGameID() const { return mGameID; }
QString CustomField::getName() const { return mName; }
QString CustomField::getValue() const { return mValue; }

//===============================================================================================================
// CustomFieldBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
CustomFieldBuilder::CustomFieldBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
CustomFieldBuilder& CustomFieldBuilder::wGameID(QString rawGameID) { mItemBlueprint.mGameID = QUuid(rawGameID); return *this; }
CustomFieldBuilder& CustomFieldBuilder::wGameID(QUuid gameID) { mItemBlueprint.mGameID = gameID; return *this; }
CustomFieldBuilder& CustomFieldBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this;}
CustomFieldBuilder& CustomFieldBuilder::wValue(QString value) { mItemBlueprint.mValue = value; return *this;}

//===============================================================================================================
// PlaylistHeader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::PlaylistHeader() {}
PlaylistHeader::PlaylistHeader(FP::Playlist flashpointPlaylist) :
    Fe::PlaylistHeader(flashpointPlaylist.getID(), flashpointPlaylist.getTitle()),
    mNestedName(flashpointPlaylist.getTitle()),
    mNotes(flashpointPlaylist.getDescription())
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistHeader::getPlaylistId() const { return mId; } // Proxy for mId
QString PlaylistHeader::getNestedName() const { return mNestedName; }
QString PlaylistHeader::getNotes() const { return mNotes; }

//===============================================================================================================
// PlaylistHeaderBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder::PlaylistHeaderBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wPlaylistId(QString rawId) { mItemBlueprint.mId = QUuid(rawId); return *this; } // Proxy for Id
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wNestedName(QString nestedName) { mItemBlueprint.mNestedName = nestedName; return *this;}
PlaylistHeaderBuilder& PlaylistHeaderBuilder::wNotes(QString notes) { mItemBlueprint.mNotes = notes; return *this;}

//===============================================================================================================
// PlaylistGame::EntryDetails
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::EntryDetails::EntryDetails(const Game& refGame) :
    mTitle(refGame.getName()),
    mFilename(QFileInfo(refGame.getAppPath()).fileName()),
    mPlatform(refGame.getPlatform())
{}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
QString PlaylistGame::EntryDetails::title() const { return mTitle; }
QString PlaylistGame::EntryDetails::filename() const { return mFilename; }
QString PlaylistGame::EntryDetails::platform() const { return mTitle; }

//===============================================================================================================
// PlaylistGame
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::PlaylistGame(FP::PlaylistGame flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap) :
    Fe::PlaylistGame(flashpointPlaylistGame.getGameID(), playlistGameDetailsMap.value(flashpointPlaylistGame.getGameID()).title()),
    mLBDatabaseId(-1),
    mGameFilename(playlistGameDetailsMap.value(flashpointPlaylistGame.getGameID()).filename()),
    mGamePlatform(playlistGameDetailsMap.value(flashpointPlaylistGame.getGameID()).platform()),
    mManualOrder(flashpointPlaylistGame.getOrder())
{}

PlaylistGame::PlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString PlaylistGame::getGameTitle() const { return mName; } // Proxy for mName
int PlaylistGame::getLBDatabaseId() const { return mLBDatabaseId; }
QString PlaylistGame::getGameFileName() const { return mGameFilename; }
QString PlaylistGame::getGamePlatform() const { return mGamePlatform; }
int PlaylistGame::getManualOrder() const { return mManualOrder; }

void PlaylistGame::setLBDatabaseId(int lbDBID) { mLBDatabaseId = lbDBID; }

//===============================================================================================================
// PlaylistGameBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder::PlaylistGameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder& PlaylistGameBuilder::wGameTitle(QString gameTitle) { mItemBlueprint.mName = gameTitle; return *this;} // Proxy for mName

PlaylistGameBuilder& PlaylistGameBuilder::wLBDatabaseID(QString rawLBDatabaseID)
{
    bool validInt = false;
    mItemBlueprint.mLBDatabaseId = rawLBDatabaseID.toInt(&validInt);
    if(!validInt)
        mItemBlueprint.mLBDatabaseId = -1;
    return *this;
}

PlaylistGameBuilder& PlaylistGameBuilder::wGameFileName(QString gameFileName) { mItemBlueprint.mGameFilename = gameFileName; return *this; }
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
// Platform
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Platform::Platform() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
QString Platform::getName() const { return mName; }

//===============================================================================================================
// PlatformBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformBuilder::PlatformBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlatformBuilder& PlatformBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this; }

//===============================================================================================================
// PlatformFolder
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
// PlatformFolderBuilder
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
// PlatformCategory
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformCategory::PlatformCategory() {}

//===============================================================================================================
// PlatformCategoryBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformCategoryBuilder::PlatformCategoryBuilder() {}

}
