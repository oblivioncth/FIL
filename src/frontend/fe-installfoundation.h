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
public:
    // Files
    static inline const QString IMAGE_EXT = ".png";
    static inline const QString BACKUP_FILE_EXT = ".fbk";

    // Base errors
    static inline const QString ERR_UNSUPPORTED_FEATURE = "A feature unsupported by the frontend was called upon!";
    static inline const QString ERR_INSEPECTION = "An unexpected error occurred while inspecting the frontend.";

    // File Errors
    static inline const QString ERR_FILE_WONT_BACKUP = R"(Cannot rename an existing file for backup:)";
    static inline const QString ERR_FILE_WONT_COPY = R"(Cannot copy the file "%1" to its destination:)";
    static inline const QString ERR_FILE_WONT_LINK = R"(Cannot create a symbolic link for "%1" at:)";
    static inline const QString ERR_CANT_MAKE_DIR = R"(Could not create the following directory. Make sure you have write permissions at that location.)";
    static inline const QString ERR_FILE_WONT_DEL = R"(Cannot remove a file. It may need to be deleted manually.)";
    static inline const QString ERR_FILE_WONT_RESTORE = R"(Cannot restore a file backup. It may need to be renamed manually.)";

    // Image Errors
    static inline const QString CAPTION_IMAGE_ERR = "Error importing game image(s)";

//-Instance Variables--------------------------------------------------------------------------------------------
private:
    // Validity
    bool mValid;

    // Files and directories
    QDir mRootDirectory;

    // Document tracking
    QSet<DataDoc::Identifier> mExistingDocuments;
    QSet<DataDoc::Identifier> mModifiedDocuments;
    QSet<DataDoc::Identifier> mLeasedDocuments;

    // Backup/Deletion tracking
    QStringList mRevertableFilePaths;

protected:
    // Import details
    std::unique_ptr<ImportDetails> mImportDetails;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    InstallFoundation(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static void allowUserWriteOnFile(QString filePath);
    static QString filePathToBackupPath(QString filePath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    bool containsAnyDataDoc(DataDoc::Type type, const QList<QString>& names) const;

protected:
    virtual void nullify();
    virtual void softReset();
    void declareValid(bool valid);
    virtual Qx::GenericError populateExistingDocs() = 0; // Stated redundantly again in Install to make it clear its part of the main interface

    virtual QString translateDocName(const QString& originalName, DataDoc::Type type) const;
    void catalogueExistingDoc(DataDoc::Identifier existingDoc);

    Qx::GenericError checkoutDataDocument(DataDoc* docToOpen, std::shared_ptr<DataDocReader> docReader);
    Qx::GenericError commitDataDocument(DataDoc* docToSave, std::shared_ptr<DataDocWriter> docWriter);

public:
    bool isValid() const;
    QString path() const;

    Qx::GenericError refreshExistingDocs(bool* changed = nullptr);
    bool containsPlatform(const QString& name) const;
    bool containsPlaylist(const QString& name) const;
    bool containsAnyPlatform(const QList<QString>& names) const;
    bool containsAnyPlaylist(const QList<QString>& names) const;

    void addRevertableFile(QString filePath);
    int revertQueueCount() const;
    int revertNextChange(Qx::GenericError& error, bool skipOnFail);
};

}

#endif // FE_INSTALLFOUNDATION_H
