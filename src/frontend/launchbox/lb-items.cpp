// Unit Include
#include "lb-items.h"

// Project Includes
#include "../../clifp.h"

namespace Lb
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}

Game::Game(const Fp::Game& flashpointGame, QString fullCLIFpPath) :
    Fe::Game(flashpointGame.id(), flashpointGame.title(), flashpointGame.platform()),
    mSeries(flashpointGame.series()),
    mDeveloper(flashpointGame.developer()),
    mPublisher(flashpointGame.publisher()),
    mSortTitle(flashpointGame.orderTitle()),
    mDateAdded(flashpointGame.dateAdded()),
    mDateModified(flashpointGame.dateModified()),
    mBroken(flashpointGame.isBroken()),
    mPlayMode(flashpointGame.playMode()),
    mStatus(flashpointGame.status()),
    mRegion(), // Ensures this field is cleared because of older tool versions
    mNotes(flashpointGame.originalDescription() +
           (!flashpointGame.notes().isEmpty() ? "\n\n" + flashpointGame.notes() : "")),
    mSource(flashpointGame.source()),
    mAppPath(QDir::toNativeSeparators(fullCLIFpPath)),
    mCommandLine(CLIFp::parametersFromStandard(flashpointGame.id())),
    mReleaseDate(flashpointGame.releaseDate()),
    mVersion(flashpointGame.version()),
    mReleaseType(flashpointGame.library() == Fp::Db::Table_Game::ENTRY_GAME_LIBRARY ? RELEASE_TYPE_GAME : RELEASE_TYPE_ANIM)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Game::title() const { return mName; } // Proxy for name
QString Game::series() const { return mSeries; }
QString Game::developer() const { return mDeveloper; }
QString Game::publisher() const { return mPublisher; }
QString Game::sortTitle() const { return mSortTitle; }
QDateTime Game::dateAdded() const { return mDateAdded; }
QDateTime Game::dateModified() const { return mDateModified; }
bool Game::isBroken() const { return mBroken; }
QString Game::playMode() const { return mPlayMode; }
QString Game::status() const { return mStatus; }
QString Game::region() const { return mRegion; }
QString Game::notes() const { return mNotes; }
QString Game::source() const { return mSource; }
QString Game::appPath() const { return mAppPath; }
QString Game::commandLine() const { return mCommandLine; }
QDateTime Game::releaseDate() const { return mReleaseDate; }
QString Game::version() const { return mVersion; }
QString Game::releaseType() const { return mReleaseType; }

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

AddApp::AddApp(const Fp::AddApp& flashpointAddApp, QString fullCLIFpPath) :
    Fe::AddApp(flashpointAddApp.id(), flashpointAddApp.name(), flashpointAddApp.parentId()),
    mAppPath(QDir::toNativeSeparators(fullCLIFpPath)),
    mCommandLine(flashpointAddApp.isPlayable() ? CLIFp::parametersFromStandard(mId)
                   : CLIFp::parametersFromStandard(flashpointAddApp.appPath(), flashpointAddApp.launchCommand())),
    mAutorunBefore(false),
    mWaitForExit(flashpointAddApp.isWaitExit())
{}



//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString AddApp::appPath() const { return mAppPath; }
QString AddApp::commandLine() const { return mCommandLine; }
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
QUuid CustomField::gameId() const { return mGameId; }
QString CustomField::name() const { return mName; }
QString CustomField::value() const { return mValue; }

//===============================================================================================================
// CustomFieldBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
CustomFieldBuilder::CustomFieldBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
CustomFieldBuilder& CustomFieldBuilder::wGameId(QString rawGameId) { mItemBlueprint.mGameId = QUuid(rawGameId); return *this; }
CustomFieldBuilder& CustomFieldBuilder::wGameId(QUuid gameId) { mItemBlueprint.mGameId = gameId; return *this; }
CustomFieldBuilder& CustomFieldBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this;}
CustomFieldBuilder& CustomFieldBuilder::wValue(QString value) { mItemBlueprint.mValue = value; return *this;}

//===============================================================================================================
// PlaylistHeader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::PlaylistHeader() {}
PlaylistHeader::PlaylistHeader(const Fp::Playlist& flashpointPlaylist) :
    Fe::PlaylistHeader(flashpointPlaylist.id(), flashpointPlaylist.title()),
    mNestedName(flashpointPlaylist.title()),
    mNotes(flashpointPlaylist.description())
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistHeader::playlistId() const { return mId; } // Proxy for mId
QString PlaylistHeader::nestedName() const { return mNestedName; }
QString PlaylistHeader::notes() const { return mNotes; }

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
PlaylistGame::EntryDetails::EntryDetails() {}

PlaylistGame::EntryDetails::EntryDetails(const Game& refGame) :
    mTitle(refGame.name()),
    mFilename(QFileInfo(refGame.appPath()).fileName()),
    mPlatform(refGame.platform())
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
PlaylistGame::PlaylistGame(const Fp::PlaylistGame& flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap) :
    Fe::PlaylistGame(flashpointPlaylistGame.gameId(), playlistGameDetailsMap.value(flashpointPlaylistGame.gameId()).title()),
    mLBDatabaseId(-1),
    mGameFilename(playlistGameDetailsMap.value(flashpointPlaylistGame.gameId()).filename()),
    mGamePlatform(playlistGameDetailsMap.value(flashpointPlaylistGame.gameId()).platform()),
    mManualOrder(flashpointPlaylistGame.order())
{}

PlaylistGame::PlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString PlaylistGame::gameTitle() const { return mName; } // Proxy for mName
int PlaylistGame::lbDatabaseId() const { return mLBDatabaseId; }
QString PlaylistGame::gameFileName() const { return mGameFilename; }
QString PlaylistGame::gamePlatform() const { return mGamePlatform; }
int PlaylistGame::manualOrder() const { return mManualOrder; }

void PlaylistGame::setLBDatabaseId(int lbDbId) { mLBDatabaseId = lbDbId; }

//===============================================================================================================
// PlaylistGameBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder::PlaylistGameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistGameBuilder& PlaylistGameBuilder::wGameTitle(QString gameTitle) { mItemBlueprint.mName = gameTitle; return *this;} // Proxy for mName

PlaylistGameBuilder& PlaylistGameBuilder::wLBDatabaseId(QString rawLBDatabaseId)
{
    bool validInt = false;
    mItemBlueprint.mLBDatabaseId = rawLBDatabaseId.toInt(&validInt);
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
QString Platform::name() const { return mName; }

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
//QString PlatformFolder::mediaType() { return mMediaType; }
//QString PlatformFolder::folderPath() { return mFolderPath; }
//QString PlatformFolder::platform() { return mPlatform; }

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
