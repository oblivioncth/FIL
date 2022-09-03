// Unit Include
#include "am-items.h"

// Project Includes
#include "../../clifp.h"

// Quote escape
#define ESCAPE(str) (str).replace(R"(")",R"(\")")

namespace Am
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
RomEntry::RomEntry() {}

RomEntry::RomEntry(const Fp::Game& flashpointGame) :
    Fe::Game(flashpointGame.id(), ESCAPE(flashpointGame.title()), flashpointGame.platform()),
    mEmulator(Fp::NAME),
    mCloneOf(),
    mYear(flashpointGame.releaseDate().date()),
    mManufacturer(ESCAPE(flashpointGame.developer())),
    mPlayers(flashpointGame.playMode()),
    mRotation(0),
    mControl(),
    mStatus(flashpointGame.status()),
    mDisplayCount(1),
    mDisplayType(),
    mAltRomName(),
    mAltTitle(ESCAPE(!flashpointGame.orderTitle().isEmpty() ?
                     flashpointGame.orderTitle() :
                     flashpointGame.title())),
    mExtra(),
    mButtons(),
    mSeries(ESCAPE(flashpointGame.series())),
    mLanguage(ESCAPE(flashpointGame.language())),
    mRegion(),
    mRating()
{}

RomEntry::RomEntry(const Fp::AddApp& flashpointAddApp, const Fp::Game& parentGame) :
    Fe::Game(flashpointAddApp.id(), ESCAPE(addAppTitle(parentGame.title(), flashpointAddApp.name())), parentGame.platform()),
    mEmulator(Fp::NAME),
    mCloneOf(flashpointAddApp.parentId().toString(QUuid::WithoutBraces)),
    mYear(parentGame.releaseDate().date()),
    mManufacturer(ESCAPE(parentGame.developer())),
    mPlayers(),
    mRotation(0),
    mControl(),
    mStatus(parentGame.status()),
    mDisplayCount(1),
    mDisplayType(),
    mAltRomName(),
    mAltTitle(ESCAPE(!parentGame.orderTitle().isEmpty() ?
                     addAppSortTitle(parentGame.orderTitle(), flashpointAddApp.name()) :
                     addAppSortTitle(parentGame.title(), flashpointAddApp.name()))),
    mExtra(),
    mButtons(),
    mSeries(ESCAPE(parentGame.series())),
    mLanguage(ESCAPE(parentGame.language())),
    mRegion(),
    mRating()
{}

//-Class Functions-----------------------------------------------------------------------------------------------
//Public:
QString RomEntry::addAppTitle(const QString& parentTitle, const QString& originalAddAppTitle)
{
    return parentTitle + " |> " + originalAddAppTitle;
}

QString RomEntry::addAppSortTitle(const QString& parentTitle, const QString& originalAddAppTitle)
{
    return parentTitle + "     " + originalAddAppTitle;
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid RomEntry::name() const { return mId; } // Alias for Fe::Game::Id
QString RomEntry::title() const { return mName; }; // Alias for Fe::Game::name
QString RomEntry::emulator() const { return mEmulator; }
QString RomEntry::cloneOf() const { return mCloneOf; }
QDate RomEntry::year() const{ return mYear; }
QString RomEntry::manufacturer() const { return mManufacturer; }
QString RomEntry::category() const { return mPlatform; } // Alias for Fe::Game::platform
QString RomEntry::players() const { return mPlayers; }
quint8 RomEntry::rotation() const { return mRotation; }
QString RomEntry::control() const { return mControl; }
QString RomEntry::status() const { return mStatus; }
quint8 RomEntry::displayCount() const { return mDisplayCount; }
QString RomEntry::displayType() const { return mDisplayType; }
QString RomEntry::altRomName() const { return mAltRomName; }
QString RomEntry::altTitle() const { return mAltTitle; }
QString RomEntry::extra() const { return mExtra; }
QString RomEntry::buttons() const { return mButtons; }
QString RomEntry::series() const { return mSeries; }
QString RomEntry::language() const { return mLanguage; }
QString RomEntry::region() const { return mRegion; }
QString RomEntry::rating() const { return mRating; }

//===============================================================================================================
// Game::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
RomEntry::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
RomEntry::Builder& RomEntry::Builder::wName(QString nameAsId) { mItemBlueprint.mId = QUuid(nameAsId); return *this; }
RomEntry::Builder& RomEntry::Builder::wTitle(QString title) { mItemBlueprint.mName = title; return *this; }
RomEntry::Builder& RomEntry::Builder::wEmulator(QString emulator) { mItemBlueprint.mEmulator = emulator; return *this; }
RomEntry::Builder& RomEntry::Builder::wCloneOf(QString cloneOf) { mItemBlueprint.mCloneOf = cloneOf; return *this; }

RomEntry::Builder& RomEntry::Builder::wYear(QString rawYear)
{
    mItemBlueprint.mYear = QDate::fromString(rawYear, Qt::ISODate);
    return *this;
}

RomEntry::Builder& RomEntry::Builder::wManufacturer(QString manufacturer) { mItemBlueprint.mManufacturer = manufacturer; return *this; }
RomEntry::Builder& RomEntry::Builder::wCategory(QString category) { mItemBlueprint.mPlatform = category; return *this; }
RomEntry::Builder& RomEntry::Builder::wPlayers(QString players) { mItemBlueprint.mPlayers = players; return *this; }
RomEntry::Builder& RomEntry::Builder::wRotation(QString rawRotation) { mItemBlueprint.mRotation = rawRotation.toUInt(); return *this; }
RomEntry::Builder& RomEntry::Builder::wControl(QString control) { mItemBlueprint.mControl = control; return *this; }
RomEntry::Builder& RomEntry::Builder::wStatus(QString status) { mItemBlueprint.mStatus = status; return *this; }
RomEntry::Builder& RomEntry::Builder::wDisplayCount(QString rawDisplayCount) { mItemBlueprint.mDisplayCount = rawDisplayCount.toUInt(); return *this; }
RomEntry::Builder& RomEntry::Builder::wDisplayType(QString displayType) { mItemBlueprint.mDisplayType = displayType; return *this; }
RomEntry::Builder& RomEntry::Builder::wAltRomName(QString altRomName) { mItemBlueprint.mAltRomName = altRomName; return *this; }
RomEntry::Builder& RomEntry::Builder::wAltTitle(QString altTitle) { mItemBlueprint.mAltTitle = altTitle; return *this; }
RomEntry::Builder& RomEntry::Builder::wExtra(QString extra) { mItemBlueprint.mExtra = extra; return *this; }
RomEntry::Builder& RomEntry::Builder::wButtons(QString buttons) { mItemBlueprint.mButtons = buttons; return *this; }
RomEntry::Builder& RomEntry::Builder::wSeries(QString series) { mItemBlueprint.mSeries = series; return *this; }
RomEntry::Builder& RomEntry::Builder::wLanguage(QString language) { mItemBlueprint.mLanguage = language; return *this; }
RomEntry::Builder& RomEntry::Builder::wRegion(QString region) { mItemBlueprint.mRegion = region; return *this; }
RomEntry::Builder& RomEntry::Builder::wRating(QString rating) { mItemBlueprint.mRating = rating; return *this; }

//===============================================================================================================
// EmulatorArtworkEntry
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
EmulatorArtworkEntry::EmulatorArtworkEntry() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString EmulatorArtworkEntry::type() const{ return mType; }
QStringList EmulatorArtworkEntry::paths() const{ return mPaths; }

//===============================================================================================================
// EmulatorArtworkEntry::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
EmulatorArtworkEntry::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
EmulatorArtworkEntry::Builder& EmulatorArtworkEntry::Builder::wType(QString type) { mItemBlueprint.mType = type; return *this; }
EmulatorArtworkEntry::Builder& EmulatorArtworkEntry::Builder::wPaths(QStringList paths) { mItemBlueprint.mPaths = paths; return *this; }

}
