#ifndef FE_ITEMS_H
#define FE_ITEMS_H

#include "qx.h"

//#include "flashpoint/fp-items.h"

namespace Fe
{

//-Class Forward Declarations---------------------------------------------------------------------------------------
class Item;
template <typename B, typename T, ENABLE_IF(std::is_base_of<Item, T>)> class ItemBuilder;

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

    QHash<QString, QString>& getOtherFields();
    const QHash<QString, QString>& getOtherFields() const;

//-Instance Functions------------------------------------------------------------------------------------------
public:
    void transferOtherFields(QHash<QString, QString>& otherFields);
};

template <typename B, typename T, ENABLE_IF2(std::is_base_of<Item, T>)>
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
};

class Game : public Item
{
    friend class GameBuilder;
//-Class Variables--------------------------------------------------------------------------------------------------
protected:
    // TODO: See if these are needed
    static inline const QString RELEASE_TYPE_GAME = "Game";
    static inline const QString RELEASE_TYPE_ANIM = "Animation";

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    QUuid mID;
    QString mTitle;
    QString mPlatform;

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getID() const;
    QString getTitle() const;
    QString getPlatform() const;
};

class GameBuilder : public ItemBuilder<GameBuilder, Game>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    GameBuilder& wID(QString rawID);
    GameBuilder& wTitle(QString title);
    GameBuilder& wPlatform(QString platform);
};

class AddApp : public Item
{
    friend class AddAppBuilder;
//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    QUuid mID;
    QUuid mGameID;
    QString mName;

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getID() const;
    QUuid getGameID() const;
    QString getName() const;
};

class AddAppBuilder : public ItemBuilder<AddAppBuilder, AddApp>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    AddAppBuilder& wID(QString rawID);
    AddAppBuilder& wGameID(QString rawGameID);
    AddAppBuilder& wName(QString name);
};

class PlaylistHeader : public Item
{
    friend class PlaylistHeaderBuilder;
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mPlaylistID;
    QString mName;

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getPlaylistID() const;
    QString getName() const;
};

class PlaylistHeaderBuilder : public ItemBuilder<PlaylistHeaderBuilder, PlaylistHeader>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistHeaderBuilder& wPlaylistID(QString rawPlaylistID);
    PlaylistHeaderBuilder& wName(QString name);
};

class PlaylistGame : public Item
{
    friend class PlaylistGameBuilder;
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mGameID;

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getGameID() const;
};

class PlaylistGameBuilder : public ItemBuilder<PlaylistGameBuilder, PlaylistGame>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder& wGameID(QString rawGameID);
};

}

#endif // FE_ITEMS_H
