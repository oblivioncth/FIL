#ifndef LR_INSTALL_INTERFACE_H
#define LR_INSTALL_INTERFACE_H

// Qt Includes
#include <QDir>

// Project Includes
#include "launcher/interface/lr-data-interface.h"
#include "import/settings.h"

namespace Lr
{

class IInstall
{
//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Validity
    bool mValid;

    // Files and directories
    QDir mRootDirectory;

    // Document tracking
    QSet<IDataDoc::Identifier> mExistingDocuments;
    QSet<IDataDoc::Identifier> mModifiedDocuments;
    QSet<IDataDoc::Identifier> mLeasedDocuments;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    IInstall(const QString& installPath); // TODO: Mabye make this default and have a virtual "init" method that takes the path and returns a bool instead of using declareValid()

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~IInstall();

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static void ensureModifiable(const QString& filePath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    // Support
    bool containsAnyDataDoc(IDataDoc::Type type, const QList<QString>& names) const;
    bool supportsImageMode(Import::ImageMode imageMode) const; // TODO: UNUSED
    QList<QString> modifiedDataDocs(IDataDoc::Type type) const;

protected:
    // Validity
    void declareValid(bool valid);

    // Docs
    DocHandlingError checkoutDataDocument(std::shared_ptr<IDataDoc::Reader> docReader);
    DocHandlingError commitDataDocument(std::shared_ptr<IDataDoc::Writer> docWriter);
    void closeDataDocument(std::unique_ptr<IDataDoc> doc);
    QList<QString> modifiedPlatforms() const;
    QList<QString> modifiedPlaylists() const;
    virtual Qx::Error populateExistingDocs(QSet<IDataDoc::Identifier>& existingDocs) = 0;

public:
    // Details
    virtual QString name() const = 0;
    virtual QList<Import::ImageMode> preferredImageModeOrder() const = 0;
    virtual QString versionString() const;
    virtual bool isRunning() const = 0;

    bool isValid() const;
    QString path() const;
    virtual void softReset();

    // Docs
    virtual QString translateDocName(const QString& originalName, IDataDoc::Type type) const;
    Qx::Error refreshExistingDocs(bool* changed = nullptr);
    bool containsPlatform(const QString& name) const;
    bool containsPlaylist(const QString& name) const;
    bool containsAnyPlatform(const QList<QString>& names) const; // Unused
    bool containsAnyPlaylist(const QList<QString>& names) const; // Unused

    virtual DocHandlingError checkoutPlatformDoc(std::unique_ptr<IPlatformDoc>& returnBuffer, const QString& name) = 0;
    virtual DocHandlingError checkoutPlaylistDoc(std::unique_ptr<IPlaylistDoc>& returnBuffer, const QString& name) = 0;
    virtual DocHandlingError commitPlatformDoc(std::unique_ptr<IPlatformDoc> platformDoc) = 0;
    virtual DocHandlingError commitPlaylistDoc(std::unique_ptr<IPlaylistDoc> playlistDoc) = 0;

    // Import stage notifier hooks
    virtual Qx::Error preImport();
    virtual Qx::Error postImport();
    virtual Qx::Error prePlatformsImport();
    virtual Qx::Error postPlatformsImport();
    virtual Qx::Error preImageProcessing();
    virtual Qx::Error postImageProcessing();
    virtual Qx::Error prePlaylistsImport();
    virtual Qx::Error postPlaylistsImport();

    // Images
    virtual QString getDestinationImagePath(const Game& game, Fp::ImageType type) = 0;
    virtual void processBulkImageSources(const Import::ImagePaths& bulkSources) = 0;
    virtual QString platformCategoryIconPath() const; // Unsupported in default implementation, needs to return path with .png extension
    virtual std::optional<QDir> platformIconsDirectory() const; // Unsupported in default implementation
    virtual std::optional<QDir> playlistIconsDirectory() const; // Unsupported in default implementation
    // TODO: These might need to be changed to support launchers where the platform images are tied closely to the platform documents,
    // but currently none do this so this works.
};

}

#endif // LR_INSTALL_INTERFACE_H
