#ifndef LAUNCHBOX_DATA_H
#define LAUNCHBOX_DATA_H

#pragma warning( disable : 4250 )

// Qt Includes
#include <QString>
#include <QFile>
#include <memory>

// Qx Includes
#include <qx/core/qx-freeindextracker.h>

// Project Includes
#include "launcher/abstract/lr-data.h"
#include "launcher/implementation/launchbox/lb-registration.h"
#include "launcher/implementation/launchbox/lb-items.h"

namespace Lb
{

class PlatformDoc : public Lr::BasicPlatformDoc<LauncherId>
{
    friend PlatformDocReader;
    friend PlatformDocWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, CustomField> mCustomFieldsFinal;
    QHash<QString, CustomField> mCustomFieldsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformDoc(Install* install, const QString& xmlPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Game prepareGame(const Fp::Game& game) override;
    AddApp prepareAddApp(const Fp::AddApp& addApp) override;

    void addCustomField(CustomField&& customField);

public:
    bool isEmpty() const override;

    void finalize() override;
};

class PlatformDocReader : public Lr::XmlDocReader<PlatformDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformDocReader(PlatformDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parseGame();
    void parseAddApp();
    void parseCustomField();
};

class PlatformDocWriter : public Lr::XmlDocWriter<PlatformDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformDocWriter(PlatformDoc* sourceDoc);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeGame(const Game& game);
    bool writeAddApp(const AddApp& addApp);
    bool writeCustomField(const CustomField& customField);
};

class PlaylistDoc : public Lr::BasicPlaylistDoc<LauncherId>
{
    friend class PlaylistDocReader;
    friend class PlaylistDocWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Qx::FreeIndexTracker* mLaunchBoxDatabaseIdTracker;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlaylistDoc(Install* install, const QString& xmlPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    PlaylistHeader preparePlaylistHeader(const Fp::Playlist& playlist) override;
    PlaylistGame preparePlaylistGame(const Fp::PlaylistGame& game) override;
};

class PlaylistDocReader : public Lr::XmlDocReader<PlaylistDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlaylistDocReader(PlaylistDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parsePlaylistHeader();
    void parsePlaylistGame();
};

class PlaylistDocWriter : public Lr::XmlDocWriter<PlaylistDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlaylistDocWriter(PlaylistDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writePlaylistHeader(const PlaylistHeader& playlistHeader);
    bool writePlaylistGame(const PlaylistGame& playlistGame);
};

class PlatformsConfigDoc : public Lr::UpdateableDoc<LauncherId>
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = u"Platforms"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, Platform> mPlatformsFinal;
    QHash<QString, Platform> mPlatformsExisting;
    QMap<QString, PlatformFolder> mPlatformFoldersFinal;
    QMap<QString, PlatformFolder> mPlatformFoldersExisting;
    QMap<QString, PlatformCategory> mPlatformCategoriesFinal;
    QMap<QString, PlatformCategory> mPlatformCategoriesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformsConfigDoc(Install* install, const QString& xmlPath, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    bool isEmpty() const override;

    const QHash<QString, Platform>& finalPlatforms() const;
    const QMap<QString, PlatformFolder>& finalPlatformFolders() const;
    const QMap<QString, PlatformCategory>& finalPlatformCategories() const;

    void addPlatform(const Platform& platform);
    void removePlatform(const QString& platformName);

    void addPlatformFolder(const PlatformFolder& platformFolder);
    void removePlatformFolders(const QString& platformName);

    void addPlatformCategory(const PlatformCategory& platformCategory);
    void removePlatformCategory(const QString& categoryName);

    void finalize() override;
};

class PlatformsConfigDoc::Reader : public Lr::XmlDocReader<PlatformsConfigDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(PlatformsConfigDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parsePlatform();
    Lr::DocHandlingError parsePlatformFolder();
    void parsePlatformCategory();
};

class PlatformsConfigDoc::Writer : public Lr::XmlDocWriter<PlatformsConfigDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(PlatformsConfigDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writePlatform(const Platform& platform);
    bool writePlatformFolder(const PlatformFolder& platformFoler);
    bool writePlatformCategory(const PlatformCategory& platformCategory);
};

class ParentsDoc : public Lr::DataDoc<LauncherId>
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = u"Parents"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QList<Parent> mParents;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit ParentsDoc(Install* install, const QString& xmlPath);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;
    bool removeIfPresent(qsizetype idx);

    qsizetype findPlatformCategory(QStringView platformCategory, QStringView parentCategory) const;
    qsizetype findPlatform(QStringView platform, QStringView parentCategory) const;
    qsizetype findPlaylist(const QUuid& playlistId, QStringView parentCategory) const;

public:
    bool isEmpty() const override;

    /* NOTE: The methods here than take an optional parent category won't look for any of their type
     * if no parent is specified, but rather one that explicitly has no parent. If these is needed for
     * some reason, consider adding "any" variants of the functions (i.e. containsAnyPlatformCategory(), etc.)
     */
    bool containsPlatformCategory(QStringView platformCategory, QStringView parentCategory = {}) const;
    bool containsPlatform(QStringView platform, QStringView parentCategory = {}) const;
    bool containsPlaylist(const QUuid& playlistId, QStringView parentCategory = {}) const;

    bool removePlatformCategory(QStringView platformCategory, QStringView parentCategory = {});
    bool removePlatform(QStringView platform, QStringView parentCategory = {});
    bool removePlaylist(const QUuid& playlistId, QStringView parentCategory = {});


    const QList<Parent>& parents() const;

    void addParent(const Parent& parent);
};

class ParentsDoc::Reader : public Lr::XmlDocReader<ParentsDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(ParentsDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parseParent();
};

class ParentsDoc::Writer : public Lr::XmlDocWriter<ParentsDoc>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(ParentsDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeParent(const Parent& parent);
};

}
#endif // LAUNCHBOX_DATA_H
