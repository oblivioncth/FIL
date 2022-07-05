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

namespace Xml
{
//-Classes-------------------------------------------------------------------------------------------------
    class Element_Game
    {
    public:
        static inline const QString NAME = "Game";

        static inline const QString ELEMENT_ID = "ID";
        static inline const QString ELEMENT_TITLE = "Title";
        static inline const QString ELEMENT_SERIES = "Series";
        static inline const QString ELEMENT_DEVELOPER = "Developer";
        static inline const QString ELEMENT_PUBLISHER = "Publisher";
        static inline const QString ELEMENT_PLATFORM = "Platform";
        static inline const QString ELEMENT_SORT_TITLE = "SortTitle";
        static inline const QString ELEMENT_DATE_ADDED = "DateAdded";
        static inline const QString ELEMENT_DATE_MODIFIED = "DateModified";
        static inline const QString ELEMENT_BROKEN = "Broken";
        static inline const QString ELEMENT_PLAYMODE = "PlayMode";
        static inline const QString ELEMENT_STATUS = "Status";
        static inline const QString ELEMENT_REGION = "Region";
        static inline const QString ELEMENT_NOTES = "Notes";
        static inline const QString ELEMENT_SOURCE = "Source";
        static inline const QString ELEMENT_APP_PATH = "ApplicationPath";
        static inline const QString ELEMENT_COMMAND_LINE = "CommandLine";
        static inline const QString ELEMENT_RELEASE_DATE = "ReleaseDate";
        static inline const QString ELEMENT_VERSION = "Version";
        static inline const QString ELEMENT_RELEASE_TYPE = "ReleaseType";
    };

    class Element_AddApp
    {
    public:
        static inline const QString NAME = "AdditionalApplication";

        static inline const QString ELEMENT_ID = "Id";
        static inline const QString ELEMENT_GAME_ID = "GameID";
        static inline const QString ELEMENT_APP_PATH = "ApplicationPath";
        static inline const QString ELEMENT_COMMAND_LINE = "CommandLine";
        static inline const QString ELEMENT_AUTORUN_BEFORE = "AutoRunBefore";
        static inline const QString ELEMENT_NAME = "Name";
        static inline const QString ELEMENT_WAIT_FOR_EXIT = "WaitForExit";
    };

    class Element_CustomField
    {
    public:
        static inline const QString NAME = "CustomField";

        static inline const QString ELEMENT_GAME_ID = "GameID";
        static inline const QString ELEMENT_NAME = "Name";
        static inline const QString ELEMENT_VALUE = "Value";
    };

    class Element_PlaylistHeader
    {
    public:
        static inline const QString NAME = "Playlist";

        static inline const QString ELEMENT_ID = "PlaylistId";
        static inline const QString ELEMENT_NAME = "Name";
        static inline const QString ELEMENT_NESTED_NAME = "NestedName";
        static inline const QString ELEMENT_NOTES = "Notes";
    };

    class Element_PlaylistGame
    {
    public:
        static inline const QString NAME = "PlaylistGame";

        static inline const QString ELEMENT_ID = "GameId";
        static inline const QString ELEMENT_GAME_TITLE = "GameTitle";
        static inline const QString ELEMENT_GAME_FILE_NAME = "GameFileName";
        static inline const QString ELEMENT_GAME_PLATFORM = "GamePlatform";
        static inline const QString ELEMENT_MANUAL_ORDER = "ManualOrder";
        static inline const QString ELEMENT_LB_DB_ID = "LaunchBoxDbId";
    };

    class Element_Platform
    {
    public:
        static inline const QString NAME = "Platform";

        static inline const QString ELEMENT_NAME = "Name";
    };

    class Element_PlatformFolder
    {
    public:
        static inline const QString NAME = "PlatformFolder";

        static inline const QString ELEMENT_MEDIA_TYPE = "MediaType";
        static inline const QString ELEMENT_FOLDER_PATH = "FolderPath";
        static inline const QString ELEMENT_PLATFORM = "Platform";
    };

    class Element_PlatformCategory
    {
    public:
        static inline const QString NAME = "PlatformCategory";
    };

//-Variables--------------------------------------------------------------------------------------------------
    const QString ROOT_ELEMENT = "LaunchBox";
};

class DocKey
{
    friend class Install;
private:
    DocKey() {};
    DocKey(const DocKey&) = default;
};

class XmlDocReader : public virtual Fe::DataDocReader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QXmlStreamReader mStreamReader;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    XmlDocReader(Fe::DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    virtual bool readTargetDoc() = 0;

public:
    Qx::GenericError readInto() override;
};

class XmlDocWriter : public virtual Fe::DataDocWriter
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QXmlStreamWriter mStreamWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    XmlDocWriter(Fe::DataDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    virtual bool writeSourceDoc() = 0;
    void writeCleanTextElement(const QString& qualifiedName, const QString& text);
    void writeOtherFields(const QHash<QString, QString>& otherFields);

public:
    Qx::GenericError writeOutOf() override;
};

class PlatformDoc : public Fe::PlatformDoc
{
    friend class PlatformDocReader;
    friend class PlatformDocWriter;
    friend class Install;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, std::shared_ptr<CustomField>> mCustomFieldsFinal;
    QHash<QString, std::shared_ptr<CustomField>> mCustomFieldsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformDoc(Install* const parent, std::unique_ptr<QFile> xmlFile, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;
    std::shared_ptr<Fe::Game> prepareGame(const Fp::Game& game) override;
    std::shared_ptr<Fe::AddApp> prepareAddApp(const Fp::AddApp& addApp) override;

    void addCustomField(std::shared_ptr<CustomField> customField);

public:
    void finalizeDerived() override;
};

class PlatformDocReader : public Fe::PlatformDocReader, public XmlDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformDocReader(PlatformDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool readTargetDoc() override;
    void parseGame();
    void parseAddApp();
    void parseCustomField();
};

class PlatformDocWriter : public Fe::PlatformDocWriter, public XmlDocWriter
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

class PlaylistDoc : public Fe::PlaylistDoc
{
    friend class PlaylistDocReader;
    friend class PlaylistDocWriter;
    friend class Install;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Qx::FreeIndexTracker<int>* mLaunchBoxDatabaseIdTracker;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlaylistDoc(Install* const parent, std::unique_ptr<QFile> xmlFile, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;
    std::shared_ptr<Fe::PlaylistHeader> preparePlaylistHeader(const Fp::Playlist& playlist) override;
    std::shared_ptr<Fe::PlaylistGame> preparePlaylistGame(const Fp::PlaylistGame& game) override;
};

class PlaylistDocReader : public Fe::PlaylistDocReader, public XmlDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlaylistDocReader(PlaylistDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool readTargetDoc() override;
    void parsePlaylistHeader();
    void parsePlaylistGame();
};

class PlaylistDocWriter : public Fe::PlaylistDocWriter, XmlDocWriter
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

class PlatformsDoc : public Fe::DataDoc
{
    friend class PlatformsDocReader;
    friend class PlatformsDocWriter;
    friend class Install;

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = "Platforms";

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, Platform> mPlatforms;
    QMap<QString, QMap<QString, QString>> mPlatformFolders;
    QList<PlatformCategory> mPlatformCategories;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformsDoc(Install* const parent, std::unique_ptr<QFile> xmlFile, const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    const QHash<QString, Platform>& getPlatforms() const;
    const QMap<QString, QMap<QString, QString>>& getPlatformFolders() const;
    const QList<PlatformCategory>& getPlatformCategories() const;

    bool containsPlatform(QString name);

    void addPlatform(Platform platform);

    void setMediaFolder(QString platform, QString mediaType, QString folderPath);

};

class PlatformsDocReader : public XmlDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformsDocReader(PlatformsDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool readTargetDoc() override;
    void parsePlatform();
    void parsePlatformFolder();
    void parsePlatformCategory();
};

class PlatformsDocWriter : public XmlDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformsDocWriter(PlatformsDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writePlatform(const Platform& platform);
    bool writePlatformFolder(const QString& platform, const QString& mediaType, const QString& folderPath);
    bool writePlatformCategory(const PlatformCategory& platformCategory);
};

}
#endif // LAUNCHBOX_XML_H
