#ifndef LAUNCHBOX_H
#define LAUNCHBOX_H

#include <QString>
#include <QDateTime>
#include <QSet>
#include "flashpoint.h"
#include "qx.h"

namespace LB
{

//-Class Forward Declarations---------------------------------------------------------------------------------------
class GameBuilder;
class AddAppBuilder;
class PlaylistHeaderBuilder;
class PlaylistGameBuilder;

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Game
{
    friend class GameBuilder;

//-Class Variables--------------------------------------------------------------------------------------------------
private:
    static inline const QString RELEASE_TYPE_GAME = "Game";
    static inline const QString RELEASE_TYPE_ANIM = "Animation";

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
    QString mReleaseType;
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Game(FP::Game flashpointGame, QString fullOFLIbPath);
    Game();

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
    QString getReleaseType() const;
    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;

    void transferOtherFields(QHash<QString, QString>& otherFields);
};

class GameBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    Game mGameBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    GameBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    GameBuilder& wID(QString rawID);
    GameBuilder& wTitle(QString title);
    GameBuilder& wSeries(QString series);
    GameBuilder& wDeveloper(QString developer);
    GameBuilder& wPublisher(QString publisher);
    GameBuilder& wPlatform(QString platform);
    GameBuilder& wSortTitle(QString sortTitle);
    GameBuilder& wDateAdded(QString rawDateAdded);
    GameBuilder& wDateModified(QString rawDateModified);
    GameBuilder& wBroken(QString rawBroken);
    GameBuilder& wPlayMode(QString playMode);
    GameBuilder& wStatus(QString status);
    GameBuilder& wRegion(QString region);
    GameBuilder& wNotes(QString notes);
    GameBuilder& wSource(QString source);
    GameBuilder& wAppPath(QString appPath);
    GameBuilder& wCommandLine(QString commandLine);
    GameBuilder& wReleaseDate(QString rawReleaseDate);
    GameBuilder& wVersion(QString version);
    GameBuilder& wReleaseType(QString releaseType);
    GameBuilder& wOtherField(QPair<QString, QString> otherField);

    Game build();
};

class AddApp
{
    friend class AddAppBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mID;
    QUuid mGameID;
    QString mAppPath;
    QString mCommandLine;
    bool mAutorunBefore;
    QString mName;
    bool mWaitForExit;
    QHash<QString, QString> mOtherFields;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    AddApp(FP::AddApp flashpointAddApp, QString fullOFLIbPath);
    AddApp();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getID() const;
    QUuid getGameID() const;
    QString getAppPath() const;
    QString getCommandLine() const;
    bool isAutorunBefore() const;
    QString getName() const;
    bool isWaitForExit() const;
    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;

    void transferOtherFields(QHash<QString, QString>& otherFields);
};

class AddAppBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    AddApp mAddAppBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    AddAppBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    AddAppBuilder& wID(QString rawID);
    AddAppBuilder& wGameID(QString rawGameID);
    AddAppBuilder& wAppPath(QString appPath);
    AddAppBuilder& wCommandLine(QString commandLine);
    AddAppBuilder& wAutorunBefore(QString rawAutorunBefore);
    AddAppBuilder& wName(QString name);
    AddAppBuilder& wWaitForExit(QString rawWaitForExit);
    AddAppBuilder& wOtherField(QPair<QString, QString> otherField);

    AddApp build();
};

class PlaylistHeader
{
    friend class PlaylistHeaderBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mPlaylistID;
    QString mName;
    QString mNestedName;
    QString mNotes;
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistHeader(FP::Playlist flashpointPlaylist);
    PlaylistHeader();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getPlaylistID() const;
    QString getName() const;
    QString getNestedName() const;
    QString getNotes() const;
    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;

    void transferOtherFields(QHash<QString, QString>& otherFields);
};

class PlaylistHeaderBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    PlaylistHeader mPlaylistHeaderBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder& wPlaylistID(QString rawPlaylistID);
    PlaylistHeaderBuilder& wName(QString name);
    PlaylistHeaderBuilder& wNestedName(QString nestedName);
    PlaylistHeaderBuilder& wNotes(QString notes);
    PlaylistHeaderBuilder& wOtherField(QPair<QString, QString> otherField);

    PlaylistHeader build();
};

class PlaylistGame
{
    friend class PlaylistGameBuilder;

//-Class Structs----------------------------------------------------------------------------------------------------
public:
    struct EntryDetails
    {
        QString title;
        QString fileName;
        QString platform;
    };

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mGameID;
    int mLBDatabaseID;
    QString mGameTitle;
    QString mGameFileName;
    QString mGamePlatform;
    int mManualOrder;
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGame(FP::PlaylistGame flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap);
    PlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getGameID() const;
    int getLBDatabaseID() const;
    QString getGameTitle() const;
    QString getGameFileName() const;
    QString getGamePlatform() const;
    int getManualOrder() const;
    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;

    void transferOtherFields(QHash<QString, QString>& otherFields);
    void setLBDatabaseID(int lbDBID);
};

class PlaylistGameBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    PlaylistGame mPlaylistGameBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder& wGameID(QString rawGameID);
    PlaylistGameBuilder& wLBDatabaseID(QString rawLBDatabaseID);
    PlaylistGameBuilder& wGameTitle(QString gameTitle);
    PlaylistGameBuilder& wGameFileName(QString gameFileName);
    PlaylistGameBuilder& wGamePlatform(QString gamePlatform);
    PlaylistGameBuilder& wManualOrder(QString rawManualOrder);
    PlaylistGameBuilder& wOtherField(QPair<QString, QString> otherField);

    PlaylistGame build();
};


}
#endif // LAUNCHBOX_H
