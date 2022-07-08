#ifndef FE_ITEMS_H
#define FE_ITEMS_H

// Standard Library Includes
#include <concepts>

// Qt Includes
#include <QHash>
#include <QUuid>

namespace Fe
{

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

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;
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
    QUuid getId() const;
    QString getName() const;
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
    QString getPlatform() const;
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
    QUuid getGameId() const;
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
    QUuid getGameId() const;
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
