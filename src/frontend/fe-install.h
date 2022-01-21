#ifndef FE_INSTALL_H
#define FE_INSTALL_H

#include "fe-data.h"
#include "qx.h"

#include <QDir>

//-Macros-------------------------------------------------------------------------------------------------------------------
#define REGISTER_FRONTEND(name, install) \
    class install##Factory : public Fe::InstallFactory \
    { \
    public: \
        install##Factory() { Fe::Install::registerInstall(name, this); } \
        virtual std::shared_ptr<Fe::Install> produce(QString installPath) { return std::make_shared<install>(installPath); } \
    }; \
    static install##Factory _##install##Factory;

namespace Fe
{

//-Forward Declarations-----------------------------------------------------------------------------------------
class InstallFactory;

class Install
{
//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum ImageType {Logo, Screenshot};
    enum ImageMode {Copy, Reference, Link};
    enum ImageRefType {Single, Bulk, None};

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Files
    static inline const QString IMAGE_EXT = ".png";
    static inline const QString MODIFIED_FILE_EXT = ".obk";

    // Base errors
    static inline const QString ERR_UNSUPPORTED_FEATURE = "A feature unsupported by the frontend was called upon!";
    static inline const QString ERR_INSEPECTION = "An unexpected error occured while inspecting the frontend.";

    // Images Errors
    static inline const QString ERR_IMAGE_WONT_BACKUP = R"(Cannot rename the an existing image for backup:)";
    static inline const QString ERR_IMAGE_WONT_COPY = R"(Cannot copy the image "%1" to its destination:)";
    static inline const QString ERR_IMAGE_WONT_LINK = R"(Cannot create a symbolic link for "%1" at:)";
    static inline const QString ERR_CANT_MAKE_DIR = R"(Could not create the following image directory. Make sure you have write permissions at that location.)";

    // Reversion Errors
    static inline const QString ERR_REVERT_CANT_REMOVE_DOC = R"(Cannot remove a data document file. It may need to be deleted and have its backup restored manually.)";
    static inline const QString ERR_REVERT_CANT_RESTORE_DOC = R"(Cannot restore a data document backup. It may need to be renamed manually.)";
    static inline const QString ERR_REVERT_CANT_REMOVE_IMAGE = R"(Cannot remove an image file. It may need to be deleted manually.)";

//-Instance Variables--------------------------------------------------------------------------------------------
protected:
    // Validity
    bool mValid;

    // Files and directories
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
    // Registry put behind function call to avoid SIOF since otherwise initialization of static registry before calls to registerFrontend would not be guarenteed
    static QMap<QString, InstallFactory*>& registry();

    static void allowUserWriteOnFile(QString filePath);

public:
    static void registerInstall(QString name, InstallFactory* factory);
    static std::shared_ptr<Install> acquireMatch(const QString& installPath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    QSet<QString> getExistingDocs(DataDoc::Type docType) const;
    Qx::GenericError transferImage(bool symlink, ImageType imageType, QDir sourceDir, const Game& game);

protected:
    void nullify();
    virtual void nullifyDerived() = 0;
    virtual void softResetDerived() = 0;

    virtual QString dataDocPath(DataDoc::Identifier identifier) const = 0;
    virtual QString imageDestinationPath(ImageType imageType, const Game& game) const = 0;

    Qx::GenericError openDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDocReader> docReader);
    Qx::GenericError saveDataDocument(DataDoc* docToSave, std::shared_ptr<DataDocWriter> docWriter);

    virtual std::shared_ptr<PlatformDocReader> prepareOpenPlatformDoc(std::unique_ptr<PlatformDoc>& platformDoc, const QString& name, const UpdateOptions& updateOptions) = 0;
    virtual std::shared_ptr<PlaylistDocReader> prepareOpenPlaylistDoc(std::unique_ptr<PlaylistDoc>& playlistDoc, const QString& name, const UpdateOptions& updateOptions) = 0;
    virtual std::shared_ptr<PlatformDocWriter> prepareSavePlatformDoc(const std::unique_ptr<PlatformDoc>& document) = 0;
    virtual std::shared_ptr<PlaylistDocWriter> prepareSavePlaylistDoc(const std::unique_ptr<PlaylistDoc>& document) = 0;

    virtual Qx::GenericError referenceImage(ImageType imageType, QDir sourceDir, const Game& game);

public:
    QString bullshit() { return "ASAD"; } // TODO: If program compiles with somewhat circular reference, change this to give actual CLIFp path

    virtual QString name() const = 0;
    virtual QString executablePath() const = 0;
    virtual ImageRefType imageRefType() const = 0;
    virtual QString versionString() const;
    bool isValid() const;
    QString getPath() const;

    QSet<QString> getExistingPlatforms() const;
    QSet<QString> getExistingPlaylists() const;

    virtual Qx::GenericError populateExistingDocs(QStringList targetPlatforms, QStringList targetPlaylists) = 0;

    Qx::GenericError openPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
    Qx::GenericError openPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
    Qx::GenericError savePlatformDoc(std::unique_ptr<PlatformDoc> platformDoc);
    Qx::GenericError savePlaylistDoc(std::unique_ptr<PlaylistDoc> playlistDoc);
    Qx::GenericError importImage(ImageMode imageMode, ImageType imageType, QDir sourceDir, const Game& game);
    virtual Qx::GenericError bulkReferenceImages(QString logoRootPath, QString screenshotRootPath, QStringList platforms);

    void softReset();
    int getRevertQueueCount() const;
    int revertNextChange(Qx::GenericError& error, bool skipOnFail);
};

class InstallFactory
{
//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual std::shared_ptr<Install> produce(QString installPath) = 0;
};

}
#endif // FE_INSTALL_H
