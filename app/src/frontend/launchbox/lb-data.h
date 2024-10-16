#ifndef LAUNCHBOX_XML_H
#define LAUNCHBOX_XML_H

#pragma warning( disable : 4250 )

// Qt Includes
#include <QString>
#include <QFile>
#include <memory>

// Qx Includes
#include <qx/core/qx-freeindextracker.h>

// Project Includes
#include "lb-items.h"
#include "frontend/fe-data.h"

// Reminder for virtual inheritance constructor mechanics if needed,
// since some classes here use multiple virtual inheritance:
// https://stackoverflow.com/questions/70746451/

namespace Lb
{

class Install;

class DocKey
{
    friend class Install;
private:
    DocKey() {}
    DocKey(const DocKey&) = default;
};

class PlatformDoc : public Fe::BasicPlatformDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, std::shared_ptr<CustomField>> mCustomFieldsFinal;
    QHash<QString, std::shared_ptr<CustomField>> mCustomFieldsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformDoc(Install* const parent, const QString& xmlPath, QString docName, const Fe::UpdateOptions& updateOptions,
                         const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:    
    std::shared_ptr<Fe::Game> prepareGame(const Fp::Game& game, const Fe::ImageSources& images) override;
    std::shared_ptr<Fe::AddApp> prepareAddApp(const Fp::AddApp& addApp) override;

    void addCustomField(std::shared_ptr<CustomField> customField);

public:
    bool isEmpty() const override;

    void finalize() override;
};

class PlatformDoc::Reader : public Fe::BasicPlatformDoc::Reader, public Fe::XmlDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(PlatformDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Fe::DocHandlingError readTargetDoc() override;
    void parseGame();
    void parseAddApp();
    void parseCustomField();
};

class PlatformDoc::Writer : public Fe::BasicPlatformDoc::Writer, public Fe::XmlDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(PlatformDoc* sourceDoc);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeGame(const Game& game);
    bool writeAddApp(const AddApp& addApp);
    bool writeCustomField(const CustomField& customField);
};

class PlaylistDoc : public Fe::BasicPlaylistDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Qx::FreeIndexTracker* mLaunchBoxDatabaseIdTracker;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlaylistDoc(Install* const parent, const QString& xmlPath, QString docName, const Fe::UpdateOptions& updateOptions,
                         const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    std::shared_ptr<Fe::PlaylistHeader> preparePlaylistHeader(const Fp::Playlist& playlist) override;
    std::shared_ptr<Fe::PlaylistGame> preparePlaylistGame(const Fp::PlaylistGame& game) override;
};

class PlaylistDoc::Reader : public Fe::BasicPlaylistDoc::Reader, public Fe::XmlDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(PlaylistDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Fe::DocHandlingError readTargetDoc() override;
    void parsePlaylistHeader();
    void parsePlaylistGame();
};

class PlaylistDoc::Writer : public Fe::BasicPlaylistDoc::Writer, Fe::XmlDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(PlaylistDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writePlaylistHeader(const PlaylistHeader& playlistHeader);
    bool writePlaylistGame(const PlaylistGame& playlistGame);
};

class PlatformsConfigDoc : public Fe::UpdateableDoc
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
    explicit PlatformsConfigDoc(Install* const parent, const QString& xmlPath, const Fe::UpdateOptions& updateOptions,
                                const DocKey&);

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

class PlatformsConfigDoc::Reader : public Fe::XmlDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(PlatformsConfigDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Fe::DocHandlingError readTargetDoc() override;
    void parsePlatform();
    Fe::DocHandlingError parsePlatformFolder();
    void parsePlatformCategory();
};

class PlatformsConfigDoc::Writer : public Fe::XmlDocWriter
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

class ParentsDoc : public Fe::DataDoc
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
    explicit ParentsDoc(Install* const parent, const QString& xmlPath, const DocKey&);

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

class ParentsDoc::Reader : public Fe::XmlDocReader
{
    //-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(ParentsDoc* targetDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Fe::DocHandlingError readTargetDoc() override;
    void parseParent();
};

class ParentsDoc::Writer : public Fe::XmlDocWriter
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
#endif // LAUNCHBOX_XML_H
