#ifndef LAUNCHBOX_XML_H
#define LAUNCHBOX_XML_H

#include <QString>
#include <QFile>
#include <memory>
#include "qx.h"
#include "qx-xml.h"
#include "lb-items.h"

namespace LB {

//-Enums----------------------------------------------------------------------------------------------------------
enum ImportMode {OnlyNew, NewAndExisting};

//-Structs---------------------------------------------------------------------------------------------------------
struct UpdateOptions
{
    ImportMode importMode;
    bool removeObsolete;
};

class Xml
{
//-Class Forward Declarations--------------------------------------------------------------------------------------
public:
    class DataDocReader;
    class DataDocWriter;
    class PlatformDocReader;
    class PlatformDocWriter;
    class PlaylistDocReader;
    class PlaylistDocWriter;
    class PlatformsDocReader;
    class PlatformsDocWriter;

//-Class Structs---------------------------------------------------------------------------------------------------
public:
    struct DataDocHandle
    {
        QString docType;
        QString docName;

        friend bool operator== (const DataDocHandle& lhs, const DataDocHandle& rhs) noexcept;
        friend uint qHash(const DataDocHandle& key, uint seed) noexcept;
    };

//-Inner Classes-------------------------------------------------------------------------------------------------
public:
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

    class DataDoc
    {
        friend class DataDocReader;
        friend class DataDocWriter;
        friend class Install;
    //-Inner Classes----------------------------------------------------------------------------------------------------
    public:
        class Key
        {
            friend class Install;
        private:
            Key() {};
            Key(const Key&) = default;
        };

    //-Instance Variables--------------------------------------------------------------------------------------------------
    protected:
        std::unique_ptr<QFile> mDocumentFile;
        DataDocHandle mHandleTarget;

    //-Constructor--------------------------------------------------------------------------------------------------------
    protected:
        DataDoc(std::unique_ptr<QFile> xmlFile, DataDocHandle handle);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    public:
        DataDocHandle getHandleTarget() const;
        void clearFile();
    };

    class DataDocReader
    {
    //-Instance Variables--------------------------------------------------------------------------------------------------
    protected:
        DataDoc* mTargetDocument;
        QXmlStreamReader mStreamReader;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        DataDocReader(DataDoc* targetDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        virtual bool readTargetDoc() = 0;

    public:
        Qx::XmlStreamReaderError readInto();
    };

    class DataDocWriter
    {
    //-Instance Variables--------------------------------------------------------------------------------------------------
    protected:
        DataDoc* mSourceDocument;
        QXmlStreamWriter mStreamWriter;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        DataDocWriter(DataDoc* sourceDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    protected:
        virtual bool writeSourceDoc() = 0;
        void writeCleanTextElement(const QString &qualifiedName, const QString &text);
        void writeOtherFields(const QHash<QString, QString>& otherFields);

    public:
        QString writeOutOf();
    };

    class ConfigDoc : public DataDoc
    {
    //-Class Variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString TYPE_NAME = "Config";

    //-Constructor--------------------------------------------------------------------------------------------------------
    protected:
        explicit ConfigDoc(std::unique_ptr<QFile> xmlFile, DataDocHandle handle);
    };

    class PlatformDoc : public DataDoc
    {
        friend class PlatformDocReader;
        friend class PlatformDocWriter;
        friend class Install;

    //-Class Variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString TYPE_NAME = "Platform";

    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        UpdateOptions mUpdateOptions;

        QHash<QUuid, Game> mGamesFinal;
        QHash<QUuid, Game> mGamesExisting;
        QHash<QUuid, AddApp> mAddAppsFinal;
        QHash<QUuid, AddApp> mAddAppsExisting;
        QHash<QString, CustomField> mCustomFieldsFinal;
        QHash<QString, CustomField> mCustomFieldsExisting;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        explicit PlatformDoc(std::unique_ptr<QFile> xmlFile, QString docName, UpdateOptions updateOptions, const Key&);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    public:
        const QHash<QUuid, Game>& getFinalGames() const;
        const QHash<QUuid, AddApp>& getFinalAddApps() const;
        const QHash<QString, CustomField> getFinalCustomFields() const;

        bool containsGame(QUuid gameID) const;
        bool containsAddApp(QUuid addAppId) const;

        void addGame(Game game);
        void addAddApp(AddApp app);
        void addCustomField(CustomField customField);

        void finalize();
    };

    class PlatformDocReader : public DataDocReader
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PlatformDocReader(PlatformDoc* targetDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    public:
        Qx::XmlStreamReaderError readInto();

    private:
        bool readTargetDoc();
        void parseGame();
        void parseAddApp();
        void parseCustomField();
    };

    class PlatformDocWriter : public DataDocWriter
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PlatformDocWriter(PlatformDoc* sourceDoc);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    private:
        bool writeSourceDoc();
        bool writeGame(const Game& game);
        bool writeAddApp(const AddApp& addApp);
        bool writeCustomField(const CustomField& customField);
    };

    class PlaylistDoc : public DataDoc
    {
        friend class PlaylistDocReader;
        friend class PlaylistDocWriter;
        friend class Install;

    //-Class Variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString TYPE_NAME = "Playlist";

    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        UpdateOptions mUpdateOptions;
        Qx::FreeIndexTracker<int>* mPlaylistGameFreeLBDBIDTracker;

        PlaylistHeader mPlaylistHeader;
        QHash<QUuid, PlaylistGame> mPlaylistGamesFinal;
        QHash<QUuid, PlaylistGame> mPlaylistGamesExisting;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        explicit PlaylistDoc(std::unique_ptr<QFile> xmlFile, QString docName, UpdateOptions updateOptions, Qx::FreeIndexTracker<int>* lbDBFIDT, const Key&);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    public:
        const PlaylistHeader& getPlaylistHeader() const;
        const QHash<QUuid, PlaylistGame>& getFinalPlaylistGames() const;

        bool containsPlaylistGame(QUuid gameID) const;

        void setPlaylistHeader(PlaylistHeader header);
        void addPlaylistGame(PlaylistGame playlistGame);

        void finalize();
    };

    class PlaylistDocReader : public DataDocReader
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PlaylistDocReader(PlaylistDoc* targetDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    public:
        Qx::XmlStreamReaderError readInto();

    private:
        bool readTargetDoc();
        void parsePlaylistHeader();
        void parsePlaylistGame();
    };

    class PlaylistDocWriter : public DataDocWriter
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PlaylistDocWriter(PlaylistDoc* sourceDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        bool writeSourceDoc();
        bool writePlaylistHeader(const PlaylistHeader& playlistHeader);
        bool writePlaylistGame(const PlaylistGame& playlistGame);
    };

    class PlatformsDoc : public ConfigDoc
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
        explicit PlatformsDoc(std::unique_ptr<QFile> xmlFile, const Key&);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    public:
        const QHash<QString, Platform>& getPlatforms() const;
        const QMap<QString, QMap<QString, QString>>& getPlatformFolders() const;
        const QList<PlatformCategory>& getPlatformCategories() const;

        bool containsPlatform(QString name);

        void addPlatform(Platform platform);

        void setMediaFolder(QString platform, QString mediaType, QString folderPath);

    };

    class PlatformsDocReader : public DataDocReader
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PlatformsDocReader(PlatformsDoc* targetDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        bool readTargetDoc();
        void parsePlatform();
        void parsePlatformFolder();
        void parsePlatformCategory();
    };

    class PlatformsDocWriter : public DataDocWriter
    {
    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        PlatformsDocWriter(PlatformsDoc* sourceDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        bool writeSourceDoc();
        bool writePlatform(const Platform& platform);
        bool writePlatformFolder(const QString& platform, const QString& mediaType, const QString& folderPath);
        bool writePlatformCategory(const PlatformCategory& platformCategory);
    };

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    static inline const QString ERR_DOC_ALREADY_OPEN = "The target XML file (%1 | %2) is already open";
    static inline const QString ERR_DOC_CANT_OPEN = "The target XML file (%1 | %2) cannot be opened; %3";
    static inline const QString ERR_NOT_LB_DOC = "The target XML file (%1 | %2) is not a LaunchBox document.";
    static inline const QString ERR_BAK_WONT_DEL = "The existing backup of the target XML file (%1 | %2) could not be removed.";
    static inline const QString ERR_CANT_MAKE_BAK = "Could not create a backup of the target XML file (%1 | %2).";
    static inline const QString ERR_DOC_TYPE_MISMATCH = "The document (%1 | %2) contained an element that belongs to a different document type than expected.";
    static inline const QString ERR_WRITE_FAILED = "Writing to the target XML file (%1 | %2) failed";

    static inline const QString XML_ROOT_ELEMENT = "LaunchBox";

//-Class Functions--------------------------------------------------------------------------------------------------
public:
    static QString formatDataDocError(QString errorTemplate, DataDocHandle docHandle);
};

}
#endif // LAUNCHBOX_XML_H
