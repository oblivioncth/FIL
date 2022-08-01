#ifndef ATTRACTMODE_ITEMS_H
#define ATTRACTMODE_ITEMS_H

// Qt Includes
#include <QString>
#include <QDateTime>

// libfp Includes
#include <fp/flashpoint/fp-items.h>

// magic_enum Includes
#include "magic_enum.hpp"

// Project Includes
#include "../fe-items.h"

namespace Am
{

class RomEntry : public Fe::Game
{
    friend class RomEntryBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    //mName - Handled as alias for Fe::Game::mId
    //mTitle - Handled as alias for Fe::Game::mName
    QString mEmulator;
    QString mCloneOf;
    QDateTime mYear;
    QString mManufacturer;
    //mCategory - Handled as alias for Fe::Game::mPlatform
    QString mPlayers;
    quint8 mRotation;
    QString mControl;
    QString mStatus;
    quint8 mDisplayCount;
    QString mDisplayType;
    QString mAltRomName;
    QString mAltTitle;
    QString mExtra;
    QString mButtons;
    QString mSeries;
    QString mLanguage;
    QString mRegion;
    QString mRating;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    RomEntry(const Fp::Game& flashpointGame);
    RomEntry(const Fp::AddApp& flashpointAddApp, const Fp::Game& parentGame);
    RomEntry();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
     QUuid name() const; // Alias for Fe::Game::Id
     QString title() const; // Alias for Fe::Game::name
     QString emulator() const;
     QString cloneOf() const;
     QDateTime year() const;
     QString manufacturer() const;
     QString category() const; // Alias for Fe::Game::platform
     QString players() const;
     quint8 rotation() const;
     QString control() const;
     QString status() const;
     quint8 displayCount() const;
     QString displayType() const;
     QString altRomName() const;
     QString altTitle() const;
     QString extra() const;
     QString buttons() const;
     QString series() const;
     QString language() const;
     QString region() const;
     QString rating() const;
};

class RomEntryBuilder : public Fe::GameBuilder<RomEntryBuilder, RomEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    RomEntryBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    RomEntryBuilder& wName(QString nameAsId);
    RomEntryBuilder& wTitle(QString title);
    RomEntryBuilder& wEmulator(QString emulator);
    RomEntryBuilder& wCloneOf(QString cloneOf);
    RomEntryBuilder& wYear(QString rawYear);
    RomEntryBuilder& wManufacturer(QString manufacturer);
    RomEntryBuilder& wCategory(QString category);
    RomEntryBuilder& wPlayers(QString players);
    RomEntryBuilder& wRotation(QString rawRotation);
    RomEntryBuilder& wControl(QString control);
    RomEntryBuilder& wStatus(QString status);
    RomEntryBuilder& wDisplayCount(QString rawDisplayCount);
    RomEntryBuilder& wDisplayType(QString displayType);
    RomEntryBuilder& wAltRomName(QString altRomName);
    RomEntryBuilder& wAltTitle(QString altTitle);
    RomEntryBuilder& wExtra(QString extra);
    RomEntryBuilder& wButtons(QString buttons);
    RomEntryBuilder& wSeries(QString series);
    RomEntryBuilder& wLanguage(QString language);
    RomEntryBuilder& wRegion(QString region);
    RomEntryBuilder& wRating(QString rating);
};

class EmulatorArtworkEntry : public Fe::Item
{
    friend class EmulatorArtworkEntryBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mType;
    QStringList mPaths;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    EmulatorArtworkEntry();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString type() const;
    QStringList paths() const;
};

class EmulatorArtworkEntryBuilder : public Fe::ItemBuilder<EmulatorArtworkEntryBuilder, EmulatorArtworkEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    EmulatorArtworkEntryBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    EmulatorArtworkEntryBuilder& wType(QString type);
    EmulatorArtworkEntryBuilder& wPaths(QStringList paths);
};

}
#endif // ATTRACTMODE_ITEMS_H
