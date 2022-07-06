#ifndef FE_INSTALL_H
#define FE_INSTALL_H

// Qt Includes
#include <QDir>

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "fe-data.h"

//-Macros-------------------------------------------------------------------------------------------------------------------
#define REGISTER_FRONTEND(fe_name, fe_install, fe_icon_path, fe_helpUrl) \
    class fe_install##Factory : public Fe::InstallFactory \
    { \
    public: \
        fe_install##Factory() \
        { \
            Install::Entry entry { \
                .factory = this, \
                .iconPath = fe_icon_path, \
                .helpUrl = fe_helpUrl \
            }; \
            Fe::Install::registerInstall(fe_name, entry); \
        } \
        virtual std::shared_ptr<Fe::Install> produce(QString installPath) const { return std::make_shared<fe_install>(installPath); } \
    }; \
    static fe_install##Factory _##install##Factory;

namespace Fe
{

//-Forward Declarations-----------------------------------------------------------------------------------------
class InstallFactory;

class Install
{
//-Class Structs------------------------------------------------------------------------------------------------------
public:
    struct Entry
    {
        const InstallFactory* factory;
        const QString* iconPath;
        const QUrl* helpUrl;
    };

//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImageMode {Copy, Reference, Link};
    enum ImageRefType {Single, Bulk, Platform, None};

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Files
    static inline const QString IMAGE_EXT = ".png";
    static inline const QString MODIFIED_FILE_EXT = ".obk";

    // Base errors
    static inline const QString ERR_UNSUPPORTED_FEATURE = "A feature unsupported by the frontend was called upon!";
    static inline const QString ERR_INSEPECTION = "An unexpected error occurred while inspecting the frontend.";

    // Images Errors
    static inline const QString ERR_IMAGE_WONT_BACKUP = R"(Cannot rename the an existing image for backup:)";
    static inline const QString ERR_IMAGE_WONT_COPY = R"(Cannot copy the image "%1" to its destination:)";
    static inline const QString ERR_IMAGE_WONT_LINK = R"(Cannot create a symbolic link for "%1" at:)";
    static inline const QString ERR_CANT_MAKE_DIR = R"(Could not create the following image directory. Make sure you have write permissions at that location.)";
    static inline const QString CAPTION_IMAGE_ERR = "Error importing game image(s)";

    // Reversion Errors
    static inline const QString ERR_REVERT_CANT_REMOVE_DOC = R"(Cannot remove a data document file. It may need to be deleted and have its backup restored manually.)";
    static inline const QString ERR_REVERT_CANT_RESTORE_DOC = R"(Cannot restore a data document backup. It may need to be renamed manually.)";
    static inline const QString ERR_REVERT_CANT_REMOVE_IMAGE = R"(Cannot remove an image file. It may need to be deleted manually.)";

//-Instance Variables--------------------------------------------------------------------------------------------
protected:
    // Validity
    bool mValid;

    // Files and directories
    QString mLinkedClifpPath;
    QDir mRootDirectory;

    // Document tracking
    QSet<DataDoc::Identifier> mExistingDocuments;
    QSet<DataDoc::Identifier> mModifiedDocuments;
    QSet<DataDoc::Identifier> mLeasedDocuments;

    // Image tracking
    QStringList mPurgeableImagePaths;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static void allowUserWriteOnFile(QString filePath);

public:
    // NOTE: Registry put behind function call to avoid SIOF since otherwise initialization of static registry before calls to registerFrontend would not be guaranteed
    static QMap<QString, Entry>& registry();
    static void registerInstall(QString name, Entry entry);
    static std::shared_ptr<Install> acquireMatch(const QString& installPath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    QSet<QString> getExistingDocs(DataDoc::Type docType) const;

protected:
    void nullify();
    virtual void nullifyDerived() = 0;
    virtual void softResetDerived() = 0;

    virtual QString dataDocPath(DataDoc::Identifier identifier) const = 0;

    Qx::GenericError openDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDocReader> docReader);
    Qx::GenericError saveDataDocument(DataDoc* docToSave, std::shared_ptr<DataDocWriter> docWriter);

    virtual std::shared_ptr<PlatformDocReader> prepareOpenPlatformDoc(std::unique_ptr<PlatformDoc>& platformDoc, const QString& name, const UpdateOptions& updateOptions) = 0;
    virtual std::shared_ptr<PlaylistDocReader> prepareOpenPlaylistDoc(std::unique_ptr<PlaylistDoc>& playlistDoc, const QString& name, const UpdateOptions& updateOptions) = 0;
    virtual std::shared_ptr<PlatformDocWriter> prepareSavePlatformDoc(const std::unique_ptr<PlatformDoc>& document) = 0;
    virtual std::shared_ptr<PlaylistDocWriter> prepareSavePlaylistDoc(const std::unique_ptr<PlaylistDoc>& document) = 0;

public:
    void linkClifpPath(QString clifpPath);
    QString linkedClifpPath() const;

    virtual QString name() const = 0;
    virtual QString executablePath() const = 0;
    virtual ImageRefType imageRefType() const = 0;
    virtual bool supportsImageMode(ImageMode imageMode) const = 0;
    virtual QString versionString() const;
    bool isValid() const;
    QString getPath() const;

    QSet<QString> getExistingPlatforms() const;
    QSet<QString> getExistingPlaylists() const;

    virtual Qx::GenericError populateExistingDocs(QStringList targetPlatforms, QStringList targetPlaylists) = 0;

    virtual Qx::GenericError openPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
    virtual Qx::GenericError openPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
    virtual Qx::GenericError savePlatformDoc(std::unique_ptr<PlatformDoc> platformDoc);
    virtual Qx::GenericError savePlaylistDoc(std::unique_ptr<PlaylistDoc> playlistDoc);

    virtual QString imageDestinationPath(Fp::ImageType imageType, const Game& game) const = 0;
    virtual Qx::GenericError referenceImage(Fp::ImageType imageType, QString sourcePath, const Game& game);
    virtual Qx::GenericError bulkReferenceImages(QString logoRootPath, QString screenshotRootPath, QStringList platforms);
    void addPurgeableImagePath(QString imagePath);

    virtual Qx::GenericError preImport();
    virtual Qx::GenericError postImport();
    virtual Qx::GenericError prePlatformsImport();
    virtual Qx::GenericError postPlatformsImport();
    virtual Qx::GenericError prePlaylistsImport();
    virtual Qx::GenericError postPlaylistsImport();

    void softReset();
    int getRevertQueueCount() const;
    int revertNextChange(Qx::GenericError& error, bool skipOnFail);
};

class InstallFactory
{
//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual std::shared_ptr<Install> produce(QString installPath) const = 0;
};

}
#endif // FE_INSTALL_H
