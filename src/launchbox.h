#ifndef LAUNCHBOX_H
#define LAUNCHBOX_H

#include <QString>
#include <QDateTime>
#include <QSet>
#include "flashpoint.h"
#include "qx.h"

namespace LB
{

//-Namespace Global Structs-----------------------------------------------------------------------------------------
struct OtherField
{
    QString name;
    QString value;
};

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
    QSet<OtherField> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxGame(QString rawID, QString title, QString series, QString developer, QString publisher, QString platform, QString sortTitle, QString rawDateAdded,
                  QString rawDateModified, QString rawBroken, QString playMode, QString status, QString region, QString notes, QString source, QString appPath,
                  QString commandLine, QString rawReleaseDate, QString version, QSet<OtherField> otherFields);

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
    QSet<OtherField> getOtherFields() const;
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
    QSet<OtherField> mOtherFields;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    LaunchBoxAdditionalApp(QString rawID, QString rawGameID, QString appPath, QString commandLine, QString rawAutorunBefore, QString name, QString rawWaitForExit, QSet<OtherField> otherFields);

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
    QSet<OtherField> getOtherFields() const;
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
    QSet<OtherField> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxPlaylistGame(QString rawGameID, QString rawLBDatabaseID, QString gameTitle, QString gamePlatform, QString rawManualOrder, QSet<OtherField> otherFields);

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
    QSet<OtherField> getOtherFields() const;
};

class LaunchBoxPlaylistHeader
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mPlaylistID;
    QString mName;
    QString mNestedName;
    QString mNotes;
    QSet<OtherField> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:    
    LaunchBoxPlaylistHeader(QString rawPlaylistID, QString name, QString nestedName, QString notes, QSet<OtherField> otherFields);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~LaunchBoxPlaylistHeader();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getPlaylistID() const;
    QString getName() const;
    QString getNestedName() const;
    QString getNotes() const;
    QSet<OtherField> getOtherFields() const;
};
}
#endif // LAUNCHBOX_H
