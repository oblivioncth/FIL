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
class Item;
template <typename B, typename T, ENABLE_IF(std::is_base_of<Item, T>)> class ItemBuilder;
class ThingBuilder;
class GameBuilder;
class AddAppBuilder;
class PlaylistHeaderBuilder;
class PlaylistGameBuilder;

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Item
{
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<Item, T>)>
    friend class ItemBuilder;
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Item();

    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;

//-Instance Functions------------------------------------------------------------------------------------------
public:
    void transferOtherFields(QHash<QString, QString>& otherFields);
};

template <typename B, typename T, ENABLE_IF2(std::is_base_of<Item, T>)>
class ItemBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
protected:
    T mItemBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    ItemBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wOtherField(QPair<QString, QString> otherField)
    {
        mItemBlueprint.mOtherFields[otherField.first] = otherField.second;
        return static_cast<B&>(*this);
    }
    T build() { return mItemBlueprint; }
};

class Game : public Item
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
};

class GameBuilder : public ItemBuilder<GameBuilder, Game>
{
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
};

class AddApp : public Item
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
};

class AddAppBuilder : public ItemBuilder<AddAppBuilder, AddApp>
{
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
};

class PlaylistHeader : public Item
{
    friend class PlaylistHeaderBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mPlaylistID;
    QString mName;
    QString mNestedName;
    QString mNotes;

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
};

class PlaylistHeaderBuilder : public ItemBuilder<PlaylistHeaderBuilder, PlaylistHeader>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder& wPlaylistID(QString rawPlaylistID);
    PlaylistHeaderBuilder& wName(QString name);
    PlaylistHeaderBuilder& wNestedName(QString nestedName);
    PlaylistHeaderBuilder& wNotes(QString notes);
};

class PlaylistGame : public Item
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

    void setLBDatabaseID(int lbDBID);
};

class PlaylistGameBuilder : public ItemBuilder<PlaylistGameBuilder, PlaylistGame>
{
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
};

class Platform  : public Item
{
    friend class PlatformBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------

//-Constructor------------------------------------------------------------------------------------------------------
public:
    Platform();

//-Instance Functions------------------------------------------------------------------------------------------------------
};

class PlatformBuilder : public ItemBuilder<PlatformBuilder, Platform>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlatformBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
};


class PlatformCategory  : public Item
{
    friend class PlatformCategoryBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------

//-Constructor------------------------------------------------------------------------------------------------------
public:
    PlatformCategory();

//-Instance Functions------------------------------------------------------------------------------------------------------
};

class PlatformCategoryBuilder
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlatformCategoryBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
};


}
#endif // LAUNCHBOX_H
