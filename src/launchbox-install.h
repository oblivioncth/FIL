#ifndef LAUNCHBOX_INSTALL_H
#define LAUNCHBOX_INSTALL_H

#include <QString>
#include <QDir>
#include <QSet>
#include <QtXml>
#include "qx-io.h"
#include "qx-xml.h"
#include "launchbox.h"
#include "launchbox-xml.h"

namespace LB {

class Install
{
//-Class Forward Declarations--------------------------------------------------------------------------------------
public:
    class XMLReaderLegacy; // TODO: Rework into genericized class child
    class XMLWriterLegacy; // TODO: Rework into genericized class child

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImageMode {Copy, Reference, Link};

//-Class Structs----------------------------------------------------------------------------------------------------
public:
    struct GeneralOptions
    {
        bool includeExtreme;
        // TODO: Add option to import animations
    };

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    //

    // Paths
    static inline const QString PLATFORMS_PATH = "Data/Platforms";
    static inline const QString PLAYLISTS_PATH = "Data/Playlists";
    static inline const QString DATA_PATH = "Data";
    static inline const QString MAIN_EXE_PATH = "LaunchBox.exe";
    static inline const QString PLATFORM_IMAGES_PATH = "Images";
    static inline const QString LOGO_PATH = "Box - Front";
    static inline const QString SCREENSHOT_PATH = "Screenshot - Gameplay";

    // Files
    static inline const QString XML_EXT = ".xml";
    static inline const QString IMAGE_EXT = ".png";
    static inline const QString MODIFIED_FILE_EXT = ".obk";

    // Images Errors
    static inline const QString ERR_IMAGE_WONT_BACKUP = R"(Cannot rename the existing image "%1" for backup.)";
    static inline const QString ERR_IMAGE_WONT_COPY = R"(Cannot copy the image "%1" to "%2".)";
    static inline const QString ERR_IMAGE_WONT_MOVE = R"(Cannot move the image "%1" to "%2".)";
    static inline const QString ERR_IMAGE_WONT_LINK = R"(Cannot create a symbolic link from "%1" to "%2".)";
    static inline const QString ERR_CANT_MAKE_DIR = R"(Could not create the image directory "%1". Make sure you have write permissions at that location.)";

    // Reversion Errors
    static inline const QString ERR_REVERT_CANT_REMOVE_XML = R"(Cannot remove the XML file "%1". It may need to be deleted and have its backup restored manually.)";
    static inline const QString ERR_REVERT_CANT_RESTORE_XML = R"(Cannot restore the XML backup "%1". It may need to be renamed manually.)";
    static inline const QString ERR_REVERT_CANT_REMOVE_IMAGE = R"(Cannot remove the image file "%1". It may need to be deleted manually.)";
    static inline const QString ERR_REVERT_CANT_MOVE_IMAGE = R"(Cannot move the image file "%1" to its original location. It may need to be moved manually.)";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mDataDirectory;
    QDir mPlatformsDirectory;
    QDir mPlaylistsDirectory;
    QDir mPlatformImagesDirectory;

    // XML Information
    QSet<Xml::DataDocHandle> mExistingDocuments;

    // XML Interaction
    QList<QString> mModifiedXMLDocuments;
    QSet<Xml::DataDocHandle> mLeasedHandles;

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
   static QString makeFileNameLBKosher(QString fileName);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
   QString transferImage(ImageMode imageMode, QDir sourceDir, QString destinationSubPath, const LB::Game& game);
   Qx::XmlStreamReaderError openDataDocument(Xml::DataDoc* docToOpen, Xml::DataDocReader* docReader);
   bool saveDataDocument(QString& errorMessage, Xml::DataDoc* docToSave, Xml::DataDocWriter* docWriter);
   QSet<QString> getExistingDocs(QString type) const;

public:
   Qx::IOOpReport populateExistingDocs();

   Qx::XmlStreamReaderError openPlatformDoc(std::unique_ptr<Xml::PlatformDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
   Qx::XmlStreamReaderError openPlaylistDoc(std::unique_ptr<Xml::PlaylistDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
   Qx::XmlStreamReaderError openPlatformsDoc(std::unique_ptr<Xml::PlatformsDoc>& returnBuffer);
   bool savePlatformDoc(QString& errorMessage, std::unique_ptr<Xml::PlatformDoc> document);
   bool savePlaylistDoc(QString& errorMessage, std::unique_ptr<Xml::PlaylistDoc> document);
   bool savePlatformsDoc(QString& errorMessage, std::unique_ptr<Xml::PlatformsDoc> document);

   bool ensureImageDirectories(QString& errorMessage, QString platform);
   bool transferLogo(QString& errorMessage, ImageMode imageMode, QDir logoSourceDir, const LB::Game& game);
   bool transferScreenshot(QString& errorMessage, ImageMode imageMode, QDir screenshotSourceDir, const LB::Game& game);

   int revertNextChange(QString& errorMessage, bool skipOnFail);
   void softReset();

   QString getPath() const;
   int getRevertQueueCount() const;
   QSet<QString> getExistingPlatforms() const;
   QSet<QString> getExistingPlaylists() const;

};

}
#endif // LAUNCHBOX_INSTALL_H
