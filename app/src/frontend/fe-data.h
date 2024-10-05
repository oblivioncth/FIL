#ifndef FE_DATA
#define FE_DATA

// Standard Library Includes
#include <concepts>
#include <memory>

// Qt Includes
#include <QFile>

// Qx Includes
#include <qx/core/qx-error.h>
#include <qx/core/qx-abstracterror.h>
#include <qx/utility/qx-macros.h>
#include <qx/utility/qx-concepts.h>

// libfp Includes
#include <fp/fp-items.h>

// Project Includes
#include "fe-items.h"

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
//-Concepts------------------------------------------------------------------------------------------------------
template<typename T>
concept raw_item = std::derived_from<T, Item>;

template<typename T>
concept shared_item = Qx::specializes<T, std::shared_ptr> && std::derived_from<typename T::element_type, Item>;

template<typename T>
concept raw_basic_item = std::derived_from<T, BasicItem>;

template<typename T>
concept shared_basic_item = Qx::specializes<T, std::shared_ptr> && std::derived_from<typename T::element_type, BasicItem>;

template<typename T>
concept item = raw_item<T> || shared_item<T>;

template<typename T>
concept basic_item = raw_basic_item<T> || shared_basic_item<T>;

template<class K>
concept updateable_item_container = Qx::qassociative<K> && item<typename K::mapped_type>;

template<class K>
concept updateable_basicitem_container = Qx::qassociative<K> && basic_item<typename K::mapped_type> &&
                                         std::same_as<typename K::key_type, QUuid>;

//-External Reference--------------------------------------------------------------------------------------------
class Install;
class DataDoc;

//-Enums----------------------------------------------------------------------------------------------------------
enum class ImportMode {OnlyNew, NewAndExisting};

//-Structs---------------------------------------------------------------------------------------------------------
struct UpdateOptions
{
    ImportMode importMode;
    bool removeObsolete;
};

//-Classes-----------------------------------------------------------------------------------------------------------
class QX_ERROR_TYPE(DocHandlingError, "Fe::DocHandlingError", 1310)
{
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        DocAlreadyOpen = 1,
        DocCantOpen = 2,
        DocCantSave = 3,
        NotParentDoc = 4,
        CantRemoveBackup = 5,
        CantCreateBackup = 6,
        DocInvalidType = 7,
        DocReadFailed = 8,
        DocWriteFailed = 9
    };

//-Class Variables-------------------------------------------------------------
private:
    // Message Macros
    static inline const QString M_DOC_TYPE = u"<docType>"_s;
    static inline const QString M_DOC_NAME = u"<docName>"_s;
    static inline const QString M_DOC_PARENT = u"<docParent>"_s;

    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {DocAlreadyOpen, u"The target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") is already open."_s},
        {DocCantOpen, u"The target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") cannot be opened."_s},
        {DocCantSave, u"The target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") cannot be saved."_s},
        {NotParentDoc, u"The target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") is not a"_s + M_DOC_PARENT + u"document."_s},
        {CantRemoveBackup, u"The existing backup of the target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") could not be removed."_s},
        {CantCreateBackup, u"Could not create a backup of the target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u")."_s},
        {DocInvalidType, u"The document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") is invalid or of the wrong type."_s},
        {DocReadFailed, u"Reading the target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") failed."_s},
        {DocWriteFailed, u"Writing to the target document ("_s + M_DOC_TYPE + u" | "_s + M_DOC_NAME + u") failed."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mErrorStr;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
public:
    DocHandlingError();
    DocHandlingError(const DataDoc& doc, Type t, const QString& s = {});

//-Class Functions-------------------------------------------------------------
private:
    static QString generatePrimaryString(const DataDoc& doc, Type t);

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;

    QString errorString() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

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
    class Reader;
    class Writer;

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
        {Type::Platform, u"Platform"_s},
        {Type::Playlist, u"Playlist"_s},
        {Type::Config, u"Config"_s}
    };

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Install* const mParent;
    const QString mDocumentPath;
    const QString mName;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDoc(Install* const parent, const QString& docPath, QString docName);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~DataDoc();

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

class DataDoc::Reader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mTargetDocument;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Reader(DataDoc* targetDoc);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Reader();

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    virtual Fe::DocHandlingError readInto() = 0;
};

class DataDoc::Writer
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mSourceDocument;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Writer(DataDoc* sourceDoc);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Writer();

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    virtual Fe::DocHandlingError writeOutOf() = 0;
};

class Errorable
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Qx::Error mError;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Errorable();

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Errorable();

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool hasError() const;
    Qx::Error error() const;
};

class UpdateableDoc : public DataDoc
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    UpdateOptions mUpdateOptions;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit UpdateableDoc(Install* const parent, const QString& docPath, QString docName, const UpdateOptions& updateOptions);

//-Class Functions-----------------------------------------------------------------------------------------------------
template<typename T>
T* itemPtr(T& item) { return &item; }

template<typename T>
T* itemPtr(std::shared_ptr<T> item) { return item.get(); }

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    template <typename C>
        requires updateable_item_container<C>
    void finalizeUpdateableItems(C& existingItems,
                                 C& finalItems)
    {
        // Copy items to final list if obsolete entries are to be kept
        if(!mUpdateOptions.removeObsolete)
            finalItems.insert(existingItems);

        // Clear existing lists
        existingItems.clear();
    }

    template <typename C>
        requires updateable_item_container<C>
    void addUpdateableItem(C& existingItems,
                           C& finalItems,
                           C::key_type key,
                           C::mapped_type newItem)
    {
        // Check if item exists
        if(existingItems.contains(key))
        {
            // Replace if existing update is on, move existing otherwise
            if(mUpdateOptions.importMode == ImportMode::NewAndExisting)
            {
                itemPtr(newItem)->transferOtherFields(itemPtr(existingItems[key])->otherFields());
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

    template <typename C>
        requires updateable_basicitem_container<C>
    void addUpdateableItem(C& existingItems,
                           C& finalItems,
                           C::mapped_type newItem)
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
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlatformDoc(Install* const parent, const QString& docPath, QString docName, const UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsGame(QUuid gameId) const = 0;
    virtual bool containsAddApp(QUuid addAppId) const = 0;

    /* NOTE: The image paths provided here can be null (i.e. images unavailable). Handle accordingly in derived.
     * Also in most cases, addSet should call parent()->processDirectGameImages().
     *
     * TODO: The back and forth here between this and derived documents is a little silly. It mostly exists
     * so that BasicPlatformDoc can call processDirectGameImages() directly; since installs need to always
     * implement custom image processing logic anyway, this maybe should be changed so that BasicPlatformDoc
     * has a pure virtual function similar to processDirectGameImages() with derived installs then implementing
     * that if they need to use BasicPlatform doc. This would free Installs that do not use said class from having
     * to re-implement that function at all (even if they have a close equivalent anyway).
     */
    virtual void addSet(const Fp::Set& set, const ImageSources& images) = 0;
};

class PlatformDoc::Reader : public virtual DataDoc::Reader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Reader(DataDoc* targetDoc);
};

class PlatformDoc::Writer : public virtual DataDoc::Writer
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Writer(DataDoc* sourceDoc);
};

class BasicPlatformDoc : public PlatformDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<Game>> mGamesFinal;
    QHash<QUuid, std::shared_ptr<Game>> mGamesExisting;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsFinal;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlatformDoc(Install* const parent, const QString& docPath, QString docName, const UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual std::shared_ptr<Game> prepareGame(const Fp::Game& game, const ImageSources& images) = 0;
    virtual std::shared_ptr<AddApp> prepareAddApp(const Fp::AddApp& game) = 0;

public:
    virtual bool isEmpty() const override;

    const QHash<QUuid, std::shared_ptr<Game>>& finalGames() const;
    const QHash<QUuid, std::shared_ptr<AddApp>>& finalAddApps() const;

    bool containsGame(QUuid gameId) const override; // NOTE: UNUSED
    bool containsAddApp(QUuid addAppId) const override; // NOTE: UNUSED

    void addSet(const Fp::Set& set, const ImageSources& images) override;

    void finalize() override;
};

class BasicPlatformDoc::Reader : public PlatformDoc::Reader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Reader(DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<Game>>& targetDocExistingGames();
    QHash<QUuid, std::shared_ptr<AddApp>>& targetDocExistingAddApps();
};

class BasicPlatformDoc::Writer : public PlatformDoc::Writer
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Writer(DataDoc* sourceDoc);
};

class PlaylistDoc : public UpdateableDoc, public Errorable
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlaylistDoc(Install* const parent, const QString& docPath, QString docName, const UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsPlaylistGame(QUuid gameId) const = 0; // NOTE: UNUSED

    virtual void setPlaylistData(const Fp::Playlist& playlist) = 0;
};

class PlaylistDoc::Reader : public virtual DataDoc::Reader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Reader(DataDoc* targetDoc);
};

class PlaylistDoc::Writer : public virtual DataDoc::Writer
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Writer(DataDoc* sourceDoc);
};

class BasicPlaylistDoc : public PlaylistDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    std::shared_ptr<PlaylistHeader> mPlaylistHeader;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesFinal;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlaylistDoc(Install* const parent, const QString& docPath, QString docName, const UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual std::shared_ptr<PlaylistHeader> preparePlaylistHeader(const Fp::Playlist& playlist) = 0;
    virtual std::shared_ptr<PlaylistGame> preparePlaylistGame(const Fp::PlaylistGame& game) = 0;

public:
    virtual bool isEmpty() const override;

    const std::shared_ptr<PlaylistHeader>& playlistHeader() const;
    const QHash<QUuid, std::shared_ptr<PlaylistGame>>& finalPlaylistGames() const;

    bool containsPlaylistGame(QUuid gameId) const override;

    void setPlaylistData(const Fp::Playlist& playlist) override;

    void finalize() override;
};

/* TODO: Consider making the existing items accessible through a public getter, or at least a function to add
 * them through a public function (similar todo already exists). If this is done then these base readers and writers
 * for specific docs can be removed since they only exist to define the "workaround" getters for existing items.
 *
 * This would mean that virtual inheritance wouldn't be required for the other readers/writers and greatly simplify
 * things
 */
class BasicPlaylistDoc::Reader : public PlaylistDoc::Reader
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Reader(DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<PlaylistGame>>& targetDocExistingPlaylistGames();
    std::shared_ptr<PlaylistHeader>& targetDocPlaylistHeader();
};

class BasicPlaylistDoc::Writer : public PlaylistDoc::Writer
{
//-Constructor-------------------------------------------------------------------------------------------------------
protected:
    Writer(DataDoc* sourceDoc);
};

}
#endif // FE_DATA
