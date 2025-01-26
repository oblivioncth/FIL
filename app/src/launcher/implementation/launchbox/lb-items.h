#ifndef LAUNCHBOX_ITEMS_H
#define LAUNCHBOX_ITEMS_H

// Qt Includes
#include <QString>
#include <QDateTime>
#include <QSet>

// libfp Includes
#include <fp/fp-items.h>

// Project Includes
#include "launcher/interface/lr-items-interface.h"

namespace Lb
{

class Game : public Lr::Game
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
    Game(const Fp::Game& flashpointGame, const QString& fullCLIFpPath);
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

class Game::Builder : public Lr::Game::Builder<Game>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wTitle(const QString& title);
    Builder& wSeries(const QString& series);
    Builder& wDeveloper(const QString& developer);
    Builder& wPublisher(const QString& publisher);
    Builder& wSortTitle(const QString& sortTitle);
    Builder& wDateAdded(const QString& rawDateAdded);
    Builder& wDateModified(const QString& rawDateModified);
    Builder& wBroken(const QString& rawBroken);
    Builder& wPlayMode(const QString& playMode);
    Builder& wStatus(const QString& status);
    Builder& wRegion(const QString& region);
    Builder& wNotes(const QString& notes);
    Builder& wSource(const QString& source);
    Builder& wAppPath(const QString& appPath);
    Builder& wCommandLine(const QString& commandLine);
    Builder& wReleaseDate(const QString& rawReleaseDate);
    Builder& wVersion(const QString& version);
    Builder& wReleaseType(const QString& releaseType);
};

class AddApp : public Lr::AddApp
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
    AddApp(const Fp::AddApp& flashpointAddApp, const QString& fullCLIFpPath);
    AddApp();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString appPath() const;
    QString commandLine() const;
    bool isAutorunBefore() const;
    bool isWaitForExit() const;
};

class AddApp::Builder : public Lr::AddApp::Builder<AddApp>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wAppPath(const QString& appPath);
    Builder& wCommandLine(const QString& commandLine);
    Builder& wAutorunBefore(const QString& rawAutorunBefore);
    Builder& wWaitForExit(const QString& rawWaitForExit);
};

class CustomField : public Lr::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    static inline const QString LANGUAGE = u"Language"_s;

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

class CustomField::Builder : public Lr::Item::Builder<CustomField>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wGameId(const QString& rawGameId);
    Builder& wGameId(const QUuid& gameId);
    Builder& wName(const QString& name);
    Builder& wValue(const QString& value);
};

class PlaylistHeader : public Lr::PlaylistHeader
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

class PlaylistHeader::Builder : public Lr::PlaylistHeader::Builder<PlaylistHeader>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wPlaylistId(const QString& rawId);
    Builder& wNestedName(const QString& nestedName);
    Builder& wNotes(const QString& notes);
};

class PlaylistGame : public Lr::PlaylistGame
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

class PlaylistGame::Builder : public Lr::PlaylistGame::Builder<PlaylistGame>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wGameTitle(const QString& gameTitle);
    Builder& wLBDatabaseId(const QString& rawLBDatabaseId);
    Builder& wGameFileName(const QString& gameFileName);
    Builder& wGamePlatform(const QString& gamePlatform);
    Builder& wManualOrder(const QString& rawManualOrder);
};

class Platform : public Lr::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mName;
//    QString mCategory;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    Platform();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
//    QString category() const;
};

class Platform::Builder : public Lr::Item::Builder<Platform>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& name);
//    Builder& wCategory(const QString& category);
};

class PlatformFolder : public Lr::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mMediaType;
    QString mFolderPath;
    QString mPlatform;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    PlatformFolder();

//-Instance Functions------------------------------------------------------------------------------------------------------
    QString mediaType() const;
    QString folderPath() const;
    QString platform() const;

    QString identifier() const;
};

class PlatformFolder::Builder : public Lr::Item::Builder<PlatformFolder>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wMediaType(const QString& mediaType);
    Builder& wFolderPath(const QString& folderPath);
    Builder& wPlatform(const QString& platform);
};

class PlatformCategory : public Lr::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mName;
    QString mNestedName;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    PlatformCategory();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
    QString nestedName() const;
};

class PlatformCategory::Builder : public Lr::Item::Builder<PlatformCategory>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& name);
    Builder& wNestedName(const QString& nestedName);
};

class Parent : public Lr::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mPlatformCategoryName;
    QString mPlatformName;
    QString mParentPlatformCategoryName;
    QUuid mPlaylistId;

//-Constructor------------------------------------------------------------------------------------------------------
public:
    Parent();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString platformCategoryName() const;
    QString platformName() const;
    QString parentPlatformCategoryName() const;
    QUuid playlistId() const;
};

class Parent::Builder : public Lr::Item::Builder<Parent>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wPlatformCategoryName(const QString& platformCategoryName);
    Builder& wPlatformName(const QString& platformName);
    Builder& wParentPlatformCategoryName(const QString& parentPlatformCategoryName);
    Builder& wPlaylistId(const QString& rawId);
    Builder& wPlaylistId(const QUuid& id);
};

}
#endif // LAUNCHBOX_ITEMS_H
