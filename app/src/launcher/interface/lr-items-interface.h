#ifndef LR_ITEMS_INTERFACE_H
#define LR_ITEMS_INTERFACE_H

// Standard Library Includes
#include <concepts>

// Qt Includes
#include <QHash>
#include <QUuid>

// Qx Includes
#include <qx/utility/qx-concepts.h>

using namespace Qt::Literals::StringLiterals;

namespace Lr
{

/*
 * TODO: Right now there is no Lr::Set or similar. This would be nice because it would set a standard for Platform
 * docs that store their add apps directly with their main games; however, with the current system this would make
 * the 'containsXXX' methods hairy and slow unless they're outright removed (which they can be, they're unused), and
 * the update situation is even rougher. The set couldn't really derive from Item because its not an item by itself,
 * and even if it was the other fields that would get transferred wouldn't actually be from the game/add apps of the set,
 * but just empty ones along side them. Likely an extra function would have to be added to UpdatableDoc like
 * "addUpdateableSet", and finalize be modified as well. Lastly there is the question of whether or not to have a fixed
 * set that just contains pointers to Lr::Game and a list of pointers to Lr::AddApp, or intended for the set to
 * be derived from as well. A fixed set is likely the better choice since any even remotely compatible launcher should
 * be able to work with it
 */

class Item;
class BasicItem;
class Game;
class AddApp;
class PlaylistHeader;
class PlaylistGame;

template<typename T>
concept item = std::derived_from<T, Item>;

template<typename T>
concept basic_item = std::derived_from<T, BasicItem>;

template<typename T>
concept game = std::derived_from<T, Game>;

template<typename T>
concept addapp = std::derived_from<T, AddApp>;

template<typename T>
concept playlistheader = std::derived_from<T, PlaylistHeader>;

template<typename T>
concept playlistgame = std::derived_from<T, PlaylistGame>;

//-Namespace Global Classes-----------------------------------------------------------------------------------------

template<typename T>
class Builder
{
//-Instance Variables------------------------------------------------------------------------------------------
protected:
    T mBlueprint;

//-Instance Functions------------------------------------------------------------------------------------------
public:
    T build() const { return mBlueprint; }
};

class Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<item T>
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    QHash<QString, QString> mOtherFields; // TODO: Does this need to be a hash or can it just be a list of pairs (and is that better)?

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Item();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QHash<QString, QString>& otherFields();
    const QHash<QString, QString>& otherFields() const;
    void copyOtherFields(const Item& other);
};

template<item T>
class Item::Builder : public Lr::Builder<T>
{
//-Constructor-------------------------------------------------------------------------------------------------
protected:
    Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wOtherField(this Self&& self, QPair<QString, QString> otherField)
    {
        self.mBlueprint.mOtherFields[otherField.first] = otherField.second;
        return self;
    }
};

class BasicItem : public Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    struct Hash;
    struct KeyEqual;
    template<basic_item T>
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    QUuid mId;
    QString mName;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    BasicItem();
    BasicItem(QUuid id, QString name);

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QUuid id() const;
    QString name() const;
};

struct BasicItem::Hash
{
    using is_transparent = void;
    std::size_t operator()(const BasicItem& bi) const noexcept { return qHash(bi.mId, 0); }
    std::size_t operator()(const QUuid& id) const noexcept { return qHash(id, 0); }
};

struct BasicItem::KeyEqual
{
    using is_transparent = void;
    bool operator()(const BasicItem& a, const BasicItem& b) const noexcept { return a.mId == b.mId; }
    bool operator()(const QUuid& id, const BasicItem& bi) const noexcept { return id == bi.mId; }
};

template<basic_item T>
class BasicItem::Builder : public Item::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wId(this Self&& self, const QString& rawId) { self.mBlueprint.mId = QUuid(rawId); return self; }

    template<class Self>
    auto wId(this Self&& self, const QUuid& id) { self.mBlueprint.mId = id; return self; }

    template<class Self>
    auto wName(this Self&& self, const QString& name) { self.mBlueprint.mName = name; return self;}
};

class Game : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<game T>
    class Builder;

//-Class Variables--------------------------------------------------------------------------------------------------
protected:
    static inline const QString RELEASE_TYPE_GAME = u"Game"_s;
    static inline const QString RELEASE_TYPE_ANIM = u"Animation"_s;

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    QString mPlatform;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    Game();
    Game(QUuid id, QString name, QString platform);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString platform() const;
};

template<game T>
class Game::Builder : public BasicItem::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wPlatform(this Self&& self, const QString& platform) { self.mBlueprint.mPlatform = platform; return self; }
};

class AddApp : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<addapp T>
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    QUuid mGameId;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    AddApp();
    AddApp(QUuid id, QString name, QUuid gameId);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid gameId() const;
};

template<addapp T>
class AddApp::Builder : public BasicItem::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wGameId(this Self&& self, const QString& rawGameId) { self.mBlueprint.mGameId = QUuid(rawGameId); return self; }

    template<class Self>
    auto wGameId(this Self&& self, const QUuid& gameId) { self.mBlueprint.mGameId = gameId; return self; }
};

class PlaylistHeader : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<playlistheader T>
    class Builder;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistHeader();
    PlaylistHeader(QUuid id, QString name);
};

template<playlistheader T>
class PlaylistHeader::Builder : public BasicItem::Builder<T>
{};

class PlaylistGame : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<playlistgame T>
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistGame();
    PlaylistGame(QUuid id, QString name);

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QUuid gameId() const;
};

template<playlistgame T>
class PlaylistGame::Builder : public BasicItem::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    // These reuse the main ID on purpose, in this case gameId is a proxy for Id
    template<class Self>
    auto wGameId(this Self&& self, QString rawGameId) { self.mBlueprint.mId = QUuid(rawGameId); return self; }

    template<class Self>
    auto wGameId(this Self&& self, QUuid gameId) { self.mBlueprint.mId = gameId; return self; }
};

}

#endif // LR_ITEMS_INTERFACE_H
