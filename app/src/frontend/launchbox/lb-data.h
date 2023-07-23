#ifndef LAUNCHBOX_XML_H
#define LAUNCHBOX_XML_H

#pragma warning( disable : 4250 )

// Qt Includes
#include <QString>
#include <QFile>
#include <memory>
#include <QXmlStreamReader>

// Qx Includes
#include <qx/core/qx-freeindextracker.h>

// Project Includes
#include "lb-items.h"
#include "../fe-data.h"

// Reminder for virtual inheritance constructor mechanics if needed,
// since some classes here use multiple virtual inheritance:
// https://stackoverflow.com/questions/70746451/

namespace Lb
{

class DocKey
{
    friend class Install;
private:
    DocKey() {};
    DocKey(const DocKey&) = default;
};

class XmlDocReader : public virtual Fe::DataDoc::Reader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QFile mXmlFile;
    QXmlStreamReader mStreamReader;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    XmlDocReader(Fe::DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    virtual Fe::DocHandlingError readTargetDoc() = 0;

protected:
    Fe::DocHandlingError streamStatus() const;

public:
    Fe::DocHandlingError readInto() override;
};

class XmlDocWriter : public virtual Fe::DataDoc::Writer
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QFile mXmlFile;
    QXmlStreamWriter mStreamWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    XmlDocWriter(Fe::DataDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    virtual bool writeSourceDoc() = 0;
    void writeCleanTextElement(const QString& qualifiedName, const QString& text);
    void writeOtherFields(const QHash<QString, QString>& otherFields);
    Fe::DocHandlingError streamStatus() const;

public:
    Fe::DocHandlingError writeOutOf() override;
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

class PlatformDoc::Reader : public Fe::BasicPlatformDoc::Reader, public XmlDocReader
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

class PlatformDoc::Writer : public Fe::BasicPlatformDoc::Writer, public XmlDocWriter
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

class PlaylistDoc::Reader : public Fe::BasicPlaylistDoc::Reader, public XmlDocReader
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

class PlaylistDoc::Writer : public Fe::BasicPlaylistDoc::Writer, XmlDocWriter
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

class PlatformsConfigDoc::Reader : public XmlDocReader
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

class PlatformsConfigDoc::Writer : public XmlDocWriter
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

}
#endif // LAUNCHBOX_XML_H
