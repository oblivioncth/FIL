#ifndef IMPORT_IMAGE_H
#define IMPORT_IMAGE_H

// Qt Includes
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-abstracterror.h>
#include <qx/network/qx-downloadmanager.h>

// Project Includes
#include "import/settings.h"

namespace Lr
{
class Game;
class IInstall;
}

namespace Fp { class Install; }

namespace Qx { class ProgressGroup; }

namespace Import
{

class QX_ERROR_TYPE(ImageTransferError, "ImageTransferError", 1351)
{
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        ImageSourceUnavailable = 1,
        ImageWontBackup = 2,
        ImageWontCopy = 3,
        ImageWontLink = 4,
        CantCreateDirectory = 5
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {ImageSourceUnavailable, u"An Expected source image does not exist."_s},
        {ImageWontBackup, u"Cannot rename an existing image for backup."_s},
        {ImageWontCopy, u"Cannot copy an image to its destination."_s},
        {ImageWontLink, u"Cannot create a symbolic link for an image."_s},
        {CantCreateDirectory, u"Could not create a directory for an image destination."_s}
    };

    static inline const QString CAPTION_IMAGE_ERR = u"Error importing game image(s)"_s;
    static inline const QString IMAGE_RETRY_PROMPT = u"Retry?"_s;
    static inline const QString SRC_PATH_TEMPLATE = u"Source: %1"_s;
    static inline const QString DEST_PATH_TEMPLATE = u"Destination: %1"_s;

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSourcePath;
    QString mDestinationPath;

//-Constructor-------------------------------------------------------------
public:
    ImageTransferError(Type t = NoError, const QString& src = {}, const QString& dest = {});

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
    QString deriveDetails() const override;
    QString deriveCaption() const override;
};

class ImageManager : public QObject
{
    Q_OBJECT;
//-Inner Classes------------------------------------------------------------------------------------------------
private:
    struct ImageMap
    {
        QString sourcePath;
        QString destPath;
    };

//-Class Variables-------------------------------------------------------------------
private:
    // Files
    static inline const QString PNG_EXT = u"png"_s;
    static inline const QByteArray PNG_MAGIC = "\x89\x50\x4E"_ba; // Missing the "G" but it's fine, lets us always read 3 bytes
    static inline const QString JPG_EXT = u"jpg"_s;
    static inline const QByteArray JPG_MAGIC = "\xFF\xD8\xFF"_ba;

//-Instance Variables-------------------------------------------------------------
private:
    // Installs
    Fp::Install* mFlashpoint;
    Lr::IInstall* mLauncher;

    // Settings
    bool mDownload;
    ImageMode mMode;

    // Processing
    bool& mCanceled;
    Qx::SyncDownloadManager mDownloadManager;
    QList<ImageMap> mTransferJobs;
    Qx::ProgressGroup* mDownloadProgress;
    Qx::ProgressGroup* mImageProgress;
    Qx::ProgressGroup* mIconProgress;

//-Constructor-------------------------------------------------------------
public:
    ImageManager(Fp::Install* fp, Lr::IInstall* lr, bool& canceledFlag);

//-Class Functions-------------------------------------------------------------
private:
    static inline const QString STEP_DOWNLOADING_IMAGES = u"Downloading images..."_s;
    static inline const QString STEP_IMPORTING_IMAGES = u"Importing images..."_s;

//-Instance Functions-------------------------------------------------------------
private:
    QString getFiletypeExtension(const QString& imgPath);
    ImageMap createImageTransfer(const Lr::Game& game, const QFileInfo& srcInfo, Fp::ImageType type);
    ImageTransferError transferImage(bool symlink, const QString& sourcePath, const QString& destPath);
    bool performImageJobs(const QList<ImageMap>& jobs, bool symlink, Qx::ProgressGroup* pg);

public:
    // Setup
    void setDownload(bool download);
    void setMode(ImageMode mode);
    void setProgressGroups(Qx::ProgressGroup* download, Qx::ProgressGroup* image, Qx::ProgressGroup* icon);

    // Process
    void prepareGameImages(const Lr::Game& game);
    Qx::DownloadManagerReport downloadImages();
    bool importImages();
    Qx::Error importIcons(bool& canceled, const QStringList& platforms, const QList<Fp::Playlist>& playlists);

//-Signal--------------------------------------------------------------------------
signals:
    void progressStepChanged(const QString& currentStep);
    void blockingErrorOccured(std::shared_ptr<int> response, const Qx::Error& blockingError, QMessageBox::StandardButtons choices);
    void authenticationRequired(const QString& prompt, QAuthenticator* authenticator);
};

}

#endif // IMPORT_IMAGE_H
