#ifndef FE_ITEMS_H
#define FE_ITEMS_H

// Standard Library Includes
#include <concepts>

// Qt Includes
#include <QHash>
#include <QUuid>

namespace Fe
{

/*
 * TODO: Right now there is no Fe::Set or similar. This would be nice because it would set a standard for Platform
 * docs that store their add apps directly with their main games; however, with the current system this would make
 * the 'containsXXX' methods hairy and slow unless they're outright removed (which they can be, they're unused), and
 * the update situation is even rougher. The set couldn't really derive from Item because its not an item by itself,
 * and even if it was the other fields that would get transferred wouldn't actually be from the game/add apps of the set,
 * but just empty ones along side them. Likely an extra function would have to be added to UpdateableDoc like
 * "addUpdateableSet", and finalize be modified as well. Lastly there is the question of whether or not to have a fixed
 * set that just contains pointers to Fe::Game and a list of pointers to Fe::AddApp, or intended for the set to
 * be derived from as well. A fixed set is likely the better choice since any even remotely compatible frontend should
 * be able to work with it
 */

//-Class Forward Declarations---------------------------------------------------------------------------------------
//class Item;
//template <typename B, typename T, QX_ENABLE_IF(std::is_base_of<Item, T>)> class ItemBuilder;

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Item
{
    template <typename B, typename T>
        requires std::derived_from<T, Item>
    friend class ItemBuilder;
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QHash<QString, QString> mOtherFields;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Item();

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Item();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QHash<QString, QString>& otherFields();
    const QHash<QString, QString>& otherFields() const;
    void transferOtherFields(QHash<QString, QString>& otherFields);
};

template <typename B, typename T>
    requires std::derived_from<T, Item>
class ItemBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
protected:
    T mItemBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    ItemBuilder() {}

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~ItemBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wOtherField(QPair<QString, QString> otherField)
    {
        mItemBlueprint.mOtherFields[otherField.first] = otherField.second;
        return static_cast<B&>(*this);
    }
    T build() { return mItemBlueprint; }
    std::shared_ptr<T> buildShared() { return std::make_shared<T>(mItemBlueprint); }
};

class BasicItem : public Item
{
    template <typename B, typename T>
        requires std::derived_from<T, BasicItem>
    friend class BasicItemBuilder;
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

template <typename B, typename T>
    requires std::derived_from<T, BasicItem>
class BasicItemBuilder : public ItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wId(QString rawId) { ItemBuilder<B,T>::mItemBlueprint.mId = QUuid(rawId); return static_cast<B&>(*this); }
    B& wId(QUuid id) { ItemBuilder<B,T>::mItemBlueprint.mId = id; return static_cast<B&>(*this); }
    B& wName(QString name) { ItemBuilder<B,T>::mItemBlueprint.mName = name; return static_cast<B&>(*this);}
};

class Game : public BasicItem
{
    template <typename B, typename T>
        requires std::derived_from<T, Game>
    friend class GameBuilder;

//-Class Variables--------------------------------------------------------------------------------------------------
protected:
    static inline const QString RELEASE_TYPE_GAME = "Game";
    static inline const QString RELEASE_TYPE_ANIM = "Animation";

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

template <typename B, typename T>
    requires std::derived_from<T, Game>
class GameBuilder : public BasicItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wPlatform(QString platform) { ItemBuilder<B,T>::mItemBlueprint.mPlatform = platform; return static_cast<B&>(*this); }
};

class AddApp : public BasicItem
{
    template <typename B, typename T>
        requires std::derived_from<T, AddApp>
    friend class AddAppBuilder;
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

template <typename B, typename T>
    requires std::derived_from<T, AddApp>
class AddAppBuilder : public BasicItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wGameId(QString rawGameId) { ItemBuilder<B,T>::mItemBlueprint.mGameId = QUuid(rawGameId); return static_cast<B&>(*this); }
    B& wGameId(QUuid gameId) { ItemBuilder<B,T>::mItemBlueprint.mGameId = gameId; return *this; }
};

class PlaylistHeader : public BasicItem
{
    template <typename B, typename T>
        requires std::derived_from<T, PlaylistHeader>
    friend class PlaylistHeaderBuilder;
//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistHeader();
    PlaylistHeader(QUuid id, QString name);
};

template <typename B, typename T>
    requires std::derived_from<T, PlaylistHeader>
class PlaylistHeaderBuilder : public BasicItemBuilder<B, T>
{};

class PlaylistGame : public BasicItem
{
    template <typename B, typename T>
        requires std::derived_from<T, PlaylistGame>
    friend class PlaylistGameBuilder;
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

template <typename B, typename T>
    requires std::derived_from<T, PlaylistGame>
class PlaylistGameBuilder : public BasicItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    // These reuse the main ID on purpose, in this case gameId is a proxy for Id
    B& wGameId(QString rawGameId) { ItemBuilder<B,T>::mItemBlueprint.mId = QUuid(rawGameId); return static_cast<B&>(*this); }
    B& wGameId(QUuid gameId) { ItemBuilder<B,T>::mItemBlueprint.mId = gameId; return static_cast<B&>(*this); }
};

}

#endif // FE_ITEMS_H
