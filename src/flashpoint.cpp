#include "flashpoint.h"
#include "qx.h"

namespace FP
{
//===============================================================================================================
// FLASHPOINT GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
FlashpointGame::FlashpointGame(QString rawID, QString title, QString series, QString developer, QString publisher, QString rawDateAdded,
                               QString rawDateModified, QString platform, QString rawBroken, QString playMode, QString status,
                               QString notes, QString source, QString appPath, QString launchCommand, QString rawReleaseDate, QString version,
                               QString originalDescription, QString language, QString orderTitle)
{
    // Set members that can be directly copied
    mTitle = title;
    mSeries = series;
    mDeveloper = developer;
    mPublisher = publisher;
    mPlatform = platform;
    mPlayMode = playMode;
    mStatus = status;
    mNotes = notes;
    mSource = source;
    mAppPath = appPath;
    mLaunchCommand = launchCommand;
    mVersion = version;
    mOriginalDescription = originalDescription;
    mLanguage = language;
    mOrderTitle = orderTitle;
    mBroken = rawBroken.toInt() != 0;

    // Set other members
    mID = QUuid(rawID);
    mDateAdded = QDateTime::fromString(rawDateAdded, Qt::ISODateWithMs);
    mDateModified = QDateTime::fromString(rawDateModified, Qt::ISODateWithMs);
    mReleaseDate = QDateTime::fromString(kosherizeRawDate(rawReleaseDate), Qt::ISODate);
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
FlashpointGame::~FlashpointGame() {}

//-Class Functions---------------------------------------------------------------------------------------------
//Private:
QString FlashpointGame::kosherizeRawDate(QString date)
{
    static const QString DEFAULT_MONTH = "-01";
    static const QString DEFAULT_DAY = "-01";

    if(Qx::String::isOnlyNumbers(date) && date.length() == 4) // Year only
        return date + DEFAULT_MONTH + DEFAULT_DAY;
    else if(Qx::String::isOnlyNumbers(date.left(4)) &&
            Qx::String::isOnlyNumbers(date.mid(5,2)) &&
            date.at(4) == '-' && date.length() == 7) // Year and month only
        return  date + DEFAULT_DAY;
    else if(Qx::String::isOnlyNumbers(date.left(4)) &&
            Qx::String::isOnlyNumbers(date.mid(5,2)) &&
            Qx::String::isOnlyNumbers(date.mid(8,2)) &&
            date.at(4) == '-' && date.at(7) == '-' && date.length() == 10) // Year month and day
        return  date;
    else
        return QString(); // Invalid date provided
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid FlashpointGame::getID() const { return mID; }
QString FlashpointGame::getTitle() const { return mTitle; }
QString FlashpointGame::getSeries() const { return mSeries; }
QString FlashpointGame::getDeveloper() const { return mDeveloper; }
QString FlashpointGame::getPublisher() const { return mPublisher; }
QDateTime FlashpointGame::getDateAdded() const { return mDateAdded; }
QDateTime FlashpointGame::getDateModified() const { return mDateModified; }
QString FlashpointGame::getPlatform() const { return mPlatform; }
QString FlashpointGame::getPlayMode() const { return mPlayMode; }
bool FlashpointGame::isBroken() const { return mBroken; }
QString FlashpointGame::getStatus() const { return mStatus; }
QString FlashpointGame::getNotes() const{ return mNotes; }
QString FlashpointGame::getSource() const { return mSource; }
QString FlashpointGame::getAppPath() const { return mAppPath; }
QString FlashpointGame::getLaunchCommand() const { return mLaunchCommand; }
QDateTime FlashpointGame::getReleaseDate() const { return mReleaseDate; }
QString FlashpointGame::getVersion() const { return mVersion; }
QString FlashpointGame::getOriginalDescription() const { return mOriginalDescription; }
QString FlashpointGame::getLanguage() const { return mLanguage; }
QString FlashpointGame::getOrderTitle() const { return mOrderTitle; }

//===============================================================================================================
// FLASHPOINT ADDITIONAL APP
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
FlashpointAdditonalApp::FlashpointAdditonalApp(QString rawID, QString appPath, QString rawAutorunBefore, QString launchCommand,
                                               QString name, QString rawWaitExit, QString rawParentID)
{
    // Set members that can be directly copied
    mAppPath = appPath;
    mLaunchCommand = launchCommand;
    mName = name;


    // Set other members
    mID = QUuid(rawID);
    mAutorunBefore = rawAutorunBefore.toInt() != 0;
    mWaitExit = rawWaitExit.toInt() != 0;
    mParentID = QUuid(rawParentID);
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
FlashpointAdditonalApp::~FlashpointAdditonalApp() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid FlashpointAdditonalApp::getID() const { return mID; }
QString FlashpointAdditonalApp::getAppPath() const { return mAppPath; }
bool FlashpointAdditonalApp::isAutorunBefore() const { return  mAutorunBefore; }
QString FlashpointAdditonalApp::getLaunchCommand() const { return mLaunchCommand; }
QString FlashpointAdditonalApp::getName() const { return mName; }
bool FlashpointAdditonalApp::isWaitExit() const { return mWaitExit; }
QUuid FlashpointAdditonalApp::getParentID() const { return mParentID; }

//===============================================================================================================
// FLASHPOINT PLAYLIST GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
FlashpointPlaylistGame::FlashpointPlaylistGame(int id, QString playlistID, QString rawOrder, QString notes, QString rawGameID)
{
    // Set members that can be directly copied
    mID = id;
    mPlaylistID = playlistID;
    mNotes = notes;

    // Set other members
    bool validInt = false;
    mOrder = rawOrder.toInt(&validInt);
    if(!validInt)
        mOrder = -1;
    mGameID = QUuid(rawGameID);
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
FlashpointPlaylistGame::~FlashpointPlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:

int FlashpointPlaylistGame::getID() const { return mID; }
QString FlashpointPlaylistGame::getPlaylistID() const { return mPlaylistID; }
int FlashpointPlaylistGame::getOrder() const { return mOrder; }
QString FlashpointPlaylistGame::getNotes() const { return mNotes; }
QUuid FlashpointPlaylistGame::getGameID() const { return mGameID; }
}
