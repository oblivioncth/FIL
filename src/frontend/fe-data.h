#ifndef FE_DATA
#define FE_DATA

#include <QFile>

#include "fe-items.h"
#include "../flashpoint/fp-items.h"

namespace Fe
{
//-Enums----------------------------------------------------------------------------------------------------------
enum class ImportMode {OnlyNew, NewAndExisting};

//-Structs---------------------------------------------------------------------------------------------------------
struct UpdateOptions
{
    ImportMode importMode;
    bool removeObsolete;
};

//-Classes-----------------------------------------------------------------------------------------------------------
class DataDoc
{
    /* TODO: Consider making this a template class where T is the type argument for the doc's parent, so that the
     * parent() method can return the type directly, without a derived document needing to cast to it's parent's type
    */

    friend class Install;
    friend class DataDocReader;
    friend class DataDocWriter;
//-Class Enums---------------------------------------------------------------------------------------------------------
public:
    enum class Type {Platform, Playlist, Config};
    enum class StandardError {DocAlreadyOpen, DocCantOpen, DocCantSave, NotParentDoc, CantRemoveBackup, CantCreateBackup, DocTypeMismatch, DocReadFailed, DocWriteFailed};

//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Identifier
    {
        friend bool operator== (const Identifier& lhs, const Identifier& rhs) noexcept;
        friend uint qHash(const Identifier& key, uint seed) noexcept;

    private:
        Type mDocType;
        QString mDocName;

    public:
        Identifier(Type docType, QString docName);

        Type docType() const;
        QString docTypeString() const;
        QString docName() const;
    };

//-Class Variables-----------------------------------------------------------------------------------------------------
private:
    static inline const QHash<Type, QString> TYPE_STRINGS = {
        {Type::Platform, "Platform"},
        {Type::Playlist, "Playlist"},
        {Type::Config, "Config"}
    };

    // Message Macros
    static inline const QString M_DOC_TYPE = "<docType>";
    static inline const QString M_DOC_NAME = "<docName>";
    static inline const QString M_DOC_PARENT = "<docParent>";

    // Standard Errors
    static inline const QHash<StandardError, QString> STD_ERRORS = {
        {StandardError::DocAlreadyOpen, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is already open."},
        {StandardError::DocCantOpen, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") cannot be opened."},
        {StandardError::DocCantSave, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") cannot be saved."},
        {StandardError::NotParentDoc, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is not a" + M_DOC_PARENT + "document."},
        {StandardError::CantRemoveBackup, "The existing backup of the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") could not be removed."},
        {StandardError::CantCreateBackup, "Could not create a backup of the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ")."},
        {StandardError::DocTypeMismatch, "The document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") contained an element that belongs to a different document type than expected."},
        {StandardError::DocReadFailed, "Reading the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") failed."},
        {StandardError::DocWriteFailed, "Writing to the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") failed."}
    };

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Install* const mParent;
    std::unique_ptr<QFile> mDocumentFile;
    QString mName;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual Type type() const = 0;
    QString filePath() const;

public:
    Install* parent() const;
    Identifier identifier() const;
    QString errorString(StandardError error) const;

    void clearFile();
};
QX_SCOPED_ENUM_HASH_FUNC(DataDoc::Type);
QX_SCOPED_ENUM_HASH_FUNC(DataDoc::StandardError);

class DataDocReader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mTargetDocument;
    QString mPrimaryError;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDocReader(DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    std::unique_ptr<QFile>& targetDocFile();

public:
    virtual Qx::GenericError readInto() = 0;
};

class DataDocWriter
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mSourceDocument;
    QString mPrimaryError;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDocWriter(DataDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    std::unique_ptr<QFile>& sourceDocFile();

public:
    virtual Qx::GenericError writeOutOf() = 0;
};

class UpdateableDoc : public DataDoc
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    UpdateOptions mUpdateOptions;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit UpdateableDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual void finalizeDerived() = 0; // Should maybe be option via default empty implementation

    template <typename T, typename K, QX_ENABLE_IF(std::is_base_of<Item, T>)>
    void finalizeUpdateableItems(QHash<K, std::shared_ptr<T>>& existingItems,
                                 QHash<K, std::shared_ptr<T>>& finalItems)
    {
        // Copy items to final list if obsolete entries are to be kept
        if(!mUpdateOptions.removeObsolete)
            finalItems.insert(existingItems);

        // Clear existing lists
        existingItems.clear();
    }

    template <typename T, typename K, QX_ENABLE_IF(std::is_base_of<Item, T>)>
    void addUpdateableItem(QHash<K, std::shared_ptr<T>>& existingItems,
                           QHash<K, std::shared_ptr<T>>& finalItems,
                           K key,
                           std::shared_ptr<T> newItem)
{
    // Check if game exists
    if(existingItems.contains(key))
    {
        // Replace if existing update is on, move existing otherwise
        if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
        {
            newItem->transferOtherFields(existingItems[key]->getOtherFields());
            finalItems[key] = newItem;
            existingItems.remove(key);
        }
        else
        {
            finalItems[key] = std::move(existingItems[key]);
            existingItems.remove(key);
        }

    }
    else
        finalItems[key] = newItem;
}

public:
    virtual void finalize();
};

class PlatformDoc : public UpdateableDoc
{
    friend class Install;
    friend class PlatformDocReader;
    friend class PlatformDocWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<Game>> mGamesFinal;
    QHash<QUuid, std::shared_ptr<Game>> mGamesExisting;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsFinal;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlatformDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

protected:
    virtual std::shared_ptr<Game> prepareGame(const FP::Game& game) = 0;
    virtual std::shared_ptr<AddApp> prepareAddApp(const FP::AddApp& game) = 0;

public:
    virtual void setGameImageReference(FP::ImageType imageType, QUuid gameId, QString sourcePath);

    const QHash<QUuid, std::shared_ptr<Game>>& getFinalGames() const;
    const QHash<QUuid, std::shared_ptr<AddApp>>& getFinalAddApps() const;

    bool containsGame(QUuid gameID) const;
    bool containsAddApp(QUuid addAppId) const;

    const Game* addGame(FP::Game game);
    const AddApp* addAddApp(FP::AddApp app);

    void finalize() override;
};

class PlatformDocReader : public virtual DataDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlatformDocReader(DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<Game>>& targetDocExistingGames();
    QHash<QUuid, std::shared_ptr<AddApp>>& targetDocExistingAddApps();
};

class PlatformDocWriter : public virtual DataDocWriter
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlatformDocWriter(DataDoc* sourceDoc);
};

class PlaylistDoc : public UpdateableDoc
{
    friend class Install;
    friend class PlaylistDocReader;
    friend class PlaylistDocWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    std::shared_ptr<PlaylistHeader> mPlaylistHeader;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesFinal;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlaylistDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

protected:
    virtual std::shared_ptr<PlaylistHeader> preparePlaylistHeader(const FP::Playlist& playlist) = 0;
    virtual std::shared_ptr<PlaylistGame> preparePlaylistGame(const FP::PlaylistGame& game) = 0;

public:
    const std::shared_ptr<PlaylistHeader>& getPlaylistHeader() const;
    const QHash<QUuid, std::shared_ptr<PlaylistGame>>& getFinalPlaylistGames() const;

    bool containsPlaylistGame(QUuid gameID) const;

    void setPlaylistHeader(FP::Playlist playlist);
    void addPlaylistGame(FP::PlaylistGame playlistGame);

    void finalize() override;
};

class PlaylistDocReader : public virtual DataDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlaylistDocReader(DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:

    QHash<QUuid, std::shared_ptr<PlaylistGame>>& targetDocExistingPlaylistGames();
    std::shared_ptr<PlaylistHeader>& targetDocPlaylistHeader();
};

class PlaylistDocWriter : public virtual DataDocWriter
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlaylistDocWriter(DataDoc* sourceDoc);
};

}
#endif // FE_DATA
