#ifndef FE_INSTALLFOUNDATION_H
#define FE_INSTALLFOUNDATION_H

// Qt Includes
#include <QDir>

// Project Includes
#include "fe-data.h"

namespace Fe
{

//-Enums----------------------------------------------------------------------------------------------------------
enum class ImageMode {Copy, Reference, Link};

class QX_ERROR_TYPE(RevertError, "Fe::RevertError", 1301)
{
    friend class InstallFoundation;
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

class InstallFoundation
{
//-Class Structs------------------------------------------------------------------------------------------------------
public:
    struct ImportDetails
    {
        UpdateOptions updateOptions;
        ImageMode imageMode;
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
    static inline const QString ERR_UNSUPPORTED_FEATURE = u"A feature unsupported by the frontend was called upon!"_s;

    // Image Errors
    static inline const QString CAPTION_IMAGE_ERR = u"Error importing game image(s)"_s;

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Validity
    bool mValid;

    // Files and directories
    QDir mRootDirectory;

    // Document tracking
    QSet<DataDoc::Identifier> mExistingDocuments;
    QSet<DataDoc::Identifier> mModifiedDocuments;
    QSet<DataDoc::Identifier> mDeletedDocuments;
    QSet<DataDoc::Identifier> mLeasedDocuments;

    // Backup/Deletion tracking
    QStringList mRevertableFilePaths;

protected:
    // Import details
    std::unique_ptr<ImportDetails> mImportDetails;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    InstallFoundation(const QString& installPath);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~InstallFoundation();

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static void allowUserWriteOnFile(const QString& filePath);

public:
    static QString filePathToBackupPath(const QString& filePath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    bool containsAnyDataDoc(DataDoc::Type type, const QList<QString>& names) const;
    QList<QString> modifiedDataDocs(DataDoc::Type type) const;

protected:
    virtual void nullify();
    virtual void softReset();
    void declareValid(bool valid);
    virtual Qx::Error populateExistingDocs() = 0; // Stated redundantly again in Install to make it clear its part of the main interface

    virtual QString translateDocName(const QString& originalName, DataDoc::Type type) const;
    void catalogueExistingDoc(DataDoc::Identifier existingDoc);

    Fe::DocHandlingError checkoutDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDoc::Reader> docReader);
    Fe::DocHandlingError commitDataDocument(DataDoc* docToSave, std::shared_ptr<DataDoc::Writer> docWriter);

    QList<QString> modifiedPlatforms() const;
    QList<QString> modifiedPlaylists() const;

public:
    bool isValid() const;
    QString path() const;

    Qx::Error refreshExistingDocs(bool* changed = nullptr);
    bool containsPlatform(const QString& name) const;
    bool containsPlaylist(const QString& name) const;
    bool containsAnyPlatform(const QList<QString>& names) const;
    bool containsAnyPlaylist(const QList<QString>& names) const;

    void addRevertableFile(const QString& filePath);
    int revertQueueCount() const;
    int revertNextChange(RevertError& error, bool skipOnFail);
};

}

#endif // FE_INSTALLFOUNDATION_H
