#ifndef FE_ITEMS_H
#define FE_ITEMS_H

#include "qx.h"

namespace Fe
{

//-Class Forward Declarations---------------------------------------------------------------------------------------
//class Item;
//template <typename B, typename T, ENABLE_IF(std::is_base_of<Item, T>)> class ItemBuilder;

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Item
{
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<Item, T>)>
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

template <typename B, typename T, ENABLE_IF(std::is_base_of<Item, T>)>
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
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<BasicItem, T>)>
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

template <typename B, typename T, ENABLE_IF(std::is_base_of<BasicItem, T>)>
class BasicItemBuilder : public ItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wId(QString rawId) { ItemBuilder<B,T>::mItemBlueprint.mId = QUuid(rawId); return *this; }
    B& wId(QUuid id) { ItemBuilder<B,T>::mItemBlueprint.mId = id; return *this; }
    B& wName(QString name) { ItemBuilder<B,T>::mItemBlueprint.mName = name; return *this;}
};

class Game : public BasicItem
{
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<Game, T>)>
    friend class GameBuilder;
//-Class Variables--------------------------------------------------------------------------------------------------
protected:
    // TODO: See if these are needed
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

template <typename B, typename T, ENABLE_IF(std::is_base_of<Game, T>)>
class GameBuilder : public BasicItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wPlatform(QString platform) { ItemBuilder<B,T>::mItemBlueprint.mPlatform = platform; return *this; }
};

class AddApp : public BasicItem
{
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<AddApp, T>)>
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
    QUuid getGameID() const;
};

template <typename B, typename T, ENABLE_IF(std::is_base_of<AddApp, T>)>
class AddAppBuilder : public BasicItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wGameId(QString rawGameId) { ItemBuilder<B,T>::mItemBlueprint.mGameId = QUuid(rawGameId); return *this; }
    B& wGameId(QUuid gameId) { ItemBuilder<B,T>::mItemBlueprint.mGameId = gameId; return *this; }
};

class PlaylistHeader : public BasicItem
{
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<PlaylistHeader, T>)>
    friend class PlaylistHeaderBuilder;
//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistHeader();
    PlaylistHeader(QUuid id, QString name);
};

template <typename B, typename T, ENABLE_IF(std::is_base_of<PlaylistHeader, T>)>
class PlaylistHeaderBuilder : public BasicItemBuilder<B, T>
{};

class PlaylistGame : public BasicItem
{
    template <typename B, typename T, ENABLE_IF2(std::is_base_of<PlaylistGame, T>)>
    friend class PlaylistGameBuilder;
//-Instance Variables-----------------------------------------------------------------------------------------------
protected:

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistGame();
    PlaylistGame(QUuid id, QString name);

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QUuid getGameID() const;
};

template <typename B, typename T, ENABLE_IF(std::is_base_of<PlaylistGame, T>)>
class PlaylistGameBuilder : public BasicItemBuilder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    // These reuse the main ID on purpose, in this case gameId is a proxy for Id
    B& wGameId(QString rawGameId) { ItemBuilder<B,T>::mItemBlueprint.mId = QUuid(rawGameId); return *this; }
    B& wGameId(QUuid gameId) { ItemBuilder<B,T>::mItemBlueprint.mId = gameId; return *this; }
};

}

#endif // FE_ITEMS_H
