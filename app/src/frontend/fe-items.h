#ifndef FE_ITEMS_H
#define FE_ITEMS_H

// Standard Library Includes
#include <concepts>

// Qt Includes
#include <QHash>
#include <QUuid>

using namespace Qt::Literals::StringLiterals;

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

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename B, typename T>
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

template <typename B, typename T>
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
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename B, typename T>
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

template <typename B, typename T>
    requires std::derived_from<T, BasicItem>
class BasicItem::Builder : public Item::Builder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wId(const QString& rawId) { Item::Builder<B,T>::mItemBlueprint.mId = QUuid(rawId); return static_cast<B&>(*this); }
    B& wId(const QUuid& id) { Item::Builder<B,T>::mItemBlueprint.mId = id; return static_cast<B&>(*this); }
    B& wName(const QString& name) { Item::Builder<B,T>::mItemBlueprint.mName = name; return static_cast<B&>(*this);}
};

class Game : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename B, typename T>
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

template <typename B, typename T>
    requires std::derived_from<T, Game>
class Game::Builder : public BasicItem::Builder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wPlatform(const QString& platform) { Item::Builder<B,T>::mItemBlueprint.mPlatform = platform; return static_cast<B&>(*this); }
};

class AddApp : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename B, typename T>
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

template <typename B, typename T>
    requires std::derived_from<T, AddApp>
class AddApp::Builder : public BasicItem::Builder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    B& wGameId(const QString& rawGameId) { Item::Builder<B,T>::mItemBlueprint.mGameId = QUuid(rawGameId); return static_cast<B&>(*this); }
    B& wGameId(const QUuid& gameId) { Item::Builder<B,T>::mItemBlueprint.mGameId = gameId; return *this; }
};

class PlaylistHeader : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename B, typename T>
        requires std::derived_from<T, PlaylistHeader>
    class Builder;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    PlaylistHeader();
    PlaylistHeader(QUuid id, QString name);
};

template <typename B, typename T>
    requires std::derived_from<T, PlaylistHeader>
class PlaylistHeader::Builder : public BasicItem::Builder<B, T>
{};

class PlaylistGame : public BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    template <typename B, typename T>
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

template <typename B, typename T>
    requires std::derived_from<T, PlaylistGame>
class PlaylistGame::Builder : public BasicItem::Builder<B, T>
{
//-Instance Functions------------------------------------------------------------------------------------------
public:
    // These reuse the main ID on purpose, in this case gameId is a proxy for Id
    B& wGameId(QString rawGameId) { Item::Builder<B,T>::mItemBlueprint.mId = QUuid(rawGameId); return static_cast<B&>(*this); }
    B& wGameId(QUuid gameId) { Item::Builder<B,T>::mItemBlueprint.mId = gameId; return static_cast<B&>(*this); }
};

}

#endif // FE_ITEMS_H
