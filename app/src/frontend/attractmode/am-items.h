#ifndef ATTRACTMODE_ITEMS_H
#define ATTRACTMODE_ITEMS_H

// Qt Includes
#include <QString>
#include <QDateTime>

// libfp Includes
#include <fp/fp-items.h>

// Project Includes
#include "frontend/fe-items.h"

namespace Am
{

class RomEntry : public Fe::Game
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    //mName - Handled as alias for Fe::Game::mId
    //mTitle - Handled as alias for Fe::Game::mName
    QString mEmulator;
    QString mCloneOf;
    QDate mYear;
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

//-Class Functions-----------------------------------------------------------------------------------------------
private:
    static QString addAppTitle(const QString& parentTitle, const QString& originalAddAppTitle);
    static QString addAppSortTitle(const QString& parentTitle, const QString& originalAddAppTitle);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
     QUuid name() const; // Alias for Fe::Game::Id
     QString title() const; // Alias for Fe::Game::name
     QString emulator() const;
     QString cloneOf() const;
     QDate year() const;
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

class RomEntry::Builder : public Fe::Game::Builder<RomEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& nameAsId);
    Builder& wTitle(const QString& title);
    Builder& wEmulator(const QString& emulator);
    Builder& wCloneOf(const QString& cloneOf);
    Builder& wYear(const QString& rawYear);
    Builder& wManufacturer(const QString& manufacturer);
    Builder& wCategory(const QString& category);
    Builder& wPlayers(const QString& players);
    Builder& wRotation(const QString& rawRotation);
    Builder& wControl(const QString& control);
    Builder& wStatus(const QString& status);
    Builder& wDisplayCount(const QString& rawDisplayCount);
    Builder& wDisplayType(const QString& displayType);
    Builder& wAltRomName(const QString& altRomName);
    Builder& wAltTitle(const QString& altTitle);
    Builder& wExtra(const QString& extra);
    Builder& wButtons(const QString& buttons);
    Builder& wSeries(const QString& series);
    Builder& wLanguage(const QString& language);
    Builder& wRegion(const QString& region);
    Builder& wRating(const QString& rating);
};

class EmulatorArtworkEntry : public Fe::Item
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;

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

class EmulatorArtworkEntry::Builder : public Fe::Item::Builder<EmulatorArtworkEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wType(const QString& type);
    Builder& wPaths(const QStringList& paths);
};

}
#endif // ATTRACTMODE_ITEMS_H
