#include "launchbox.h"
#include "flashpointinstall.h"

namespace LB
{
//===============================================================================================================
// LAUNCHBOX GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxGame::LaunchBoxGame(FP::FlashpointGame flashpointGame, QString fullOFLIbPath)
{
    // Set members
    mID = flashpointGame.getID();
    mTitle = flashpointGame.getTitle();
    mSeries = flashpointGame.getSeries();
    mDeveloper = flashpointGame.getDeveloper();
    mPublisher = flashpointGame.getPublisher();
    mPlatform = flashpointGame.getPlatform();
    mSortTitle = flashpointGame.getOrderTitle();
    mDateAdded = flashpointGame.getDateAdded();
    mBroken = flashpointGame.isBroken();
    mPlayMode = flashpointGame.getPlayMode();
    mStatus = flashpointGame.getStatus();
    mRegion = flashpointGame.getLanguage();
    mNotes = flashpointGame.getNotes();
    mSource = flashpointGame.getSource();
    mAppPath = fullOFLIbPath;
    mCommandLine = FP::FlashpointInstall::OFLIb::parametersFromStandard(flashpointGame.getAppPath(),
                                                                        flashpointGame.getLaunchCommand());
    mReleaseDate = flashpointGame.getReleaseDate();
    mVersion = flashpointGame.getVersion();
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxGame::~LaunchBoxGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString LaunchBoxGame::getID() const { return mID; }
QString LaunchBoxGame::getTitle() const { return mTitle; }
QString LaunchBoxGame::getSeries() const { return mSeries; }
QString LaunchBoxGame::getDeveloper() const { return mDeveloper; }
QString LaunchBoxGame::getPublisher() const { return mPublisher; }
QString LaunchBoxGame::getPlatform() const { return mPlatform; }
QString LaunchBoxGame::getSortTitle() const { return mSortTitle; }
QDateTime LaunchBoxGame::getDateAdded() const { return mDateAdded; }
bool LaunchBoxGame::isBroken() const { return mBroken; }
QString LaunchBoxGame::getPlayMode() const { return mPlayMode; }
QString LaunchBoxGame::getStatus() const { return mStatus; }
QString LaunchBoxGame::getRegion() const { return mRegion; }
QString LaunchBoxGame::getNotes() const { return mNotes; }
QString LaunchBoxGame::getSource() const { return mSource; }
QString LaunchBoxGame::getAppPath() const { return mAppPath; }
QString LaunchBoxGame::getCommandLine() const { return mCommandLine; }
QDateTime LaunchBoxGame::getReleaseDate() const { return mReleaseDate; }
QString LaunchBoxGame::getVersion() const { return mVersion; }

//===============================================================================================================
// LAUNCHBOX ADDITIONAL APP
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxAdditionalApp::LaunchBoxAdditionalApp(FP::FlashpointAdditonalApp flashpointAdditionalApp, QString fullOFLIbPath)
{
    // Set members
    mID = flashpointAdditionalApp.getID();
    mGameID = flashpointAdditionalApp.getParentID();
    mAppPath = fullOFLIbPath;
    mCommandLine = FP::FlashpointInstall::OFLIb::parametersFromStandard(flashpointAdditionalApp.getAppPath(),
                                                                        flashpointAdditionalApp.getLaunchCommand());
    mAutorunBefore = flashpointAdditionalApp.isAutorunBefore();
    mName = flashpointAdditionalApp.getName();
    mWaitForExit = flashpointAdditionalApp.isWaitExit();
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxAdditionalApp::~LaunchBoxAdditionalApp() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString LaunchBoxAdditionalApp::getID() const { return mID; }
QString LaunchBoxAdditionalApp::getGameID() const { return mGameID; }
QString LaunchBoxAdditionalApp::getAppPath() const { return mAppPath; }
QString LaunchBoxAdditionalApp::getCommandLine() const { return mCommandLine; }
bool LaunchBoxAdditionalApp::isAutorunBefore() const { return mAutorunBefore; }
QString LaunchBoxAdditionalApp::getName() const { return mName; }
bool LaunchBoxAdditionalApp::isWaitForExit() const { return mWaitForExit; }

//===============================================================================================================
// LAUNCHBOX PLAYLIST GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistGame::LaunchBoxPlaylistGame(FP::FlashpointPlaylistGame flashpointPlaylistGame, Qx::FreeIndexTracker<int>& inUseDBIDs,
                                             QHash<QString, QPair<QString, QString>>& gameID_title_platformMap)
{
    // Set members
    mGameID = flashpointPlaylistGame.getGameID();
    mGameTitle =  gameID_title_platformMap.value(mGameID).first;
    mGamePlatform = gameID_title_platformMap.value(mGameID).second;
    mManualOrder = flashpointPlaylistGame.getOrder();
    mLBDatabaseID = !inUseDBIDs.isReserved(flashpointPlaylistGame.getID()) ? flashpointPlaylistGame.getID() : inUseDBIDs.reserveFirstFree();
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistGame::~LaunchBoxPlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString LaunchBoxPlaylistGame::getGameID() const { return mGameID; };
int LaunchBoxPlaylistGame::getLBDatabaseID() const { return mLBDatabaseID; }
QString LaunchBoxPlaylistGame::getGameTitle() const { return mGameTitle; }
QString LaunchBoxPlaylistGame::getGamePlatform() const { return mGamePlatform; }
int LaunchBoxPlaylistGame::getManualOrder() const { return mManualOrder; }

//===============================================================================================================
// LAUNCHBOX PLAYLIST HEADER
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistHeader::LaunchBoxPlaylistHeader(QString playlistID, QString name, QString nestedName, QString notes)
{
    // Set members
    mPlaylistID = playlistID;
    mName = name;
    mNestedName = nestedName;
    mNotes = notes;
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistHeader::~LaunchBoxPlaylistHeader() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString LaunchBoxPlaylistHeader::getPlaylistID() const { return mPlaylistID; }
QString LaunchBoxPlaylistHeader::getName() const { return mName; }
QString LaunchBoxPlaylistHeader::getNestedName() const { return mNestedName; }
QString LaunchBoxPlaylistHeader::getNotes() const { return mNotes; }

}
