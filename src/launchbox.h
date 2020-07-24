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
    QUuid mID;
    QString mTitle;
    QString mSeries;
    QString mDeveloper;
    QString mPublisher;
    QString mPlatform;
    QString mSortTitle;
    QDateTime mDateAdded;
    QDateTime mDateModified;
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
    QUuid getID() const;
    QString getTitle() const;
    QString getSeries() const;
    QString getDeveloper() const;
    QString getPublisher() const;
    QString getPlatform() const;
    QString getSortTitle() const;
    QDateTime getDateAdded() const;
    QDateTime getDateModified() const;
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
    QUuid mID;
    QUuid mGameID;
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
    QUuid getID() const;
    QUuid getGameID() const;
    QString getAppPath() const;
    QString getCommandLine() const;
    bool isAutorunBefore() const;
    QString getName() const;
    bool isWaitForExit() const;
};

class LaunchBoxPlaylistGame
{
//-Class Structs----------------------------------------------------------------------------------------------------
    struct EntryDetails
    {
        QString title;
        QString platform;
    };

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mGameID;
    int mLBDatabaseID;
    QString mGameTitle;
    QString mGamePlatform;
    int mManualOrder;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxPlaylistGame(FP::FlashpointPlaylistGame flashpointPlaylistGame, Qx::FreeIndexTracker<int>& inUseDBIDs,
                          QHash<QUuid, EntryDetails>& playlistGameDetailsMap);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~LaunchBoxPlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getGameID() const;
    int getLBDatabaseID() const;
    QString getGameTitle() const;
    QString getGamePlatform() const;
    int getManualOrder() const;
};

class LaunchBoxPlaylistHeader
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mPlaylistID;
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
    QUuid getPlaylistID() const;
    QString getName() const;
    QString getNestedName() const;
    QString getNotes() const;
};
}
#endif // LAUNCHBOX_H
