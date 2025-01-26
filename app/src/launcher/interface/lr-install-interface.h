#ifndef LR_INSTALL_INTERFACE_H
#define LR_INSTALL_INTERFACE_H

// Qt Includes
#include <QDir>

// Project Includes
#include "launcher/interface/lr-data-interface.h"
#include "import/settings.h"

namespace Lr
{

class QX_ERROR_TYPE(RevertError, "Lr::RevertError", 1301)
{
    friend class IInstall;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        FileWontDelete = 1,
        FileWontRestore = 2
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {FileWontDelete, u"Cannot remove a file. It may need to be deleted manually."_s},
        {FileWontRestore, u"Cannot restore a file backup. It may need to be renamed manually.."_s}
    };

    static inline const QString CAPTION_REVERT_ERR = u"Error reverting changes"_s;

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    RevertError(Type t, const QString& s);

public:
    RevertError();

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
    QString deriveCaption() const override;
};

class IInstall
{
//-Class Structs------------------------------------------------------------------------------------------------------
public:
    struct ImportDetails
    {
        Import::UpdateOptions updateOptions;
        Import::ImageMode imageMode;
        QString clifpPath;
        QList<QString> involvedPlatforms;
        QList<QString> involvedPlaylists;
    };

    struct ImageMap
    {
        QString sourcePath;
        QString destPath;
    };

//-Class Variables-----------------------------------------------------------------------------------------------
private:
    // Files
    static inline const QString BACKUP_FILE_EXT = u"fbk"_s;

protected:
    // Files
    static inline const QString IMAGE_EXT = u"png"_s;

public:
    // Base errors
    // TODO: This is unused, should it be in-use somewhere?
    static inline const QString ERR_UNSUPPORTED_FEATURE = u"A feature unsupported by the launcher was called upon!"_s;

    // Image Errors
    static inline const QString CAPTION_IMAGE_ERR = u"Error importing game image(s)"_s;

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Validity
    bool mValid;

    // Files and directories
    QDir mRootDirectory;

    // Document tracking
    QSet<IDataDoc::Identifier> mExistingDocuments;
    QSet<IDataDoc::Identifier> mModifiedDocuments;
    QSet<IDataDoc::Identifier> mDeletedDocuments;
    QSet<IDataDoc::Identifier> mLeasedDocuments;

    // Backup/Deletion tracking
    QStringList mRevertableFilePaths;

    // Import details
    std::optional<ImportDetails> mImportDetails;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    IInstall(const QString& installPath);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~IInstall();

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static void ensureModifiable(const QString& filePath);

public:
    // TODO: Improve the backup system so that its more encapsulated and this doesn't need to be public
    static QString filePathToBackupPath(const QString& filePath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    // Support
    bool containsAnyDataDoc(IDataDoc::Type type, const QList<QString>& names) const;
    bool supportsImageMode(Import::ImageMode imageMode) const; // TODO: UNUSED
    QList<QString> modifiedDataDocs(IDataDoc::Type type) const;

protected:
    // Validity
    virtual void nullify();
    void declareValid(bool valid);

    // Docs
    virtual Qx::Error populateExistingDocs() = 0;
    void catalogueExistingDoc(IDataDoc::Identifier existingDoc);
    DocHandlingError checkoutDataDocument(IDataDoc* docToOpen, std::shared_ptr<IDataDoc::Reader> docReader);
    DocHandlingError commitDataDocument(IDataDoc* docToSave, std::shared_ptr<IDataDoc::Writer> docWriter);
    QList<QString> modifiedPlatforms() const;
    QList<QString> modifiedPlaylists() const;

public:
    // Details
    virtual QString name() const = 0;
    virtual QList<Import::ImageMode> preferredImageModeOrder() const = 0;
    virtual QString versionString() const;
    virtual bool isRunning() const = 0;

    bool isValid() const;
    QString path() const;
    virtual void softReset();

    // Import
    ImportDetails importDetails() const;

    // Docs
    virtual QString translateDocName(const QString& originalName, IDataDoc::Type type) const;
    Qx::Error refreshExistingDocs(bool* changed = nullptr);
    bool containsPlatform(const QString& name) const;
    bool containsPlaylist(const QString& name) const;
    bool containsAnyPlatform(const QList<QString>& names) const;
    bool containsAnyPlaylist(const QList<QString>& names) const;

    virtual DocHandlingError checkoutPlatformDoc(std::unique_ptr<IPlatformDoc>& returnBuffer, const QString& name) = 0;
    virtual DocHandlingError checkoutPlaylistDoc(std::unique_ptr<IPlaylistDoc>& returnBuffer, const QString& name) = 0;
    virtual DocHandlingError commitPlatformDoc(std::unique_ptr<IPlatformDoc> platformDoc) = 0;
    virtual DocHandlingError commitPlaylistDoc(std::unique_ptr<IPlaylistDoc> playlistDoc) = 0;

    // Reversion
    void addRevertableFile(const QString& filePath);
    int revertQueueCount() const;
    int revertNextChange(RevertError& error, bool skipOnFail);

    // Import stage notifier hooks
    virtual Qx::Error preImport(const ImportDetails& details);
    virtual Qx::Error postImport();
    virtual Qx::Error prePlatformsImport();
    virtual Qx::Error postPlatformsImport();
    virtual Qx::Error preImageProcessing(const ImagePaths& bulkSources);
    virtual Qx::Error postImageProcessing();
    virtual Qx::Error prePlaylistsImport();
    virtual Qx::Error postPlaylistsImport();

    // Images
    virtual QString platformCategoryIconPath() const; // Unsupported in default implementation, needs to return path with .png extension
    virtual std::optional<QDir> platformIconsDirectory() const; // Unsupported in default implementation
    virtual std::optional<QDir> playlistIconsDirectory() const; // Unsupported in default implementation
    // TODO: These might need to be changed to support launchers where the platform images are tied closely to the platform documents,
    // but currently none do this so this works.
};

}

#endif // LR_INSTALL_INTERFACE_H
