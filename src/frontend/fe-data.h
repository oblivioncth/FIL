#ifndef FE_DATA
#define FE_DATA

// Standard Library Includes
#include <concepts>

// Qt Includes
#include <QFile>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/utility/qx-macros.h>

// libfp Includes
#include <fp/flashpoint/fp-items.h>

// Project Includes
#include "fe-items.h"

namespace Fe
{
//-Enums----------------------------------------------------------------------------------------------------------
enum class ImportMode {OnlyNew, NewAndExisting};
enum class DocHandlingError {DocAlreadyOpen, DocCantOpen, DocCantSave, NotParentDoc, CantRemoveBackup, CantCreateBackup, DocInvalidType, DocReadFailed, DocWriteFailed};
QX_SCOPED_ENUM_HASH_FUNC(DocHandlingError);

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

    bool clearFile();
};
QX_SCOPED_ENUM_HASH_FUNC(DataDoc::Type);

class DataDocReader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mTargetDocument;
    QString mStdReadErrorStr;

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
    QString mStdWriteErrorStr;

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
    virtual void finalizeDerived();

    template <typename T, typename K>
        requires std::derived_from<T, Item>
    void finalizeUpdateableItems(QHash<K, std::shared_ptr<T>>& existingItems,
                                 QHash<K, std::shared_ptr<T>>& finalItems)
    {
        // Copy items to final list if obsolete entries are to be kept
        if(!mUpdateOptions.removeObsolete)
            finalItems.insert(existingItems);

        // Clear existing lists
        existingItems.clear();
    }

    template <typename T, typename K>
        requires std::derived_from<T, Item>
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

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlatformDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual void setGameImageReference(Fp::ImageType imageType, QUuid gameId, QString sourcePath);

    virtual bool containsGame(QUuid gameId) const = 0;
    virtual bool containsAddApp(QUuid addAppId) const = 0;

    virtual const Game* addGame(Fp::Game game) = 0;
    virtual void addAddApp(Fp::AddApp app) = 0;
};

class BasicPlatformDoc : public PlatformDoc
{
    friend class Install;
    friend class BasicPlatformDocReader;
    friend class BasicPlatformDocWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<Game>> mGamesFinal;
    QHash<QUuid, std::shared_ptr<Game>> mGamesExisting;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsFinal;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlatformDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual std::shared_ptr<Game> prepareGame(const Fp::Game& game) = 0;
    virtual std::shared_ptr<AddApp> prepareAddApp(const Fp::AddApp& game) = 0;

public:
    const QHash<QUuid, std::shared_ptr<Game>>& getFinalGames() const;
    const QHash<QUuid, std::shared_ptr<AddApp>>& getFinalAddApps() const;

    bool containsGame(QUuid gameId) const override;
    bool containsAddApp(QUuid addAppId) const override;

    const Game* addGame(Fp::Game game) override;
    void addAddApp(Fp::AddApp app) override;

    void finalize() override;
};

class PlatformDocReader : public virtual DataDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlatformDocReader(DataDoc* targetDoc);
};

class BasicPlatformDocReader : public PlatformDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    BasicPlatformDocReader(DataDoc* targetDoc);

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

class BasicPlatformDocWriter : public PlatformDocWriter
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    BasicPlatformDocWriter(DataDoc* sourceDoc);
};

class PlaylistDoc : public UpdateableDoc
{
    friend class Install;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlaylistDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsPlaylistGame(QUuid gameId) const = 0;

    virtual void setPlaylistHeader(Fp::Playlist playlist) = 0;
    virtual void addPlaylistGame(Fp::PlaylistGame playlistGame) = 0;
};

class BasicPlaylistDoc : public PlaylistDoc
{
    friend class Install;
    friend class BasicPlaylistDocReader;
    friend class BasicPlaylistDocWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    std::shared_ptr<PlaylistHeader> mPlaylistHeader;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesFinal;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlaylistDoc(Install* const parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual std::shared_ptr<PlaylistHeader> preparePlaylistHeader(const Fp::Playlist& playlist) = 0;
    virtual std::shared_ptr<PlaylistGame> preparePlaylistGame(const Fp::PlaylistGame& game) = 0;

public:
    const std::shared_ptr<PlaylistHeader>& getPlaylistHeader() const;
    const QHash<QUuid, std::shared_ptr<PlaylistGame>>& getFinalPlaylistGames() const;

    bool containsPlaylistGame(QUuid gameId) const override;

    void setPlaylistHeader(Fp::Playlist playlist) override;
    void addPlaylistGame(Fp::PlaylistGame playlistGame) override;

    void finalize() override;
};

class PlaylistDocReader : public virtual DataDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlaylistDocReader(DataDoc* targetDoc);
};

class BasicPlaylistDocReader : public PlaylistDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    BasicPlaylistDocReader(DataDoc* targetDoc);

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

class BasicPlaylistDocWriter : public PlaylistDocWriter
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    BasicPlaylistDocWriter(DataDoc* sourceDoc);
};

//-Functions-------------------------------------------------------------------------------------------------------
QString docHandlingErrorString(const DataDoc* doc, DocHandlingError handlingError);

}
#endif // FE_DATA
