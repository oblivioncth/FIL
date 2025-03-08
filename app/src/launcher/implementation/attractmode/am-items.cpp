// Unit Include
#include "am-items.h"

// Project Includes
#include <fp/fp-install.h>

// Quote escape
#define ESCAPE(str) (str).replace(uR"(")"_s, uR"(\")"_s)

namespace Am
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
RomEntry::RomEntry() {}

RomEntry::RomEntry(const Fp::Game& flashpointGame) :
    Lr::Game(flashpointGame.id(), ESCAPE(flashpointGame.title()), flashpointGame.platformName()),
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
                     flashpointGame.title()).toUpper()),
    mExtra(),
    mButtons(),
    mSeries(ESCAPE(flashpointGame.series())),
    mLanguage(ESCAPE(flashpointGame.language())),
    mRegion(),
    mRating()
{}

RomEntry::RomEntry(const Fp::AddApp& flashpointAddApp, const Fp::Game& parentGame) :
    Lr::Game(flashpointAddApp.id(), ESCAPE(addAppTitle(parentGame.title(), flashpointAddApp.name())), parentGame.platformName()),
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
    return parentTitle + u" |> "_s + originalAddAppTitle;
}

QString RomEntry::addAppSortTitle(const QString& parentTitle, const QString& originalAddAppTitle)
{
    /* Multiple space more-or-less ensure this title will directly follow the parent title,
     * uppercase ensures sorting isn't broken up between lower and uppercase letters as AM's
     * sorting doesn't account for case and seems to be a basic character code sorter
     */
    return (parentTitle + u"     "_s + originalAddAppTitle).toUpper();
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid RomEntry::name() const { return mId; } // Alias for Lr::Game::Id
QString RomEntry::title() const { return mName; }; // Alias for Lr::Game::name
QString RomEntry::emulator() const { return mEmulator; }
QString RomEntry::cloneOf() const { return mCloneOf; }
QDate RomEntry::year() const{ return mYear; }
QString RomEntry::manufacturer() const { return mManufacturer; }
QString RomEntry::category() const { return mPlatform; } // Alias for Lr::Game::platform
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
RomEntry::Builder& RomEntry::Builder::wName(const QString&  nameAsId) { mBlueprint.mId = QUuid(nameAsId); return *this; }
RomEntry::Builder& RomEntry::Builder::wTitle(const QString&  title) { mBlueprint.mName = title; return *this; }
RomEntry::Builder& RomEntry::Builder::wEmulator(const QString&  emulator) { mBlueprint.mEmulator = emulator; return *this; }
RomEntry::Builder& RomEntry::Builder::wCloneOf(const QString&  cloneOf) { mBlueprint.mCloneOf = cloneOf; return *this; }

RomEntry::Builder& RomEntry::Builder::wYear(const QString&  rawYear)
{
    mBlueprint.mYear = QDate::fromString(rawYear, Qt::ISODate);
    return *this;
}

RomEntry::Builder& RomEntry::Builder::wManufacturer(const QString& manufacturer) { mBlueprint.mManufacturer = manufacturer; return *this; }
RomEntry::Builder& RomEntry::Builder::wCategory(const QString& category) { mBlueprint.mPlatform = category; return *this; }
RomEntry::Builder& RomEntry::Builder::wPlayers(const QString& players) { mBlueprint.mPlayers = players; return *this; }
RomEntry::Builder& RomEntry::Builder::wRotation(const QString& rawRotation) { mBlueprint.mRotation = rawRotation.toUInt(); return *this; }
RomEntry::Builder& RomEntry::Builder::wControl(const QString& control) { mBlueprint.mControl = control; return *this; }
RomEntry::Builder& RomEntry::Builder::wStatus(const QString& status) { mBlueprint.mStatus = status; return *this; }
RomEntry::Builder& RomEntry::Builder::wDisplayCount(const QString& rawDisplayCount) { mBlueprint.mDisplayCount = rawDisplayCount.toUInt(); return *this; }
RomEntry::Builder& RomEntry::Builder::wDisplayType(const QString& displayType) { mBlueprint.mDisplayType = displayType; return *this; }
RomEntry::Builder& RomEntry::Builder::wAltRomName(const QString& altRomName) { mBlueprint.mAltRomName = altRomName; return *this; }
RomEntry::Builder& RomEntry::Builder::wAltTitle(const QString& altTitle) { mBlueprint.mAltTitle = altTitle; return *this; }
RomEntry::Builder& RomEntry::Builder::wExtra(const QString& extra) { mBlueprint.mExtra = extra; return *this; }
RomEntry::Builder& RomEntry::Builder::wButtons(const QString& buttons) { mBlueprint.mButtons = buttons; return *this; }
RomEntry::Builder& RomEntry::Builder::wSeries(const QString& series) { mBlueprint.mSeries = series; return *this; }
RomEntry::Builder& RomEntry::Builder::wLanguage(const QString& language) { mBlueprint.mLanguage = language; return *this; }
RomEntry::Builder& RomEntry::Builder::wRegion(const QString& region) { mBlueprint.mRegion = region; return *this; }
RomEntry::Builder& RomEntry::Builder::wRating(const QString& rating) { mBlueprint.mRating = rating; return *this; }

//===============================================================================================================
// Overview
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Overview::Overview(const QUuid& gameId, const QString& text) :
    mGameId(gameId),
    mText(text)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid Overview::gameId() const{ return mGameId; }
QString Overview::text() const{ return mText; }

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
EmulatorArtworkEntry::Builder& EmulatorArtworkEntry::Builder::wType(const QString& type) { mBlueprint.mType = type; return *this; }
EmulatorArtworkEntry::Builder& EmulatorArtworkEntry::Builder::wPaths(const QStringList& paths) { mBlueprint.mPaths = paths; return *this; }

}
