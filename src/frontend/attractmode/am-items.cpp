// Unit Include
#include "am-items.h"

// Project Includes
#include "../../clifp.h"

namespace Am
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
RomEntry::RomEntry() {}

RomEntry::RomEntry(const Fp::Game& flashpointGame) :
    Fe::Game(flashpointGame.id(), flashpointGame.title(), flashpointGame.platform()),
    mEmulator(Fp::NAME),
    mCloneOf(),
    mYear(flashpointGame.releaseDate()),
    mManufacturer(flashpointGame.developer()),
    mPlayers(flashpointGame.playMode()),
    mRotation(0),
    mControl(),
    mStatus(flashpointGame.status()),
    mDisplayCount(1),
    mDisplayType(),
    mAltRomName(),
    mAltTitle(flashpointGame.orderTitle()),
    mExtra(),
    mButtons(),
    mSeries(flashpointGame.series()),
    mLanguage(flashpointGame.language()),
    mRegion(),
    mRating()
{}

RomEntry::RomEntry(const Fp::AddApp& flashpointAddApp) :
    Fe::Game(flashpointAddApp.id(), flashpointAddApp.name(), QString()),
    mEmulator(Fp::NAME),
    mCloneOf(flashpointAddApp.parentId().toString(QUuid::WithoutBraces)),
    mYear(),
    mManufacturer(),
    mPlayers(),
    mRotation(0),
    mControl(),
    mStatus(),
    mDisplayCount(1),
    mDisplayType(),
    mAltRomName(),
    mAltTitle(),
    mExtra(),
    mButtons(),
    mSeries(),
    mLanguage(),
    mRegion(),
    mRating()
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid RomEntry::name() const { return mId; } // Alias for Fe::Game::Id
QString RomEntry::title() const { return mName; }; // Alias for Fe::Game::name
QString RomEntry::emulator() const { return mEmulator; }
QString RomEntry::cloneOf() const { return mCloneOf; }
QDateTime RomEntry::year() const{ return mYear; }
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
// GameBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
RomEntryBuilder::RomEntryBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
RomEntryBuilder& RomEntryBuilder::wName(QString nameAsId) { mItemBlueprint.mId = QUuid(nameAsId); return *this; }
RomEntryBuilder& RomEntryBuilder::wTitle(QString title) { mItemBlueprint.mName = title; return *this; }
RomEntryBuilder& RomEntryBuilder::wEmulator(QString emulator) { mItemBlueprint.mEmulator = emulator; return *this; }
RomEntryBuilder& RomEntryBuilder::wCloneOf(QString cloneOf) { mItemBlueprint.mCloneOf = cloneOf; return *this; }

RomEntryBuilder& RomEntryBuilder::wYear(QString rawYear)
{
    mItemBlueprint.mYear = QDateTime::fromString(rawYear);
    return *this;
}

RomEntryBuilder& RomEntryBuilder::wManufacturer(QString manufacturer) { mItemBlueprint.mManufacturer = manufacturer; return *this; }
RomEntryBuilder& RomEntryBuilder::wCategory(QString category) { mItemBlueprint.mPlatform = category; return *this; }
RomEntryBuilder& RomEntryBuilder::wPlayers(QString players) { mItemBlueprint.mPlayers = players; return *this; }
RomEntryBuilder& RomEntryBuilder::wRotation(QString rawRotation) { mItemBlueprint.mRotation = rawRotation.toUInt(); return *this; }
RomEntryBuilder& RomEntryBuilder::wControl(QString control) { mItemBlueprint.mControl = control; return *this; }
RomEntryBuilder& RomEntryBuilder::wStatus(QString status) { mItemBlueprint.mStatus = status; return *this; }
RomEntryBuilder& RomEntryBuilder::wDisplayCount(QString rawDisplayCount) { mItemBlueprint.mDisplayCount = rawDisplayCount.toUInt(); return *this; }
RomEntryBuilder& RomEntryBuilder::wDisplayType(QString displayType) { mItemBlueprint.mDisplayType = displayType; return *this; }
RomEntryBuilder& RomEntryBuilder::wAltRomName(QString altRomName) { mItemBlueprint.mAltRomName = altRomName; return *this; }
RomEntryBuilder& RomEntryBuilder::wAltTitle(QString altTitle) { mItemBlueprint.mAltTitle = altTitle; return *this; }
RomEntryBuilder& RomEntryBuilder::wExtra(QString extra) { mItemBlueprint.mExtra = extra; return *this; }
RomEntryBuilder& RomEntryBuilder::wButtons(QString buttons) { mItemBlueprint.mButtons = buttons; return *this; }
RomEntryBuilder& RomEntryBuilder::wSeries(QString series) { mItemBlueprint.mSeries = series; return *this; }
RomEntryBuilder& RomEntryBuilder::wLanguage(QString language) { mItemBlueprint.mLanguage = language; return *this; }
RomEntryBuilder& RomEntryBuilder::wRegion(QString region) { mItemBlueprint.mRegion = region; return *this; }
RomEntryBuilder& RomEntryBuilder::wRating(QString rating) { mItemBlueprint.mRating = rating; return *this; }

//===============================================================================================================
// CrudeMainConfigEntry
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
CrudeMainConfigEntry::CrudeMainConfigEntry() {}

//-Class Variables--------------------------------------------------------------------------------------------------
//Public:
QUuid CrudeMainConfigEntry::equivalentId(QString type, QString name)
{
    return QUuid::createUuidV5(NAMESPACE_SEED, type + name);
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString CrudeMainConfigEntry::type() const{ return mType; }
QStringList CrudeMainConfigEntry::contents() const{ return mContents; }

//===============================================================================================================
// CrudeMainConfigEntryBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
CrudeMainConfigEntryBuilder::CrudeMainConfigEntryBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
CrudeMainConfigEntryBuilder& CrudeMainConfigEntryBuilder::wTypeAndName(QString type, QString name)
{
    mItemBlueprint.mType = type;
    mItemBlueprint.mName = name;
    mItemBlueprint.mId = CrudeMainConfigEntry::equivalentId(type, name);
    return *this;
}
CrudeMainConfigEntryBuilder& CrudeMainConfigEntryBuilder::wContents(const QStringList& contents) { mItemBlueprint.mContents = contents; return *this; }

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
// EmulatorArtworkEntryBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
EmulatorArtworkEntryBuilder::EmulatorArtworkEntryBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
EmulatorArtworkEntryBuilder& EmulatorArtworkEntryBuilder::wType(QString type) { mItemBlueprint.mType = type; return *this; }
EmulatorArtworkEntryBuilder& EmulatorArtworkEntryBuilder::wPaths(QStringList paths) { mItemBlueprint.mPaths = paths; return *this; }

}
