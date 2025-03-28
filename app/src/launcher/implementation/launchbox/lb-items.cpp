// Unit Include
#include "lb-items.h"

// Project Includes
#include "kernel/clifp.h"

namespace Lb
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}

Game::Game(const Fp::Game& flashpointGame, const QString& fullCLIFpPath) :
    Lr::Game(flashpointGame.id(), flashpointGame.title(), flashpointGame.platformName()),
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
           (!flashpointGame.notes().isEmpty() ? u"\n\n"_s + flashpointGame.notes() : u""_s)),
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
// Game::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Game::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Game::Builder& Game::Builder::wTitle(const QString& title) { mBlueprint.mName = title; return *this; } // Proxy for name
Game::Builder& Game::Builder::wSeries(const QString& series) { mBlueprint.mSeries = series; return *this; }
Game::Builder& Game::Builder::wDeveloper(const QString& developer) { mBlueprint.mDeveloper = developer; return *this; }
Game::Builder& Game::Builder::wPublisher(const QString& publisher) { mBlueprint.mPublisher = publisher; return *this; }
Game::Builder& Game::Builder::wSortTitle(const QString& sortTitle) { mBlueprint.mSortTitle = sortTitle; return *this; }

Game::Builder& Game::Builder::wDateAdded(const QString& rawDateAdded)
{
    mBlueprint.mDateAdded = QDateTime::fromString(rawDateAdded, Qt::ISODateWithMs);
    return *this;
}

Game::Builder& Game::Builder::wDateModified(const QString& rawDateModified)
{
    mBlueprint.mDateModified = QDateTime::fromString(rawDateModified, Qt::ISODateWithMs);
    return *this;
}

Game::Builder& Game::Builder::wBroken(const QString& rawBroken) { mBlueprint.mBroken = rawBroken.toInt() != 0; return *this; }
Game::Builder& Game::Builder::wPlayMode(const QString& playMode) { mBlueprint.mPlayMode = playMode; return *this; }
Game::Builder& Game::Builder::wStatus(const QString& status) { mBlueprint.mStatus = status; return *this; }
Game::Builder& Game::Builder::wRegion(const QString& region) { mBlueprint.mRegion = region; return *this; }
Game::Builder& Game::Builder::wNotes(const QString& notes) { mBlueprint.mNotes = notes; return *this; }
Game::Builder& Game::Builder::wSource(const QString& source) { mBlueprint.mSource = source; return *this; }
Game::Builder& Game::Builder::wAppPath(const QString& appPath) { mBlueprint.mAppPath = appPath; return *this; }
Game::Builder& Game::Builder::wCommandLine(const QString& commandLine) { mBlueprint.mCommandLine = commandLine; return *this; }

Game::Builder& Game::Builder::wReleaseDate(const QString& rawReleaseDate)
{
    mBlueprint.mReleaseDate = QDateTime::fromString(rawReleaseDate, Qt::ISODateWithMs);
    return *this;
}

Game::Builder& Game::Builder::wVersion(const QString& version) { mBlueprint.mVersion = version; return *this; }
Game::Builder& Game::Builder::wReleaseType(const QString& releaseType) { mBlueprint.mReleaseType = releaseType; return *this; }

//===============================================================================================================
// AddApp
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
AddApp::AddApp() {}

AddApp::AddApp(const Fp::AddApp& flashpointAddApp, const QString& fullCLIFpPath) :
    Lr::AddApp(flashpointAddApp.id(), flashpointAddApp.name(), flashpointAddApp.parentId()),
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
// AddApp::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
AddApp::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
AddApp::Builder& AddApp::Builder::wAppPath(const QString& appPath) { mBlueprint.mAppPath = appPath; return *this; }
AddApp::Builder& AddApp::Builder::wCommandLine(const QString& commandLine) { mBlueprint.mCommandLine = commandLine; return *this; }
AddApp::Builder& AddApp::Builder::wAutorunBefore(const QString& rawAutorunBefore) { mBlueprint.mAutorunBefore = rawAutorunBefore != u"0"_s; return *this; }
AddApp::Builder& AddApp::Builder::wWaitForExit(const QString& rawWaitForExit) { mBlueprint.mWaitForExit = rawWaitForExit != u"0"_s; return *this; }

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
// CustomField::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
CustomField::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
CustomField::Builder& CustomField::Builder::wGameId(const QString& rawGameId) { mBlueprint.mGameId = QUuid(rawGameId); return *this; }
CustomField::Builder& CustomField::Builder::wGameId(const QUuid& gameId) { mBlueprint.mGameId = gameId; return *this; }
CustomField::Builder& CustomField::Builder::wName(const QString& name) { mBlueprint.mName = name; return *this;}
CustomField::Builder& CustomField::Builder::wValue(const QString& value) { mBlueprint.mValue = value; return *this;}

//===============================================================================================================
// PlaylistHeader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::PlaylistHeader() {}
PlaylistHeader::PlaylistHeader(const Fp::Playlist& flashpointPlaylist) :
    Lr::PlaylistHeader(flashpointPlaylist.id(), flashpointPlaylist.title()),
    mNestedName(flashpointPlaylist.title()),
    mNotes(flashpointPlaylist.description())
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid PlaylistHeader::playlistId() const { return mId; } // Proxy for mId
QString PlaylistHeader::nestedName() const { return mNestedName; }
QString PlaylistHeader::notes() const { return mNotes; }

//===============================================================================================================
// PlaylistHeader::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistHeader::Builder& PlaylistHeader::Builder::wPlaylistId(const QString& rawId) { mBlueprint.mId = QUuid(rawId); return *this; } // Proxy for Id
PlaylistHeader::Builder& PlaylistHeader::Builder::wNestedName(const QString& nestedName) { mBlueprint.mNestedName = nestedName; return *this;}
PlaylistHeader::Builder& PlaylistHeader::Builder::wNotes(const QString& notes) { mBlueprint.mNotes = notes; return *this;}

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
QString PlaylistGame::EntryDetails::platform() const { return mPlatform; }

//===============================================================================================================
// PlaylistGame
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::PlaylistGame(const Fp::PlaylistGame& flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap) :
    Lr::PlaylistGame(flashpointPlaylistGame.gameId(), playlistGameDetailsMap.value(flashpointPlaylistGame.gameId()).title()),
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
// PlaylistGame::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistGame::Builder& PlaylistGame::Builder::wGameTitle(const QString& gameTitle) { mBlueprint.mName = gameTitle; return *this;} // Proxy for mName

PlaylistGame::Builder& PlaylistGame::Builder::wLBDatabaseId(const QString& rawLBDatabaseId)
{
    bool validInt = false;
    mBlueprint.mLBDatabaseId = rawLBDatabaseId.toInt(&validInt);
    if(!validInt)
        mBlueprint.mLBDatabaseId = -1;
    return *this;
}

PlaylistGame::Builder& PlaylistGame::Builder::wGameFileName(const QString& gameFileName) { mBlueprint.mGameFilename = gameFileName; return *this; }
PlaylistGame::Builder& PlaylistGame::Builder::wGamePlatform(const QString& gamePlatform) { mBlueprint.mGamePlatform = gamePlatform; return *this; }

PlaylistGame::Builder& PlaylistGame::Builder::wManualOrder(const QString& rawManualOrder)
{
    bool validInt = false;
    mBlueprint.mManualOrder = rawManualOrder.toInt(&validInt);
    if(!validInt)
        mBlueprint.mManualOrder = -1;
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
//QString Platform::category() const { return mCategory; }

//===============================================================================================================
// Platform::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Platform::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Platform::Builder& Platform::Builder::wName(const QString& name) { mBlueprint.mName = name; return *this; }
//Platform::Builder& Platform::Builder::wCategory(const QString& category) { mBlueprint.mCategory = category; return *this; }

//===============================================================================================================
// PlatformFolder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformFolder::PlatformFolder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
QString PlatformFolder::mediaType() const { return mMediaType; }
QString PlatformFolder::folderPath() const { return mFolderPath; }
QString PlatformFolder::platform() const { return mPlatform; }
QString PlatformFolder::identifier() const { return mPlatform + mMediaType; }

//===============================================================================================================
// PlatformFolder::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformFolder::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlatformFolder::Builder& PlatformFolder::Builder::wMediaType(const QString& mediaType) { mBlueprint.mMediaType = mediaType; return *this; }
PlatformFolder::Builder& PlatformFolder::Builder::wFolderPath(const QString& folderPath) { mBlueprint.mFolderPath = folderPath; return *this; }
PlatformFolder::Builder& PlatformFolder::Builder::wPlatform(const QString& platform) { mBlueprint.mMediaType = platform; return *this; }

//===============================================================================================================
// PlatformCategory
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformCategory::PlatformCategory() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
QString PlatformCategory::name() const { return mName; }
QString PlatformCategory::nestedName() const { return mNestedName; }

//===============================================================================================================
// PlatformCategory::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlatformCategory::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlatformCategory::Builder& PlatformCategory::Builder::wName(const QString& name) { mBlueprint.mName = name; return *this; }
PlatformCategory::Builder& PlatformCategory::Builder::wNestedName(const QString& nestedName) { mBlueprint.mNestedName = nestedName; return *this; }

//===============================================================================================================
// Parent
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Parent::Parent() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
QString Parent::platformCategoryName() const { return mPlatformCategoryName; }
QString Parent::platformName() const { return mPlatformName; }
QString Parent::parentPlatformCategoryName() const { return mParentPlatformCategoryName; }
QUuid Parent::playlistId() const { return mPlaylistId; }

//===============================================================================================================
// ParentCategory::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Parent::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Parent::Builder& Parent::Builder::wPlatformCategoryName(const QString& platformCategoryName) { mBlueprint.mPlatformCategoryName = platformCategoryName; return *this; }
Parent::Builder& Parent::Builder::wPlatformName(const QString& platformName) { mBlueprint.mPlatformName = platformName; return *this; }
Parent::Builder& Parent::Builder::wParentPlatformCategoryName(const QString& parentPlatformCategoryName) { mBlueprint.mParentPlatformCategoryName = parentPlatformCategoryName; return *this; }
Parent::Builder& Parent::Builder::wPlaylistId(const QString& rawId) { mBlueprint.mPlaylistId = QUuid(rawId); return *this; }
Parent::Builder& Parent::Builder::wPlaylistId(const QUuid& id) { mBlueprint.mPlaylistId = id; return *this; }

}
