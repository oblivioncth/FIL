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

//-Namespace Global Structs-----------------------------------------------------------------------------------------
struct OtherField
{
    QString name;
    QString value;

    friend inline bool operator== (const OtherField& lhs, const OtherField& rhs) noexcept;
    friend inline uint qHash(const OtherField& key, uint seed) noexcept;
};

class Game
{
    friend class GameBuilder;

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
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Game(FP::Game flashpointGame, QString fullOFLIbPath);
    Game();

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~Game();

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
    GameBuilder& wOtherField(OtherField otherField);

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

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~AddApp();

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
    AddAppBuilder& wOtherField(OtherField otherField);

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
    PlaylistHeader(QString rawPlaylistID, QString name, QString nestedName, QString notes, QSet<OtherField> otherFields);

    PlaylistHeader();

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~PlaylistHeader();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getPlaylistID() const;
    QString getName() const;
    QString getNestedName() const;
    QString getNotes() const;
    QSet<OtherField> getOtherFields() const;
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
    PlaylistHeaderBuilder& wOtherField(OtherField otherField);

    PlaylistHeader build();
};

class PlaylistGame
{
    friend class PlaylistGameBuilder;

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
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGame(FP::PlaylistGame flashpointPlaylistGame, Qx::FreeIndexTracker<int>& inUseDBIDs,
                          QHash<QUuid, EntryDetails>& playlistGameDetailsMap);
    PlaylistGame();

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~PlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getGameID() const;
    int getLBDatabaseID() const;
    QString getGameTitle() const;
    QString getGamePlatform() const;
    int getManualOrder() const;
    QSet<OtherField> getOtherFields() const;
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
    PlaylistGameBuilder& wGamePlatform(QString gamePlatform);
    PlaylistGameBuilder& wManualOrder(QString rawManualOrder);
    PlaylistGameBuilder& wOtherField(OtherField otherField);

    PlaylistGame build();
};


}
#endif // LAUNCHBOX_H
