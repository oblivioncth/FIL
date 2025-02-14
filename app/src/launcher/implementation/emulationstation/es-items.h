#ifndef EMULATIONSTATION_ITEMS_H
#define EMULATIONSTATION_ITEMS_H

// libfp Includes
#include <fp/fp-items.h>

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

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Game();
    Game(const Fp::Game& flashpointGame);
    Game(const Fp::AddApp& flashpointAddApp, const Fp::Game& parentGame);

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static QString pathFromId(const QUuid& id);
    static QUuid idFromPath(const QString& path);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString path() const;

};

class Game::Builder : public Lr::Game::Builder<Game>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wPath(const QString& path);
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

class System : public Lr::Item
{
    // NOTE: We don't use this, it's just a formal way of ensuring existing entries are maintained
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mName;
    QString mFullName;
    QString mSystemSortName;
    QHash<QString, QString> mCommands;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    System();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
    QString fullName() const;
    QString systemSortName() const;
    const QHash<QString, QString>& commands() const;
};

class System::Builder : public Lr::Item::Builder<System>
{
//-Constructor------------- ------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& name);
    Builder& wFullName(const QString& fullName);
    Builder& wSystemSortName(const QString& sortName);
    Builder& wCommand(const QString& label, const QString& command);
};

}

#endif // EMULATIONSTATION_ITEMS_H
