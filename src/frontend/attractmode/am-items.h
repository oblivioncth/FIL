#ifndef ATTRACTMODE_ITEMS_H
#define ATTRACTMODE_ITEMS_H

// Qt Includes
#include <QString>
#include <QDateTime>

// libfp Includes
#include <fp/flashpoint/fp-items.h>

// Project Includes
#include "../fe-items.h"

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

class RomEntry::Builder : public Fe::Game::Builder<RomEntry::Builder, RomEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(QString nameAsId);
    Builder& wTitle(QString title);
    Builder& wEmulator(QString emulator);
    Builder& wCloneOf(QString cloneOf);
    Builder& wYear(QString rawYear);
    Builder& wManufacturer(QString manufacturer);
    Builder& wCategory(QString category);
    Builder& wPlayers(QString players);
    Builder& wRotation(QString rawRotation);
    Builder& wControl(QString control);
    Builder& wStatus(QString status);
    Builder& wDisplayCount(QString rawDisplayCount);
    Builder& wDisplayType(QString displayType);
    Builder& wAltRomName(QString altRomName);
    Builder& wAltTitle(QString altTitle);
    Builder& wExtra(QString extra);
    Builder& wButtons(QString buttons);
    Builder& wSeries(QString series);
    Builder& wLanguage(QString language);
    Builder& wRegion(QString region);
    Builder& wRating(QString rating);
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

class EmulatorArtworkEntry::Builder : public Fe::Item::Builder<EmulatorArtworkEntry::Builder, EmulatorArtworkEntry>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wType(QString type);
    Builder& wPaths(QStringList paths);
};

}
#endif // ATTRACTMODE_ITEMS_H
