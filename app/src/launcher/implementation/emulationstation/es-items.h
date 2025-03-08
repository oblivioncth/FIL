#ifndef EMULATIONSTATION_ITEMS_H
#define EMULATIONSTATION_ITEMS_H

// libfp Includes
#include <fp/fp-items.h>

// Qt Includes
#include <QMetaType>

// Project Includes
#include "launcher/interface/lr-items-interface.h"

namespace Es
{

class Game : public Lr::Game
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mPath;
    QString mSortName;
    QString mCollectionSortName;
    QString mDesc;
    QDateTime mReleaseDate;
    QString mDeveloper;
    QString mPublisher;
    QString mGenre;
    QString mPlayers;
    bool mKidGame;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Game();
    Game(const Fp::Game& fpGame, const Fp::GameTags& fpTags, const QString& systemName);
    Game(const Fp::AddApp& fpAddApp, const Fp::Game& parentGame, const Fp::GameTags& parentGameTags, const QString& systemName);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static QString addAppTitle(const QString& parentTitle, const QString& originalAddAppTitle);
    static QString addAppSortTitle(const QString& parentTitle, const QString& originalAddAppTitle);

public:
    static QString filenameFromId(const QUuid& id);
    static QUuid idFromFilename(const QString& filename);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString path() const;
    QString sortName() const;
    QString collectionSortName() const;
    QString desc() const;
    QDateTime releaseDate() const;
    QString developer() const;
    QString publisher() const;
    QString genre() const;
    QString players() const;
    bool kidGame() const;
    QString systemName() const; // Alias for Lr::Game::Platform
};

class Game::Builder : public Lr::Game::Builder<Game>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wPath(const QString& path);
    Builder& wSortName(const QString& sortName);
    Builder& wCollectionSortName(const QString& collectionSortName);
    Builder& wDesc(const QString& desc);
    Builder& wReleaseDate(const QString& rawReleaseDate);
    Builder& wDeveloper(const QString& developer);
    Builder& wPublisher(const QString& publisher);
    Builder& wGenre(const QString& genre);
    Builder& wPlayers(const QString& players);
    Builder& wKidGame(const QString& rawKidGame);
};

class Folder : public Lr::Item
{
    // NOTE: We don't use this, it's just a formal way of ensuring existing entries are maintained
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mPath;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Folder();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString path() const;

};

class Folder::Builder : public Lr::Item::Builder<Folder>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wPath(const QString& path);
};

class CollectionEntry : public Lr::BasicItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mSystemName;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    CollectionEntry();
    CollectionEntry(const QUuid& gameId, const QString& systemName);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString systemName() const;
};

class CollectionEntry::Builder : public Lr::BasicItem::Builder<CollectionEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wSystemName(const QString& name);
};

class System : public Lr::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    static inline const QString NAME_PREFIX = u"fp"_s;
    static inline const QString FULL_NAME_PREFIX = u"[FP]"_s;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mName;
    QString mFullName;
    QString mSystemSortName;
    QString mPath;
    QString mExtension;
    QHash<QString, QString> mCommands;
    QString mPlatform;
    QString mTheme;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    System();

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    /* TODO: The whole short vs full system name thing for this launcher is a bit of a mess,
     * as those different version of the name are needed in various different places which
     * results in a lot of conversions and copying around data. For this system it would
     * almost be better if translateDocName() wasn't a requirement and it could just always
     * work with the original so it could convert it to the right version on a case-by-case
     * basis, so reworking that system might be beneficial; though, it's mainly required for
     * doc presence detection and checkout management using private doc sets so it would be tricky.
     */
    static QString originalNameToFullName(const QString& original);
    static QString fullNameToName(const QString& full);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
    QString fullName() const;
    QString systemSortName() const;
    QString path() const;
    QString extension() const;
    const QHash<QString, QString>& commands() const;
    QString platform() const;
    QString theme() const;
};

class System::Builder : public Lr::Item::Builder<System>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& name);
    Builder& wFullName(const QString& fullName);
    Builder& wSystemSortName(const QString& sortName);
    Builder& wPath(const QString& path);
    Builder& wExtension(const QString& extension);
    Builder& wCommand(const QString& label, const QString& command);
    Builder& wPlatform(const QString& platform);
    Builder& wTheme(const QString& theme);
};

class Setting
{
//-Class Variables---------------------------------------------------------------------------------------------
private:
    static inline constexpr QMetaType QSTRING_TYPE = QMetaType::fromType<QString>();

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QMetaType mType;
    QString mName;
    QString mValue;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Setting();
    Setting(const QString& type, const QString& name, const QString& value);
    Setting(const QMetaType& type, const QString& name, const QString& value);

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    template<typename T>
    static Setting make(const QString& name, const QString& value) {return Setting(QMetaType::fromType<T>(), name, value); }

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString type() const;
    QString name() const;
    QString value() const;
    bool isValid() const;

    template<typename T>
    bool is() const
    {
        auto destType = QMetaType::fromType<T>();
        if(destType != mType)
            return false;

        if(!QMetaType::canConvert(QSTRING_TYPE, destType))
        {
            qWarning("Setting: QString cannot be converted to recorded type %s", destType.name());
            return false;
        }

        return true;
    }

    template<typename T>
    bool get(T& buffer) const
    {
        if(!is<T>())
        {
            qWarning("Setting: Incorrect type for get<T>()! Recorded as '%s', tried '%s'.", mType.name(), QMetaType::fromType<T>().name());
            buffer = T();
            return false;
        }

        QVariant v(mValue);
        if(!v.convert(mType))
        {
            qWarning("Setting: Typecast to '%s' failed for value '%s'!", mType.name(), qPrintable(mValue));
            buffer = T();
            return false;
        }

        buffer = v.value<T>();
        return true;
    }
};



}

#endif // EMULATIONSTATION_ITEMS_H
