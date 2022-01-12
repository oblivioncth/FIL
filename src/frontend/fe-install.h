#ifndef FE_INSTALL_H
#define FE_INSTALL_H

#include "fe-data.h"
#include "qx.h"

#include <QDir>

namespace Fe
{

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
    static inline const QString UNSUPPORTED_FEATURE = "A feature unsupported by the frontend was called upon!";

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
    static void allowUserWriteOnFile(QString filePath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void nullify();

    QSet<QString> getExistingDocs(DataDoc::Type docType) const;

    Qx::GenericError openDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDocReader> docReader);
    Qx::GenericError saveDataDocument(DataDoc* docToSave, std::shared_ptr<DataDocWriter> docWriter);
    Qx::GenericError transferImage(bool symlink, ImageType imageType, QDir sourceDir, const Game& game);

protected:
    virtual void nullifyDerived() = 0;
    virtual void softResetDerived() = 0;

    virtual QString dataDocPath(DataDoc::Identifier identifier) const = 0;
    virtual QString imageDestinationPath(ImageType imageType, const Game& game) = 0;

    virtual std::shared_ptr<PlatformDocReader> prepareOpenPlatformDoc(std::unique_ptr<PlatformDoc>& platformDoc) = 0;
    virtual std::shared_ptr<PlaylistDocReader> prepareOpenPlaylistDoc(std::unique_ptr<PlaylistDoc>& playlistDoc) = 0;
    virtual std::shared_ptr<PlatformDocWriter> prepareSavePlatformDoc(const std::unique_ptr<PlatformDoc>& document) = 0;
    virtual std::shared_ptr<PlaylistDocWriter> prepareSavePlaylistDoc(const std::unique_ptr<PlaylistDoc>& document) = 0;

    virtual Qx::GenericError referenceImage(ImageType imageType, QDir sourceDir, const Game& game);

public:
    virtual QString name() = 0;
    virtual ImageRefType imageRefType() = 0;
    bool isValid() const;
    QString getPath() const;

    QSet<QString> getExistingPlatforms() const;
    QSet<QString> getExistingPlaylists() const;

    virtual Qx::GenericError populateExistingDocs(QStringList targetPlatforms, QStringList targetPlaylists) = 0;

    Qx::GenericError openPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
    Qx::GenericError openPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, QString name, UpdateOptions updateOptions);
    Qx::GenericError savePlatformDoc(std::unique_ptr<PlatformDoc> document);
    Qx::GenericError savePlaylistDoc(std::unique_ptr<PlaylistDoc> document);
    Qx::GenericError importImage(ImageMode imageMode, ImageType imageType, QDir sourceDir, const Game& game);
    virtual Qx::GenericError bulkReferenceImages(ImageType imageType, QString rootImagePath);

    void softReset();
    int getRevertQueueCount() const;
    int revertNextChange(Qx::GenericError& error, bool skipOnFail);
};

}
#endif // FE_INSTALL_H
