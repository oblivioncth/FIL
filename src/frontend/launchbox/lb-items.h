#ifndef LAUNCHBOX_ITEMS_H
#define LAUNCHBOX_ITEMS_H

#include <QString>
#include <QDateTime>
#include <QSet>
#include "../fe-items.h"
#include "../../flashpoint/fp-items.h"
#include "qx.h"

namespace Lb
{

class Game : public Fe::Game
{
    friend class GameBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mSeries;
    QString mDeveloper;
    QString mPublisher;
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
    Game(Fp::Game flashpointGame, QString fullCLIFpPath);
    Game();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getTitle() const;
    QString getSeries() const;
    QString getDeveloper() const;
    QString getPublisher() const;
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

class GameBuilder : public Fe::GameBuilder<GameBuilder, Game>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    GameBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    GameBuilder& wTitle(QString title);
    GameBuilder& wSeries(QString series);
    GameBuilder& wDeveloper(QString developer);
    GameBuilder& wPublisher(QString publisher);
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

class AddApp : public Fe::AddApp
{
    friend class AddAppBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mAppPath;
    QString mCommandLine;
    bool mAutorunBefore;
    bool mWaitForExit;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    AddApp(Fp::AddApp flashpointAddApp, QString fullCLIFpPath);
    AddApp();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getAppPath() const;
    QString getCommandLine() const;
    bool isAutorunBefore() const;
    bool isWaitForExit() const;
};

class AddAppBuilder : public Fe::AddAppBuilder<AddAppBuilder, AddApp>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    AddAppBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    AddAppBuilder& wAppPath(QString appPath);
    AddAppBuilder& wCommandLine(QString commandLine);
    AddAppBuilder& wAutorunBefore(QString rawAutorunBefore);
    AddAppBuilder& wWaitForExit(QString rawWaitForExit);
};

class CustomField : public Fe::Item
{
    friend class CustomFieldBuilder;
//-Class Variables--------------------------------------------------------------------------------------------------
public:
    static inline const QString LANGUAGE = "Language";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mGameId;
    QString mName;
    QString mValue;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    CustomField();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getGameId() const;
    QString getName() const;
    QString getValue() const;
};

class CustomFieldBuilder : public Fe::ItemBuilder<CustomFieldBuilder, CustomField>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    CustomFieldBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    CustomFieldBuilder& wGameId(QString rawGameId);
    CustomFieldBuilder& wGameId(QUuid gameId);
    CustomFieldBuilder& wName(QString name);
    CustomFieldBuilder& wValue(QString value);
};

class PlaylistHeader : public Fe::PlaylistHeader
{
    friend class PlaylistHeaderBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mNestedName;
    QString mNotes;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistHeader(Fp::Playlist flashpointPlaylist);
    PlaylistHeader();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getPlaylistId() const;
    QString getNestedName() const;
    QString getNotes() const;
};

class PlaylistHeaderBuilder : public Fe::PlaylistHeaderBuilder<PlaylistHeaderBuilder, PlaylistHeader>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder& wPlaylistId(QString rawId);
    PlaylistHeaderBuilder& wNestedName(QString nestedName);
    PlaylistHeaderBuilder& wNotes(QString notes);
};

class PlaylistGame : public Fe::PlaylistGame
{
    friend class PlaylistGameBuilder;

//-Class Structs----------------------------------------------------------------------------------------------------
public:
    class EntryDetails
    {
    private:
        QString mTitle;
        QString mFilename;
        QString mPlatform;

    public:
        EntryDetails();
        EntryDetails(const Game& refGame);

        QString title() const;
        QString filename() const;
        QString platform() const;
    };

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    int mLBDatabaseId;
    QString mGameFilename;
    QString mGamePlatform;
    int mManualOrder;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGame(Fp::PlaylistGame flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap);
    PlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString getGameTitle() const;
    int getLBDatabaseId() const;
    QString getGameFileName() const;
    QString getGamePlatform() const;
    int getManualOrder() const;

    void setLBDatabaseId(int lbDbId);
};

class PlaylistGameBuilder : public Fe::PlaylistGameBuilder<PlaylistGameBuilder, PlaylistGame>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder& wGameTitle(QString gameTitle);
    PlaylistGameBuilder& wLBDatabaseId(QString rawLBDatabaseId);
    PlaylistGameBuilder& wGameFileName(QString gameFileName);
    PlaylistGameBuilder& wGamePlatform(QString gamePlatform);
    PlaylistGameBuilder& wManualOrder(QString rawManualOrder);
};

class Platform : public Fe::Item
{
    friend class PlatformBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
    QString mName;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    Platform();

//-Instance Functions------------------------------------------------------------------------------------------------------
    QString getName() const;
};

class PlatformBuilder : public Fe::ItemBuilder<PlatformBuilder, Platform>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlatformBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlatformBuilder& wName(QString name);
};

//class PlatformFolder  : public Item
//{
//    friend class PlatformFolderBuilder;

////-Instance Variables-----------------------------------------------------------------------------------------------
//private:
//    QString mMediaType;
//    QString mFolderPath;
//    QString mPlatform;

////-Constructor------------------------------------------------------------------------------------------------------
//public:
//    PlatformFolder();

////-Instance Functions------------------------------------------------------------------------------------------------------
//    QString getMediaType();
//    QString getFolderPath();
//    QString getPlatform();
//};

//class PlatformFolderBuilder : public ItemBuilder<PlatformFolderBuilder, PlatformFolder>
//{
////-Constructor-------------------------------------------------------------------------------------------------
//public:
//    PlatformFolderBuilder();

////-Instance Functions------------------------------------------------------------------------------------------
//public:
//    PlatformFolderBuilder& wMediaType(QString mediaType);
//    PlatformFolderBuilder& wFolderPath(QString folderPath);
//    PlatformFolderBuilder& wPlatform(QString platform);
//};

class PlatformCategory  : public Fe::Item
{
    friend class PlatformCategoryBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------

//-Constructor------------------------------------------------------------------------------------------------------
public:
    PlatformCategory();

//-Instance Functions------------------------------------------------------------------------------------------------------
};

class PlatformCategoryBuilder : public Fe::ItemBuilder<PlatformCategoryBuilder, PlatformCategory>
{
//-Instance Variables-----------------------------------------------------------------------------------------------


//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlatformCategoryBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
};


}
#endif // LAUNCHBOX_ITEMS_H
