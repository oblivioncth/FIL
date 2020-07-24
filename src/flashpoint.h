#ifndef FLASHPOINT_H
#define FLASHPOINT_H

#include <QString>
#include <QDateTime>

namespace FP
{

class FlashpointGame
{
//-Class Enums------------------------------------------------------------------------------------------------------
public:

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mID;
    QString mTitle;
    QString mSeries;
    QString mDeveloper;
    QString mPublisher;
    QDateTime mDateAdded;
    QDateTime mDateModified;
    QString mPlatform;
    bool mBroken;
    QString mPlayMode;
    QString mStatus;
    QString mNotes;
    QString mSource;
    QString mAppPath;
    QString mLaunchCommand;
    QDateTime mReleaseDate;
    QString mVersion;
    QString mOriginalDescription;
    QString mLanguage;
    QString mOrderTitle;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    FlashpointGame(QString id, QString title, QString series, QString developer, QString publisher, QString rawDateAdded,
                   QString rawDateModified, QString platform, QString rawBroken, QString playMode, QString status,
                   QString notes, QString source, QString appPath, QString launchCommand, QString rawReleaseDate, QString version,
                   QString originalDescription, QString language, QString orderTitle);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~FlashpointGame();

//-Class Functions---------------------------------------------------------------------------------------------
private:
    static QString kosherizeRawDate(QString date);

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QString getID() const;
    QString getTitle() const;
    QString getSeries() const;
    QString getDeveloper() const;
    QString getPublisher() const;
    QDateTime getDateAdded() const;
    QDateTime getDateModified() const;
    QString getPlatform() const;
    bool isBroken() const;
    QString getPlayMode() const;
    QString getStatus() const;
    QString getNotes() const;
    QString getSource() const;
    QString getAppPath() const;
    QString getLaunchCommand() const;
    QDateTime getReleaseDate() const;
    QString getVersion() const;
    QString getOriginalDescription() const;
    QString getLanguage() const;
    QString getOrderTitle() const;
};

class FlashpointAdditonalApp
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mID;
    QString mAppPath;
    bool mAutorunBefore;
    QString mLaunchCommand;
    QString mName;
    bool mWaitExit;
    QString mParentID;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    FlashpointAdditonalApp(QString id, QString appPath, QString rawAutorunBefore, QString launchCommand, QString name, QString rawWaitExit, QString parentID);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~FlashpointAdditonalApp();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getID() const;
    QString getAppPath() const;
    bool isAutorunBefore() const;
    QString getLaunchCommand() const;
    QString getName() const;
    bool isWaitExit() const;
    QString getParentID() const;
};

class FlashpointPlaylistGame
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    int mID;
    QString mPlaylistID;
    int mOrder;
    QString mNotes;
    QString mGameID;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    FlashpointPlaylistGame(int id, QString playlistID, QString rawOrder, QString notes, QString gameID);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~FlashpointPlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    int getID() const;
    QString getPlaylistID() const;
    int getOrder() const;
    QString getNotes() const;
    QString getGameID() const;
};

}

#endif // FLASHPOINT_H
