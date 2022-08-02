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
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

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

class Game::Builder : public Fe::Game::Builder<Game::Builder, Game>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wTitle(QString title);
    Builder& wSeries(QString series);
    Builder& wDeveloper(QString developer);
    Builder& wPublisher(QString publisher);
    Builder& wSortTitle(QString sortTitle);
    Builder& wDateAdded(QString rawDateAdded);
    Builder& wDateModified(QString rawDateModified);
    Builder& wBroken(QString rawBroken);
    Builder& wPlayMode(QString playMode);
    Builder& wStatus(QString status);
    Builder& wRegion(QString region);
    Builder& wNotes(QString notes);
    Builder& wSource(QString source);
    Builder& wAppPath(QString appPath);
    Builder& wCommandLine(QString commandLine);
    Builder& wReleaseDate(QString rawReleaseDate);
    Builder& wVersion(QString version);
    Builder& wReleaseType(QString releaseType);
};

class AddApp : public Fe::AddApp
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

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

class AddApp::Builder : public Fe::AddApp::Builder<AddApp::Builder, AddApp>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wAppPath(QString appPath);
    Builder& wCommandLine(QString commandLine);
    Builder& wAutorunBefore(QString rawAutorunBefore);
    Builder& wWaitForExit(QString rawWaitForExit);
};

class CustomField : public Fe::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

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

class CustomField::Builder : public Fe::Item::Builder<CustomField::Builder, CustomField>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wGameId(QString rawGameId);
    Builder& wGameId(QUuid gameId);
    Builder& wName(QString name);
    Builder& wValue(QString value);
};

class PlaylistHeader : public Fe::PlaylistHeader
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

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

class PlaylistHeader::Builder : public Fe::PlaylistHeader::Builder<PlaylistHeader::Builder, PlaylistHeader>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wPlaylistId(QString rawId);
    Builder& wNestedName(QString nestedName);
    Builder& wNotes(QString notes);
};

class PlaylistGame : public Fe::PlaylistGame
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

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

class PlaylistGame::Builder : public Fe::PlaylistGame::Builder<PlaylistGame::Builder, PlaylistGame>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wGameTitle(QString gameTitle);
    Builder& wLBDatabaseId(QString rawLBDatabaseId);
    Builder& wGameFileName(QString gameFileName);
    Builder& wGamePlatform(QString gamePlatform);
    Builder& wManualOrder(QString rawManualOrder);
};

class Platform : public Fe::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
    QString mName;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    Platform();

//-Instance Functions------------------------------------------------------------------------------------------------------
    QString name() const;
};

class Platform::Builder : public Fe::Item::Builder<Platform::Builder, Platform>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(QString name);
};

//class PlatformFolder : public Fe::Item
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

//class PlatformFolderBuilder : public Fe::ItemBuilder<PlatformFolderBuilder, PlatformFolder>
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
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    PlatformCategory();

//-Instance Functions------------------------------------------------------------------------------------------------------
};

class PlatformCategory::Builder : public Fe::Item::Builder<PlatformCategory::Builder, PlatformCategory>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();
};


}
#endif // LAUNCHBOX_ITEMS_H
