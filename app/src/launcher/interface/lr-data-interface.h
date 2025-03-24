#ifndef LR_DATA_INTERFACE_H
#define LR_DATA_INTERFACE_H

// Standard Library Includes
#include <concepts>
#include <unordered_set>

// Qt Includes
#include <QFile>
#include <QXmlStreamReader>

// Qx Includes
#include <qx/core/qx-error.h>
#include <qx/core/qx-abstracterror.h>
#include <qx/utility/qx-macros.h>

// libfp Includes
#include <fp/fp-items.h>

// Project Includes
#include "launcher/interface/lr-items-interface.h"
#include "import/settings.h"

namespace Lr
{
class IInstall;
class IDataDoc;

//-Concepts------------------------------------------------------------------------------------------------------

template<class K>
concept updateable_item_container = Qx::qassociative<K> && item<typename K::mapped_type>;

template<class K>
concept updateable_basicitem_container = Qx::qassociative<K> && basic_item<typename K::mapped_type> &&
                                         std::same_as<typename K::key_type, QUuid>;
template<class K>
concept updateable_data_set = Qx::specializes<K, QSet>;

//-Classes-----------------------------------------------------------------------------------------------------------
class QX_ERROR_TYPE(DocHandlingError, "Lr::DocHandlingError", 1310)
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
    DocHandlingError(const IDataDoc& doc, Type t, const QString& s = {});

//-Class Functions-------------------------------------------------------------
private:
    static QString generatePrimaryString(const IDataDoc& doc, Type t);

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

class IDataDoc
{
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
        friend size_t qHash(const Identifier& key, size_t seed) noexcept;

    private:
        Type mDocType;
        QString mDocName;

    public:
        Identifier(Type docType, const QString& docName);

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
private:
    IInstall* mInstall;
    const QString mDocumentPath;
    const QString mName;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    IDataDoc(IInstall* install, const QString& docPath, const QString& docName);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~IDataDoc() = default;

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual Type type() const = 0;

public:
    IInstall* install() const;
    QString path() const;
    Identifier identifier() const;
    virtual bool isEmpty() const = 0;
    virtual void postCheckout(); // Nothing by default
    virtual void preCommit(); // Nothing by default
};
QX_SCOPED_ENUM_HASH_FUNC(IDataDoc::Type);

class IDataDoc::Reader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    IDataDoc* mTargetDocument;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Reader(IDataDoc* targetDoc);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Reader();

//-Instance Functions------------------------------------------------------------------------------------------------
public:
    IDataDoc* target() const;
    virtual DocHandlingError readInto() = 0;
};

class IDataDoc::Writer
{
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    IDataDoc* mSourceDocument;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Writer(IDataDoc* sourceDoc);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Writer();

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    IDataDoc* source() const;
    virtual DocHandlingError writeOutOf() = 0;
};

// class IErrorable
// {
// //-Instance Variables--------------------------------------------------------------------------------------------------
// protected:
//     Qx::Error mError;

// //-Constructor--------------------------------------------------------------------------------------------------------
// protected:
//     IErrorable();

// //-Destructor-------------------------------------------------------------------------------------------------
// public:
//     virtual ~IErrorable();

// //-Instance Functions--------------------------------------------------------------------------------------------------
// public:
//     bool hasError() const;
//     Qx::Error error() const;
// };

class IUpdatableDoc : public IDataDoc
{
//-Inner Classes--------------------------------------------------------------------------------------------------
protected:
    template<typename Data>
    class UpdatableContainer;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    bool mUpdating;

protected:
    Import::UpdateOptions mUpdateOptions;


//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit IUpdatableDoc(IInstall* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    virtual void postCheckout() override;
};

// These concepts help the following container
template<typename C>
concept HasHash = requires (typename C::Hash hasher, const C& key) {
    { hasher(key) } -> std::convertible_to<std::size_t>;
    // Only checks for main hash, but can have more for heterogeneous search
};

template<typename C>
concept HasKeyEqual = requires (typename C::KeyEqual comparator, const C& a, const C& b) {
    { comparator(a, b) } -> std::convertible_to<bool>;
    // Only checks for main compare, but can have more for heterogeneous search
};

template<typename C>
concept CustomizedUpdateableContainer = HasHash<C> && HasKeyEqual<C>;

/* NOTE: This specifically is a container that's useful for handling item/data updates for use within a data doc.
 *
 * To use types with this class, there must exist an equality operator and std::hash specialization for Data;
 * alternative, public inner structs/classes Equals and Compare can be added to a type in order to further
 * customize lookup behavior, such as enabling heterogeneous lookup with other types (see BasicItem) for an
 * example, or just provide hashing/equality check facilities in a uniform place.
 *
 * Items derived from BasicItem are supported by default and use their id() for comparison/hashing
 *
 */

template<typename Data>
class IUpdatableDoc::UpdatableContainer
{
    template<typename D>
    struct storage{ using type = std::unordered_set<D>; };

    template<CustomizedUpdateableContainer D>
    struct storage<D>{ using type = std::unordered_set<D, typename D::Hash, typename D::KeyEqual>; };

    using Storage = storage<Data>::type;
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    const IUpdatableDoc* mDoc;
    Storage mExisting; // Holds obsolete entries after updates are complete if 'remove obsolete entries' is enabled
    Storage mUpdated;
    Storage mNew;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit UpdatableContainer(const IUpdatableDoc* doc) : mDoc(doc) {}

//-Class Functions--------------------------------------------------------------------------------------------------
private:
    template<typename F>
    static auto forEach(F&& f, const Storage& s)
    {
        for(const auto& e : s)
        {
            if constexpr(std::predicate<F, Data>)
            {
                if(!f(e))
                    return false;
            }
            else
                f(e);
        }

        if constexpr(std::predicate<F, Data>)
            return true;
    }

    template<typename F, typename... Stores>
    static auto allForEach(F&& f, Stores&&... stores)
    {
        if constexpr(std::predicate<F, Data>)
            return (forEach(f, stores) && ...);
        else
            ((forEach(f, stores)), ...);
    }

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    template<typename T>
    const Data* insertInit(T&& d) { return &(*mExisting.insert(std::forward<T>(d)).first); }

    template<typename T>
    const Data* insertMod(T&& d)
    {
        // This could benefit from move semantics if QSet ever adds them, otherwise could use std::unordered_set
        auto ex = mExisting.find(d);
        if(ex == mExisting.end())
            return &(*mNew.insert(std::forward<T>(d)).first);
        else
        {
            const Data* inserted;

            // Replace if NewAndExisting, otherwise retain exact original
            if(mDoc->mUpdateOptions.importMode == Import::UpdateMode::NewAndExisting)
            {
                // Special case for Items (copy other fields)
                if constexpr(item<Data>)
                    d.copyOtherFields(*ex);

                inserted = &(*mUpdated.insert(std::forward<T>(d)).first);
            }
            else
                inserted = &(*mUpdated.insert(*ex).first);

            mExisting.erase(ex);

            return inserted;
        }
    }

public:
    template<typename T>
        requires std::same_as<std::remove_cvref_t<T>, Data>
    const Data* insert(T&& d)
    {
        if(!mDoc->mUpdating)
            return insertInit(std::forward<T>(d));
        else
            return insertMod(std::forward<T>(d));
    }

    /* For posterity, even though the following function should likely never be used during the initial read,
     * it would be more proper if they returned different results depending on mDoc->mUpdating (e.g.
     * containsObsolete() would always return false when not in the updating phase).
     *
     * The templated functions essentially take 'Data' arguments, but T may be allowed to be more if Data defines
     * as Hash and KeyEqual class that enable heterogeneous comparison.
     */

    template<typename T>
    bool containsExisting(T&& t) const { return mExisting.contains(std::forward<T>(t)); }

    template<typename T>
    bool containsObsolete(T&& t) const { return mDoc->mUpdateOptions.removeObsolete && mExisting.contains(std::forward<T>(t)); }

    template<typename T>
    bool containsNew(T&& t) const { return mNew.contains(std::forward<T>(t)); }

    template<typename T>
    bool containsFinal(T&& t) const
    {
        return (!mDoc->mUpdateOptions.removeObsolete && mExisting.contains(std::forward<T>(t))) ||
               mUpdated.contains(std::forward<T>(t)) ||
               mNew.contains(std::forward<T>(t));
    }

    template<typename T>
    bool contains(T&& t) const { return mExisting.contains(std::forward<T>(t)) || mUpdated.contains(std::forward<T>(t)) || mNew.contains(std::forward<T>(t)); }

    template<typename T>
    const Data* findExisting(T&& t) const
    {
        auto itr = mExisting.find(std::forward<T>(t));
        return itr != mExisting.end() ? &(*itr) : nullptr;
    }

    template<typename T>
    const Data* findObsolete(T&& t) const
    {
        auto itr = mDoc->mUpdateOptions.removeObsolete ? mExisting.find(std::forward<T>(t)) : mExisting.end();
        return itr != mExisting.end() ? &(*itr) : nullptr;
    }

    template<typename T>
    const Data* findNew(T&& t) const
    {
        auto itr = mNew.find(std::forward<T>(t));
        return itr != mNew.end() ? &(*itr) : nullptr;
    }

    template<typename T>
    const Data* findFinal(T&& t) const
    {
        auto itr = !mDoc->mUpdateOptions.removeObsolete ? mExisting.find(std::forward<T>(t)) : mExisting.end();
        if(itr != mExisting.end())
            return &(*itr);

        itr = mUpdated.find(std::forward<T>(t));
        if(itr != mExisting.end())
            return &(*itr);

        itr = mNew.find(std::forward<T>(t));
        return itr != mExisting.end() ? &(*itr) : nullptr;
    }

    template<typename T>
    const Data* find(T&& t) const
    {
        auto itr = mExisting.find(std::forward<T>(t));
        if(itr != mExisting.end())
            return &(*itr);

        itr = mUpdated.find(std::forward<T>(t));
        if(itr != mExisting.end())
            return &(*itr);

        itr = mNew.find(std::forward<T>(t));
        return itr != mExisting.end() ? &(*itr) : nullptr;
    }

    template<typename T>
    bool removeExisting(T&& t) { return mExisting.erase(std::forward<T>(t)); }

    template<typename T>
    bool removeObsolete(T&& t) { return mDoc->mUpdateOptions.removeObsolete && mExisting.erase(std::forward<T>(t)); }

    template<typename T>
    bool removeNew(T&& t) { return mNew.erase(std::forward<T>(t)); }

    template<typename T>
    bool removeFinal(T&& t)
    {
        return (!mDoc->mUpdateOptions.removeObsolete && mExisting.erase(std::forward<T>(t))) ||
               mUpdated.erase(std::forward<T>(t)) ||
               mNew.erase(std::forward<T>(t));
    }

    template<typename T>
    bool remove(T&& t) { return mExisting.erase(std::forward<T>(t)) || mUpdated.erase(std::forward<T>(t)) || mNew.erase(std::forward<T>(t)); }

    /* TODO: For now we use a return of auto to allow halting early if the functor is a predicate  but in the long run it would be more flexible to create
     * an iterator type for each category (obsolete, new, final, etc.), but of course that would be annoying. Could try to have some kind of base class that handles
     * most of the tasks with just an override for increment/decrement
     */
    template<typename F>
    auto forEachExisting(F&& f) const
    {
        return allForEach(f, mExisting);
    }

    template<typename F>
    auto forEachObsolete(F&& f) const
    {
        if(mDoc->mUpdateOptions.removeObsolete)
            return allForEach(f, mExisting);
        else if constexpr(std::predicate<F, Data>)
            return true;
    }

    template<typename F>
    auto forEachNew(F&& f) const
    {
        return allForEach(f, mNew);
    }

    template<typename F>
    auto forEachFinal(F&& f) const
    {
        if(!mDoc->mUpdateOptions.removeObsolete)
            return allForEach(f, mExisting, mUpdated, mNew);
        else
            return allForEach(f, mUpdated, mNew);
    }

    template<typename P>
    Storage::size_type eraseObsoleteIf(P&& p)
    {
        if(mDoc->mUpdateOptions.removeObsolete)
            return std::erase_if(mExisting, p);

        return 0;
    }

    template<typename P>
    Storage::size_type eraseNewIf(P&& p) { return std::erase_if(mNew, p); }

    template<typename P>
    Storage::size_type eraseFinalIf(P&& p)
    {
        typename Storage::size_type c = 0;

        if(!mDoc->mUpdateOptions.removeObsolete)
            c += std::erase_if(mExisting, p);

        c += std::erase_if(mUpdated, p);
        c += std::erase_if(mNew, p);

        return c;
    }

    template<typename P>
    Storage::size_type eraseIf(P&& p)
    {
        typename Storage::size_type c = 0;

        c += std::erase_if(mExisting, p);
        c += std::erase_if(mUpdated, p);
        c += std::erase_if(mNew, p);

        return c;
    }

    bool isEmpty() const { return mExisting.empty() && mUpdated.empty() && mNew.empty(); }
};

class IPlatformDoc : public IUpdatableDoc
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit IPlatformDoc(IInstall* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsGame(const QUuid& gameId) const = 0; // NOTE: UNUSED
    virtual bool containsAddApp(const QUuid& addAppId) const = 0; // NOTE: UNUSED
    virtual const Game* addSet(const Fp::Set& set) = 0;
};

class IPlaylistDoc : public IUpdatableDoc
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit IPlaylistDoc(IInstall* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsPlaylistGame(const QUuid& gameId) const = 0; // NOTE: UNUSED
    virtual void setPlaylistData(const Fp::Playlist& playlist) = 0;
};

}
#endif // LR_DATA_INTERFACE_H
