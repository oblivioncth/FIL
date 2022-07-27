#ifndef FE_DATA
#define FE_DATA

// Standard Library Includes
#include <concepts>
#include <memory>

// Qt Includes
#include <QFile>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/utility/qx-macros.h>

// libfp Includes
#include <fp/flashpoint/fp-items.h>

// Project Includes
#include "fe-items.h"

/* TODO: Consider making readers/writers child classes of their respective docs (their composition can still be
 * declared outside the doc class by using Doc::Reader{ declarations... }) so that access to the doc's members
 * is the default and the use of "friend" can be significantly reduced. The same can be done for the builders
 * of items (i.e. ::Builder)
 */

/* TODO: Right now all docs that need to be constructed by an install have that install marked as their friend,
 * but they also are using the Passkey Idiom, a key class with a private constructor that they are also friends
 * with, which is is redundant for the purposes of construction. First see if the docs really need to be friends
 * with the Installs (I think they do for the parent() Install pointer to be used as it is). Then, if they do,
 * the only reason the Passkey Idiom is also being used is because these docs are constructed using
 * std::make_shared<>(); even if the doc itself has the Install marked as a friend, it doesnt have
 * the function std::make_shared<Install>() marked as a friend, so it can't be constructed that way. Because
 * of this the Install constructor has to be public and the idiom used. So, double check the minor differences
 * between constructing an instance on the heap and then creating a smart pointer with the regular pointer vs.
 * using std::make_shared<>(), and see if allowing for its use when creating the docs is really worth also
 * having to do Passkey.
 */

namespace Fe
{
//-External Reference--------------------------------------------------------------------------------------------
class Install;

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
class ImageSources
{
//-Instance Members--------------------------------------------------------------------------------------------------
private:
    QString mLogoPath;
    QString mScreenshotPath;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    ImageSources();
    ImageSources(const QString& logoPath, const QString& screenshotPath);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isNull() const;
    QString logoPath() const;
    QString screenshotPath() const;
    void setLogoPath(const QString& path);
    void setScreenshotPath(const QString& path);
};

class DataDoc
{
    /* TODO: Consider making this a template class where T is the type argument for the doc's parent, so that the
     * parent() method can return the type directly, without a derived document needing to cast to it's parent's type
    */

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
    const QString mDocumentPath;
    const QString mName;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDoc(Install* const parent, const QString& docPath, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual Type type() const = 0;

public:
    Install* parent() const;
    QString path() const;
    Identifier identifier() const;
    virtual bool isEmpty() const = 0;
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
public:
    virtual Qx::GenericError writeOutOf() = 0;
};

class Errorable
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Qx::GenericError mError;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Errorable();

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool hasError() const;
    Qx::GenericError error() const;
};

class UpdateableDoc : public DataDoc
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    UpdateOptions mUpdateOptions;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit UpdateableDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
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
                newItem->transferOtherFields(existingItems[key]->otherFields());
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

    template <typename T>
        requires std::derived_from<T, BasicItem>
    void addUpdateableItem(QHash<QUuid, std::shared_ptr<T>>& existingItems,
                           QHash<QUuid, std::shared_ptr<T>>& finalItems,
                           std::shared_ptr<T> newItem)
    {
        addUpdateableItem(existingItems,
                          finalItems,
                          std::static_pointer_cast<BasicItem>(newItem)->id(),
                          newItem);
    }

public:
    virtual void finalize();
};

class PlatformDoc : public UpdateableDoc, public Errorable
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlatformDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsGame(QUuid gameId) const = 0;
    virtual bool containsAddApp(QUuid addAppId) const = 0;

    /* NOTE: The image paths provided here can be null (i.e. images unavailable). Handle accordingly in derived.
     * Also in most cases, addGame should call parent()->processDirectGameImages()
     */
    virtual void addGame(const Fp::Game& game, const ImageSources& images) = 0;
    virtual void addAddApp(const Fp::AddApp& app) = 0;
};

class BasicPlatformDoc : public PlatformDoc
{
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
    explicit BasicPlatformDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual std::shared_ptr<Game> prepareGame(const Fp::Game& game, const ImageSources& images) = 0;
    virtual std::shared_ptr<AddApp> prepareAddApp(const Fp::AddApp& game) = 0;

public:
    virtual bool isEmpty() const override;

    const QHash<QUuid, std::shared_ptr<Game>>& finalGames() const;
    const QHash<QUuid, std::shared_ptr<AddApp>>& finalAddApps() const;

    bool containsGame(QUuid gameId) const override;
    bool containsAddApp(QUuid addAppId) const override;

    void addGame(const Fp::Game& game, const ImageSources& images) override;
    void addAddApp(const Fp::AddApp& app) override;

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

class PlaylistDoc : public UpdateableDoc, public Errorable
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlaylistDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsPlaylistGame(QUuid gameId) const = 0;

    virtual void setPlaylistHeader(const Fp::Playlist& playlist) = 0;
    virtual void addPlaylistGame(const Fp::PlaylistGame& playlistGame) = 0;
};

class BasicPlaylistDoc : public PlaylistDoc
{
    friend class BasicPlaylistDocReader;
    friend class BasicPlaylistDocWriter;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    std::shared_ptr<PlaylistHeader> mPlaylistHeader;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesFinal;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlaylistDoc(Install* const parent, const QString& docPath, QString docName, UpdateOptions updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual std::shared_ptr<PlaylistHeader> preparePlaylistHeader(const Fp::Playlist& playlist) = 0;
    virtual std::shared_ptr<PlaylistGame> preparePlaylistGame(const Fp::PlaylistGame& game) = 0;

public:
    virtual bool isEmpty() const override;

    const std::shared_ptr<PlaylistHeader>& playlistHeader() const;
    const QHash<QUuid, std::shared_ptr<PlaylistGame>>& finalPlaylistGames() const;

    bool containsPlaylistGame(QUuid gameId) const override;

    void setPlaylistHeader(const Fp::Playlist& playlist) override;
    void addPlaylistGame(const Fp::PlaylistGame& playlistGame) override;

    void finalize() override;
};

class PlaylistDocReader : public virtual DataDocReader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    PlaylistDocReader(DataDoc* targetDoc);
};

/* TODO: Consider making the existing items accessible through a public getter, or at least a function to add
 * them through a public function (similar todo already exists). If this is done then these base readers and writers
 * for specific docs can be removed since they only exist to define the "workaround" getters for existing items.
 *
 * This would mean that virtual inheritance wouldn't be required for the other readers/writers and greatly simplify
 * things
 */
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
