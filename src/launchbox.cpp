#include "launchbox.h"
#include "flashpointinstall.h"

namespace LB
{
//===============================================================================================================
// LAUNCHBOX GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxGame::LaunchBoxGame(QString rawID, QString title, QString series, QString developer, QString publisher, QString platform, QString sortTitle, QString rawDateAdded,
              QString rawDateModified, QString rawBroken, QString playMode, QString status, QString region, QString notes, QString source, QString appPath,
              QString commandLine, QString rawReleaseDate, QString version, QSet<OtherField> otherFields)
{
    // Set members that can be directly copied
    mTitle = title;
    mSeries = series;
    mDeveloper = developer;
    mPublisher = publisher;
    mPlatform = platform;
    mSortTitle = sortTitle;
    mPlayMode = playMode;
    mStatus = status;
    mRegion = region;
    mNotes = notes;
    mSource = source;
    mAppPath = appPath;
    mCommandLine = commandLine;
    mVersion = version;
    mOtherFields = otherFields;

    // Set other members
    mID = QUuid(rawID);
    mDateAdded = QDateTime::fromString(rawDateAdded, Qt::ISODateWithMs);
    mDateModified = QDateTime::fromString(rawDateModified, Qt::ISODateWithMs);
    mBroken = rawBroken.toInt() != 0;
    mReleaseDate = QDateTime::fromString(rawReleaseDate, Qt::ISODateWithMs);
}

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
    mDateModified = flashpointGame.getDateModified();
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
QUuid LaunchBoxGame::getID() const { return mID; }
QString LaunchBoxGame::getTitle() const { return mTitle; }
QString LaunchBoxGame::getSeries() const { return mSeries; }
QString LaunchBoxGame::getDeveloper() const { return mDeveloper; }
QString LaunchBoxGame::getPublisher() const { return mPublisher; }
QString LaunchBoxGame::getPlatform() const { return mPlatform; }
QString LaunchBoxGame::getSortTitle() const { return mSortTitle; }
QDateTime LaunchBoxGame::getDateAdded() const { return mDateAdded; }
QDateTime LaunchBoxGame::getDateModified() const { return mDateModified; }
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
LaunchBoxAdditionalApp::LaunchBoxAdditionalApp(QString rawID, QString rawGameID, QString appPath, QString commandLine, QString rawAutorunBefore, QString name, QString rawWaitForExit, QSet<OtherField> otherFields)
{
    // Set members that can be directly coppied
    mAppPath = appPath;
    mCommandLine = commandLine;
    mName = name;
    mOtherFields = otherFields;

    // Set other members
    mID = QUuid(rawID);
    mGameID = QUuid(rawGameID);
    mAutorunBefore = rawAutorunBefore != 0;
    mWaitForExit = rawWaitForExit != 0;
}

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
QUuid LaunchBoxAdditionalApp::getID() const { return mID; }
QUuid LaunchBoxAdditionalApp::getGameID() const { return mGameID; }
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
LaunchBoxPlaylistGame::LaunchBoxPlaylistGame(QString rawGameID, QString rawLBDatabaseID, QString gameTitle, QString gamePlatform, QString rawManualOrder, QSet<OtherField> otherFields)
{
    // Set members that can be directly copied
    mGameTitle = gameTitle;
    mGamePlatform = gamePlatform;
    mOtherFields = otherFields;

    // Set other members
    mGameID = QUuid(rawGameID);
    bool validInt = false;
    mLBDatabaseID = rawLBDatabaseID.toInt(&validInt);
    if(!validInt)
        mLBDatabaseID = -1;
    mManualOrder = rawManualOrder.toInt(&validInt);
    if(!validInt)
        mManualOrder = -1;
}

LaunchBoxPlaylistGame::LaunchBoxPlaylistGame(FP::FlashpointPlaylistGame flashpointPlaylistGame, Qx::FreeIndexTracker<int>& inUseDBIDs,
                                             QHash<QUuid, EntryDetails>& playlistGameDetailsMap)
{
    // Set members
    mGameID = flashpointPlaylistGame.getGameID();
    mGameTitle =  playlistGameDetailsMap.value(mGameID).title;
    mGamePlatform = playlistGameDetailsMap.value(mGameID).platform;
    mManualOrder = flashpointPlaylistGame.getOrder();
    mLBDatabaseID = !inUseDBIDs.isReserved(flashpointPlaylistGame.getID()) ? flashpointPlaylistGame.getID() : inUseDBIDs.reserveFirstFree();
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistGame::~LaunchBoxPlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid LaunchBoxPlaylistGame::getGameID() const { return mGameID; };
int LaunchBoxPlaylistGame::getLBDatabaseID() const { return mLBDatabaseID; }
QString LaunchBoxPlaylistGame::getGameTitle() const { return mGameTitle; }
QString LaunchBoxPlaylistGame::getGamePlatform() const { return mGamePlatform; }
int LaunchBoxPlaylistGame::getManualOrder() const { return mManualOrder; }

//===============================================================================================================
// LAUNCHBOX PLAYLIST HEADER
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistHeader::LaunchBoxPlaylistHeader(QString rawPlaylistID, QString name, QString nestedName, QString notes, QSet<OtherField> otherFields)
{
    // Set members
    mPlaylistID = rawPlaylistID;
    mName = name;
    mNestedName = nestedName;
    mNotes = notes;
    mOtherFields = otherFields;
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxPlaylistHeader::~LaunchBoxPlaylistHeader() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid LaunchBoxPlaylistHeader::getPlaylistID() const { return mPlaylistID; }
QString LaunchBoxPlaylistHeader::getName() const { return mName; }
QString LaunchBoxPlaylistHeader::getNestedName() const { return mNestedName; }
QString LaunchBoxPlaylistHeader::getNotes() const { return mNotes; }

}
