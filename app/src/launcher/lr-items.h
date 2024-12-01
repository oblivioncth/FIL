#ifndef LR_ITEMS_H
#define LR_ITEMS_H

// Standard Library Includes
#include <concepts>

// Qt Includes
#include <QHash>
#include <QUuid>

using namespace Qt::Literals::StringLiterals;

namespace Lr
{

/*
 * TODO: Right now there is no Lr::Set or similar. This would be nice because it would set a standard for Platform
 * docs that store their add apps directly with their main games; however, with the current system this would make
 * the 'containsXXX' methods hairy and slow unless they're outright removed (which they can be, they're unused), and
 * the update situation is even rougher. The set couldn't really derive from Item because its not an item by itself,
 * and even if it was the other fields that would get transferred wouldn't actually be from the game/add apps of the set,
 * but just empty ones along side them. Likely an extra function would have to be added to UpdateableDoc like
 * "addUpdateableSet", and finalize be modified as well. Lastly there is the question of whether or not to have a fixed
 * set that just contains pointers to Lr::Game and a list of pointers to Lr::AddApp, or intended for the set to
 * be derived from as well. A fixed set is likely the better choice since any even remotely compatible launcher should
 * be able to work with it
 */

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename T>
        requires std::derived_from<T, Item>
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
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

template <typename T>
    requires std::derived_from<T, Item>
class Item::Builder
{
//-Instance Variables------------------------------------------------------------------------------------------
protected:
    T mItemBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    Builder() {}

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wOtherField(this Self&& self, QPair<QString, QString> otherField)
    {
        self.mItemBlueprint.mOtherFields[otherField.first] = otherField.second;
        return self;
    }
    T build() { return mItemBlueprint; }
    std::shared_ptr<T> buildShared() { return std::make_shared<T>(mItemBlueprint); }
};

class BasicItem : public Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<typename T>
        requires std::derived_from<T, BasicItem>
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

template<typename T>
    requires std::derived_from<T, BasicItem>
class BasicItem::Builder : public Item::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wId(this Self&& self, const QString& rawId) { self.mItemBlueprint.mId = QUuid(rawId); return self; }

    template<class Self>
    auto wId(this Self&& self, const QUuid& id) { self.mItemBlueprint.mId = id; return self; }

    template<class Self>
    auto wName(this Self&& self, const QString& name) { self.mItemBlueprint.mName = name; return self;}
};

class Game : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<typename T>
        requires std::derived_from<T, Game>
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

template<typename T>
    requires std::derived_from<T, Game>
class Game::Builder : public BasicItem::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wPlatform(this Self&& self, const QString& platform) { self.mItemBlueprint.mPlatform = platform; return self; }
};

class AddApp : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<typename T>
        requires std::derived_from<T, AddApp>
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

template<typename T>
    requires std::derived_from<T, AddApp>
class AddApp::Builder : public BasicItem::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    template<class Self>
    auto wGameId(this Self&& self, const QString& rawGameId) { self.mItemBlueprint.mGameId = QUuid(rawGameId); return self; }

    template<class Self>
    auto wGameId(this Self&& self, const QUuid& gameId) { self.mItemBlueprint.mGameId = gameId; return self; }
};

class PlaylistHeader : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<typename T>
        requires std::derived_from<T, PlaylistHeader>
    class Builder;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistHeader();
    PlaylistHeader(QUuid id, QString name);
};

template<typename T>
    requires std::derived_from<T, PlaylistHeader>
class PlaylistHeader::Builder : public BasicItem::Builder<T>
{};

class PlaylistGame : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template<typename T>
        requires std::derived_from<T, PlaylistGame>
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

template<typename T>
    requires std::derived_from<T, PlaylistGame>
class PlaylistGame::Builder : public BasicItem::Builder<T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    // These reuse the main ID on purpose, in this case gameId is a proxy for Id
    template<class Self>
    auto wGameId(this Self&& self, QString rawGameId) { self.mItemBlueprint.mId = QUuid(rawGameId); return self; }

    template<class Self>
    auto wGameId(this Self&& self, QUuid gameId) { self.mItemBlueprint.mId = gameId; return self; }
};

}

#endif // LR_ITEMS_H
