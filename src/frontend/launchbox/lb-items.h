#ifndef LAUNCHBOX_ITEMS_H
#define LAUNCHBOX_ITEMS_H

// Qt Includes
#include <QString>
#include <QDateTime>
#include <QSet>

// libfp Includes
#include <fp/flashpoint/fp-items.h>

// Project Includes
#include "../fe-items.h"

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
    Game(const Fp::Game& flashpointGame, QString fullCLIFpPath);
    Game();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString title() const;
    QString series() const;
    QString developer() const;
    QString publisher() const;
    QString sortTitle() const;
    QDateTime dateAdded() const;
    QDateTime dateModified() const;
    bool isBroken() const;
    QString playMode() const;
    QString status() const;
    QString region() const;
    QString notes() const;
    QString source() const;
    QString appPath() const;
    QString commandLine() const;
    QDateTime releaseDate() const;
    QString version() const;
    QString releaseType() const;
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
    AddApp(const Fp::AddApp& flashpointAddApp, QString fullCLIFpPath);
    AddApp();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString appPath() const;
    QString commandLine() const;
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
    QUuid gameId() const;
    QString name() const;
    QString value() const;
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
    PlaylistHeader(const Fp::Playlist& flashpointPlaylist);
    PlaylistHeader();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid playlistId() const;
    QString nestedName() const;
    QString notes() const;
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
    PlaylistGame(const Fp::PlaylistGame& flashpointPlaylistGame, const QHash<QUuid, EntryDetails>& playlistGameDetailsMap);
    PlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString gameTitle() const;
    int lbDatabaseId() const;
    QString gameFileName() const;
    QString gamePlatform() const;
    int manualOrder() const;

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
    QString name() const;
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
//    QString mediaType();
//    QString folderPath();
//    QString platform();
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
