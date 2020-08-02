#ifndef LAUNCHBOXINSTALL_H
#define LAUNCHBOXINSTALL_H

#include <QString>
#include <QDir>
#include <QSet>
#include <QtXml>
#include "qx-io.h"
#include "launchbox.h"

namespace LB {

class LaunchBoxInstall
{
//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum XMLDocType {Platform, Playlist};

//-Class Structs----------------------------------------------------------------------------------------------------
public:
    struct XMLHandle
    {
        XMLDocType type;
        QString name;

        friend inline bool operator== (const XMLHandle& lhs, const XMLHandle& rhs) noexcept;
        friend inline uint qHash(const XMLHandle& key, uint seed) noexcept;
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

    class XMLMainElement_AdditionalApp
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

        static inline const QString ELEMENT_GAME_ID = "GameId";
        static inline const QString ELEMENT_GAME_TITLE = "GameTitle";
        static inline const QString ELEMENT_GAME_PLATFORM = "GamePlatform";
        static inline const QString ELEMENT_MANUAL_ORDER = "ManualOrder";
        static inline const QString ELEMENT_LB_DB_ID = "LaunchBoxDbId";
    };

    class LBXMLDoc
    {
    //-Inner Classes----------------------------------------------------------------------------------------------------
    public:
        class Key
        {
            friend class LaunchBoxInstall;
        private:
            Key() {};
            Key(const Key&) = default;
        };

    //-Class Variables--------------------------------------------------------------------------------------------------
    private:
        const QList<LaunchBoxGame> DUMMY_GAME_LIST;
        QList<LaunchBoxAdditionalApp> DUMMY_ADDITIONAL_APP_LIST;
        LaunchBoxPlaylistHeader DUMMY_PLAYLIST_HEADER;
        QList<LaunchBoxPlaylistGame> DUMMY_PLAYLIST_GAME_LIST;


    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        std::unique_ptr<QFile> mDocumentFile;
        XMLHandle mHandleTarget;

        QList<LaunchBoxGame> mGames;
        QList<LaunchBoxAdditionalApp> mAdditionalApps;
        LaunchBoxPlaylistHeader mPlaylistHeader;
        QList<LaunchBoxPlaylistGame> mPlaylistGames;


    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        explicit LBXMLDoc(std::unique_ptr<QFile> xmlFile,  XMLHandle xmlMetaData, const Key&);

    //-Instance Functions--------------------------------------------------------------------------------------------------
    public:
        QXmlStreamReader::Error readAll();
        void close(bool flushData = true);

        bool isValid() const;
        XMLHandle getHandleTarget() const;
        const QList<LaunchBoxGame>& getGames() const;
        const QList<LaunchBoxAdditionalApp>& getAdditionalApps() const;
        const LaunchBoxPlaylistHeader& getPlaylistHeader() const;
        const QList<LaunchBoxPlaylistGame>& getPlaylistGames() const;

       void addGame(LaunchBoxGame game);
       void addAdditionalApp(LaunchBoxAdditionalApp app);
       void setPlaylistHeader(LaunchBoxPlaylistHeader header);
       void addPlaylistGame(LaunchBoxPlaylistGame playlistGame);
    };

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    // Paths
    static inline const QString PLATFORMS_PATH = "Data/Platforms";
    static inline const QString PLAYLISTS_PATH = "Data/Playlists";
    static inline const QString MAIN_EXE_PATH = "LaunchBox.exe";

    // XML
    static inline const QString XML_EXT = ".xml";
    static inline const QString XML_ROOT_ELEMENT = "LaunchBox";

    // General
    static inline const QString MODIFIED_FILE_EXT = ".obk";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mPlatformsDirectory;
    QDir mPlaylistsDirectory;

    // XML Information
    QSet<QString> mExistingPlatforms;
    QSet<QString> mExistingPlaylists;

    // XML Interaction
    QSet<QString> mModifiedXMLDocuments;
    QHash<XMLHandle, std::shared_ptr<LBXMLDoc>> mLeasedHandles;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxInstall(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
public:
   static bool pathIsValidLaunchBoxInstall(QString installPath);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
   Qx::IO::IOOpReport populateExistingItems();

   std::shared_ptr<LBXMLDoc> openXMLDocument(XMLHandle requestHandle);
   bool saveXMLDocument(std::shared_ptr<LBXMLDoc> document);
   bool revertAllChanges();

   QSet<QString> getExistingPlatforms() const;
   QSet<QString> getExistingPlaylists() const;

};

}
#endif // LAUNCHBOXINSTALL_H
