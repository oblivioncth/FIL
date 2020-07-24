#ifndef LAUNCHBOX_H
#define LAUNCHBOX_H

#include <QString>
#include <QDateTime>
#include <QSet>
#include "flashpoint.h"
#include "qx.h"

namespace LB
{

class LaunchBoxGame
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mID;
    QString mTitle;
    QString mSeries;
    QString mDeveloper;
    QString mPublisher;
    QString mPlatform;
    QString mSortTitle;
    QDateTime mDateAdded;
    bool mBroken;
    QString mPlayMode;
    QString mStatus;
    QString mRegion;
    QString mNotes;
    QString mSource;
    QString mAppPath;
    QString mCommandLine;
    QDateTime mReleaseDate;
    QString mVersion;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxGame(FP::FlashpointGame flashpointGame, QString fullOFLIbPath);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~LaunchBoxGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getID() const;
    QString getTitle() const;
    QString getSeries() const;
    QString getDeveloper() const;
    QString getPublisher() const;
    QString getPlatform() const;
    QString getSortTitle() const;
    QDateTime getDateAdded() const;
    bool isBroken() const;
    QString getPlayMode() const;
    QString getStatus() const;
    QString getRegion() const;
    QString getNotes() const;
    QString getSource() const;
    QString getAppPath() const;
    QString getCommandLine() const;
    QDateTime getReleaseDate() const;
    QString getVersion() const;
};

class LaunchBoxAdditionalApp
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mID;
    QString mGameID;
    QString mAppPath;
    QString mCommandLine;
    bool mAutorunBefore;
    QString mName;
    bool mWaitForExit;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxAdditionalApp(FP::FlashpointAdditonalApp flashpointAdditionalApp, QString fullOFLIbPath);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~LaunchBoxAdditionalApp();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getID() const;
    QString getGameID() const;
    QString getAppPath() const;
    QString getCommandLine() const;
    bool isAutorunBefore() const;
    QString getName() const;
    bool isWaitForExit() const;
};

class LaunchBoxPlaylistGame
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mGameID;
    int mLBDatabaseID;
    QString mGameTitle;
    QString mGamePlatform;
    int mManualOrder;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxPlaylistGame(FP::FlashpointPlaylistGame flashpointPlaylistGame, Qx::FreeIndexTracker<int>& inUseDBIDs,
                          QHash<QString, QPair<QString, QString>>& gameID_title_platformMap);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~LaunchBoxPlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getGameID() const;
    int getLBDatabaseID() const;
    QString getGameTitle() const;
    QString getGamePlatform() const;
    int getManualOrder() const;
};

class LaunchBoxPlaylistHeader
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mPlaylistID;
    QString mName;
    QString mNestedName;
    QString mNotes;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxPlaylistHeader(QString playlistID, QString name, QString nestedName, QString notes);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~LaunchBoxPlaylistHeader();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getPlaylistID() const;
    QString getName() const;
    QString getNestedName() const;
    QString getNotes() const;
};
}
#endif // LAUNCHBOX_H
