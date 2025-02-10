#ifndef LR_DATA_INTERFACE_H
#define LR_DATA_INTERFACE_H

// Standard Library Includes
#include <concepts>
#include <memory>

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
private:
    IInstall* mInstall;
    const QString mDocumentPath;
    const QString mName;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    IDataDoc(IInstall* install, const QString& docPath, QString docName);

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

class IUpdateableDoc : public IDataDoc
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Import::UpdateOptions mUpdateOptions;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit IUpdateableDoc(IInstall* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Class Functions-----------------------------------------------------------------------------------------------------
template<typename T>
static T* itemPtr(T& item) { return &item; }

template<typename T>
static T* itemPtr(std::shared_ptr<T> item) { return item.get(); }

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
                           typename C::key_type key,
                           typename C::mapped_type newItem)
    {
        // Check if item exists
        if(existingItems.contains(key))
        {
            // Replace if existing update is on, move existing otherwise
            if(mUpdateOptions.importMode == Import::UpdateMode::NewAndExisting)
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
                           typename C::mapped_type newItem)
    {
        addUpdateableItem(existingItems,
                          finalItems,
                          std::static_pointer_cast<BasicItem>(newItem)->id(),
                          newItem);
    }

public:
    virtual void finalize();
};

class IPlatformDoc : public IUpdateableDoc
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit IPlatformDoc(IInstall* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsGame(QUuid gameId) const = 0; // NOTE: UNUSED
    virtual bool containsAddApp(QUuid addAppId) const = 0; // NOTE: UNUSED
    virtual void addSet(const Fp::Set& set, Import::ImagePaths& images) = 0;
};

class IPlaylistDoc : public IUpdateableDoc
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit IPlaylistDoc(IInstall* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const override;

public:
    virtual bool containsPlaylistGame(QUuid gameId) const = 0; // NOTE: UNUSED
    virtual void setPlaylistData(const Fp::Playlist& playlist) = 0;
};

}
#endif // LR_DATA_INTERFACE_H
