#ifndef LAUNCHBOXINSTALL_H
#define LAUNCHBOXINSTALL_H

#include <QString>
#include <QDir>
#include <QSet>
#include <QtXml>
#include "qx-io.h"
#include "qx-xml.h"
#include "launchbox.h"

namespace LB {

class Install
{
//-Class Forward Declarations--------------------------------------------------------------------------------------
public:
    class XMLReader;
    class XMLWriter;

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum XMLDocType {Platform, Playlist};
    enum ImportMode {OnlyNew, NewAndExisting};
    enum ImageMode {LB_Copy, LB_Link, FP_Link};

//-Class Structs----------------------------------------------------------------------------------------------------
public:
    struct UpdateOptions
    {
        ImportMode importMode;
        bool removeObsolete;
    };

    class XMLHandle
    {
    public:
        XMLDocType type;
        QString name;

        friend bool operator== (const XMLHandle& lhs, const XMLHandle& rhs) noexcept;
        friend uint qHash(const XMLHandle& key, uint seed) noexcept;
    };

//-Inner Classes-------------------------------------------------------------------------------------------------
public:    
    class XMLMainElement_Game
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
    };

    class XMLMainElement_AddApp
    {
    public:
        static inline const QString NAME = "AddApplication";

        static inline const QString ELEMENT_ID = "Id";
        static inline const QString ELEMENT_GAME_ID = "GameID";
        static inline const QString ELEMENT_APP_PATH = "ApplicationPath";
        static inline const QString ELEMENT_COMMAND_LINE = "CommandLine";
        static inline const QString ELEMENT_AUTORUN_BEFORE = "AutoRunBefore";
        static inline const QString ELEMENT_NAME = "Name";
        static inline const QString ELEMENT_WAIT_FOR_EXIT = "WaitForExit";
    };

    class XMLMainElement_PlaylistHeader
    {
    public:
        static inline const QString NAME = "Playlist";

        static inline const QString ELEMENT_ID = "PlaylistId";
        static inline const QString ELEMENT_NAME = "Name";
        static inline const QString ELEMENT_NESTED_NAME = "NestedName";
        static inline const QString ELEMENT_NOTES = "Notes";
    };

    class XMLMainElement_PlaylistGame
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

    class XMLDoc
    {
        friend class XMLReader;
        friend class XMLWriter;
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
    private:
        std::unique_ptr<QFile> mDocumentFile;
        XMLHandle mHandleTarget;
        UpdateOptions mUpdateOptions;
        Qx::FreeIndexTracker<int>* mPlaylistGameFreeLBDBIDTracker;

        QHash<QUuid, Game> mGamesFinal;
        QHash<QUuid, Game> mGamesExisting;
        QHash<QUuid, AddApp> mAddAppsFinal;
        QHash<QUuid, AddApp> mAddAppsExisting;
        PlaylistHeader mPlaylistHeader;
        QHash<QUuid, PlaylistGame> mPlaylistGamesFinal;
        QHash<QUuid, PlaylistGame> mPlaylistGamesExisting;


    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        explicit XMLDoc(std::unique_ptr<QFile> xmlFile, XMLHandle xmlMetaData, UpdateOptions updateOptions, Qx::FreeIndexTracker<int>* lbDBFIDT, const Key&);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    public:
        XMLHandle getHandleTarget() const;
        const QHash<QUuid, Game>& getFinalGames() const;
        const QHash<QUuid, AddApp>& getFinalAddApps() const;
        const PlaylistHeader& getPlaylistHeader() const;
        const QHash<QUuid, PlaylistGame>& getFinalPlaylistGames() const;

        bool containsGame(QUuid gameID) const;
        bool containsAddApp(QUuid addAppId) const;
        bool containsPlaylistGame(QUuid gameID) const;

        void addGame(Game game);
        void addAddApp(AddApp app);
        void setPlaylistHeader(PlaylistHeader header);
        void addPlaylistGame(PlaylistGame playlistGame);

        void finalize();
    };

    class XMLReader
    {
    //-Class variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString ERR_DOC_ALREADY_OPEN = "The target XML file (%1 | %2) is already open";
        static inline const QString ERR_DOC_IN_USE = "The target XML file (%1 | %2) is in use by another program";
        static inline const QString ERR_NOT_LB_DOC = "The target XML file (%1 | %2) is not a LaunchBox document.";
        static inline const QString ERR_BAK_WONT_DEL = "The existing backup of the target XML file (%1 | %2) could not be removed.";
        static inline const QString ERR_CANT_MAKE_BAK = "Could not create a backup of the target XML file (%1 | %2).";
        static inline const QString ERR_DOC_TYPE_MISMATCH = "The document (%1 | %2) contained an element that belongs to a different document type than expected.";

    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        QXmlStreamReader mStreamReader;
        XMLDoc* mTargetDocument;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        XMLReader(XMLDoc* targetDoc);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    public:
        Qx::XmlStreamReaderError readInto();

    private:
        Qx::XmlStreamReaderError readLaunchBoxDocument();
        void parseGame();
        void parseAddApp();
        void parsePlaylistHeader();
        void parsePlaylistGame();
    };

    class XMLWriter
    {
        //-Class variables-----------------------------------------------------------------------------------------------------
        public: // TODO: Make sure this error is used with its arugments
            static inline const QString ERR_WRITE_FAILED = "Writing to the target XML file (%1 | %2) failed";

        //-Instance Variables--------------------------------------------------------------------------------------------------
        private:
            QXmlStreamWriter mStreamWriter;
            XMLDoc* mSourceDocument;

        //-Constructor--------------------------------------------------------------------------------------------------------
        public:
            XMLWriter(XMLDoc* sourceDoc);

        //-Instance Functions-------------------------------------------------------------------------------------------------
        public:
            bool writeOutOf();

        private:
            bool writeLaunchBoxDocument();
            bool writeGame(const Game& game);
            bool writeAddApp(const AddApp& addApp);
            bool writePlaylistHeader(const PlaylistHeader& playlistHeader);
            bool writePlaylistGame(const PlaylistGame& playlistGame);
    };

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    // Paths
    static inline const QString PLATFORMS_PATH = "Data/Platforms";
    static inline const QString PLAYLISTS_PATH = "Data/Playlists";
    static inline const QString MAIN_EXE_PATH = "LaunchBox.exe";
    static inline const QString PLATFORM_IMAGES_PATH = "Images";
    static inline const QString LOGO_PATH = "Box - Front";
    static inline const QString SCREENSHOT_PATH = "Screenshot - Gameplay";

    // Images
    static inline const QString IMAGE_EXT = ".png";

    // Images Errors
    static inline const QString ERR_IMAGE_WONT_REMOVE = "Cannot remove the existing image %1";
    static inline const QString ERR_IMAGE_WONT_COPY = "Cannot copy the image %1 to %2";
    static inline const QString ERR_IMAGE_WONT_MOVE = "Cannot move the image %1 to %2";
    static inline const QString ERR_IMAGE_WONT_LINK = "Cannot create a symbolic link from %1 to %2";
    static inline const QString ERR_CANT_MAKE_DIR = "Could not create the image directory %1. Make sure you have write permissions at that location.";

    // XML
    static inline const QString XML_EXT = ".xml";
    static inline const QString XML_ROOT_ELEMENT = "LaunchBox";
    static inline const QString MODIFIED_FILE_EXT = ".obk";

    // Reversion Errors
    static inline const QString ERR_REVERT_CANT_REMOVE_XML = "Cannot remove the XML file %1. It may need to be deleted and have its backup restored manually.";
    static inline const QString ERR_REVERT_CANT_RESTORE_EXML = "Cannot restore the XML backup %1. It may need to be renamed manually.";
    static inline const QString ERR_REVERT_CANT_REMOVE_IMAGE = "Cannot remove the image file %1. It may need to be deleted manually";
    static inline const QString ERR_REVERT_CANT_MOVE_IMAGE = "Cannot move the image file %1 to its original location. It may need to be moved manually";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mPlatformsDirectory;
    QDir mPlaylistsDirectory;
    QDir mPlatformImagesDirectory;

    // XML Information
    QSet<QString> mExistingPlatforms;
    QSet<QString> mExistingPlaylists;

    // XML Interaction
    QList<QString> mModifiedXMLDocuments;
    QSet<XMLHandle> mLeasedHandles;

    // Other trackers
    QList<QString> mPurgableImages;
    QMap<QString, QString> mLinksToReverse;
    Qx::FreeIndexTracker<int> mLBDatabaseIDTracker = Qx::FreeIndexTracker<int>(0, -1, {});

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static void allowUserWriteOnXML(QString xmlPath);

public:
   static bool pathIsValidInstall(QString installPath);
   static QString populateErrorWithTarget(QString error, XMLHandle target);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
   Qx::IOOpReport populateExistingItems();

   Qx::XmlStreamReaderError openXMLDocument(std::unique_ptr<XMLDoc>& returnBuffer, XMLHandle requestHandle, UpdateOptions updateOptions);
   bool saveXMLDocument(std::unique_ptr<XMLDoc> document);
   bool ensureImageDirectories(QString& errorMessage, QString platform);
   bool transferImages(QString& errorMessage, ImageMode imageOption, QDir logoSourceDir, QDir screenshotSourceDir, const LB::Game& game);

   int revertNextChange(QString& errorMessage, bool skipOnFail);
   void softReset();

   QSet<QString> getExistingPlatforms() const;
   QSet<QString> getExistingPlaylists() const;
   int getRevertQueueCount() const;

};

}
#endif // LAUNCHBOXINSTALL_H
