// Unit Include
#include "es-items.h"

// Qx Includes
#include <qx/io/qx-common-io.h>

// Project Includes
#include "launcher/implementation/emulationstation/es-install.h"

namespace Es
{
//===============================================================================================================
// Game
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}
Game::Game(const Fp::Game& fpGame, const Fp::GameTags& fpTags, const QString& systemName) :
    Lr::Game(fpGame.id(), fpGame.title(), systemName),
    mPath(u"./"_s + filenameFromId(fpGame.id())),
    mSortName(fpGame.orderTitle()),
    mCollectionSortName(mSortName),
    mDesc(fpGame.notes()),
    mReleaseDate(fpGame.releaseDate()),
    mDeveloper(fpGame.developer()),
    mPublisher(fpGame.publisher()),
    mGenre(fpTags.tags(u"genre"_s).join(u" | "_s)),
    mPlayers(fpGame.playMode()),
    mKidGame(fpTags.tags(u"warning"_s).isEmpty())
{}

Game::Game(const Fp::AddApp& fpAddApp, const Fp::Game& parentGame, const Fp::GameTags& parentGameTags, const QString& systemName) :
    Lr::Game(fpAddApp.id(), addAppTitle(parentGame.title(), fpAddApp.name()), systemName),
    mPath(u"./"_s + filenameFromId(fpAddApp.id())),
    mSortName(!parentGame.orderTitle().isEmpty() ?
              addAppSortTitle(parentGame.orderTitle(), fpAddApp.name()) :
              addAppSortTitle(parentGame.title(), fpAddApp.name())),
    mCollectionSortName(mSortName),
    mDesc(),
    mReleaseDate(parentGame.releaseDate()),
    mDeveloper(parentGame.developer()),
    mPublisher(parentGame.publisher()),
    mGenre(parentGameTags.tags(u"genre"_s).join(u" | "_s)),
    mPlayers(parentGame.playMode()),
    mKidGame(parentGameTags.tags(u"warning"_s).isEmpty())
{}

//-Class Functions------------------------------------------------------------------------------------------------------
//Private:
QString Game::addAppTitle(const QString& parentTitle, const QString& originalAddAppTitle)
{
    return parentTitle + u" |> "_s + originalAddAppTitle;
}

QString Game::addAppSortTitle(const QString& parentTitle, const QString& originalAddAppTitle)
{
    /* Multiple space more-or-less ensure this title will directly follow the parent title */
    return parentTitle + u"     "_s + originalAddAppTitle;
}

//Public:
QString Game::filenameFromId(const QUuid& id) { return id.toString(QUuid::WithoutBraces) + '.' + Install::DUMMY_EXT; }
QUuid Game::idFromFilename(const QString& filename) { return QUuid::fromString(filename.chopped(Install::DUMMY_EXT.length() + 1)); } // +1 for '.'

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Game::path() const { return mPath; }
QString Game::sortName() const { return mSortName; }
QString Game::collectionSortName() const { return mCollectionSortName; }
QString Game::desc() const { return mDesc; }
QDateTime Game::releaseDate() const { return mReleaseDate; }
QString Game::developer() const { return mDeveloper; }
QString Game::publisher() const { return mPublisher; }
QString Game::genre() const { return mGenre; }
QString Game::players() const { return mPlayers; }
bool Game::kidGame() const { return mKidGame; }
QString Game::systemName() const { return mPlatform; }

//===============================================================================================================
// Game::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Game::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Game::Builder& Game::Builder::wPath(const QString& path) { mBlueprint.mPath = path; return *this; }
Game::Builder& Game::Builder::wSortName(const QString& sortName) { mBlueprint.mSortName = sortName; return *this; }
Game::Builder& Game::Builder::wCollectionSortName(const QString& collectionSortName) { mBlueprint.mCollectionSortName = collectionSortName; return *this; }
Game::Builder& Game::Builder::wDesc(const QString& desc) { mBlueprint.mDesc = desc; return *this; }

Game::Builder& Game::Builder::wReleaseDate(const QString& rawReleaseDate)
{
    mBlueprint.mReleaseDate = QDateTime::fromString(rawReleaseDate, Qt::ISODate);;
    return *this;
}

Game::Builder& Game::Builder::wDeveloper(const QString& developer) { mBlueprint.mDeveloper = developer; return *this; }

Game::Builder& Game::Builder::wPublisher(const QString& publisher) { mBlueprint.mPublisher = publisher; return *this; }
Game::Builder& Game::Builder::wGenre(const QString& genre) { mBlueprint.mGenre = genre; return *this; }
Game::Builder& Game::Builder::wPlayers(const QString& players) { mBlueprint.mPlayers = players; return *this; }
Game::Builder& Game::Builder::wKidGame(const QString& rawKidGame) { mBlueprint.mKidGame = rawKidGame == u"true"_s; return *this; }

//===============================================================================================================
// Folder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Folder::Folder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Folder::path() const { return mPath; }

//===============================================================================================================
// Folder::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Folder::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
Folder::Builder& Folder::Builder::wPath(const QString& path) { mBlueprint.mPath = path; return *this; }

//===============================================================================================================
// CollectionEntry
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
CollectionEntry::CollectionEntry() {}

CollectionEntry::CollectionEntry(const QUuid& gameId, const QString& systemName) :
    BasicItem(gameId),
    mSystemName(systemName)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString CollectionEntry::systemName() const { return mSystemName; }

//===============================================================================================================
// CollectionEntry::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
CollectionEntry::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
CollectionEntry::Builder& CollectionEntry::Builder::wSystemName(const QString& name) { mBlueprint.mSystemName = name; return *this; }

//===============================================================================================================
// System
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
System::System() {}

//-Class Functions------------------------------------------------------------------------------------------------------
//Public:
QString System::originalNameToFullName(const QString& original) { return FULL_NAME_PREFIX + ' ' + original; }
QString System::fullNameToName(const QString& full)
{
    QString n(full);

    // Swap prefixes
    n.replace(0, FULL_NAME_PREFIX.size(), NAME_PREFIX);

    // Replace spaces with underscores
    n.replace(u' ', u'_');

    // Kosherize + lowercase otherwise
    return Qx::kosherizeFileName(n.toLower());
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString System::name() const { return mName; }
QString System::fullName() const { return mFullName; }
QString System::systemSortName() const { return mSystemSortName; }
QString System::path() const { return mPath; }
QString System::extension() const { return mExtension; }
const QHash<QString, QString>& System::commands() const { return mCommands; }
QString System::platform() const { return mPlatform; }
QString System::theme() const { return mTheme; }

//===============================================================================================================
// System::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
System::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
System::Builder& System::Builder::wName(const QString& name) { mBlueprint.mName = name; return *this; }
System::Builder& System::Builder::wFullName(const QString& name) { mBlueprint.mFullName = name; return *this; }
System::Builder& System::Builder::wSystemSortName(const QString& name) { mBlueprint.mSystemSortName = name; return *this; }
System::Builder& System::Builder::wPath(const QString& path) { mBlueprint.mPath = path; return *this; }
System::Builder& System::Builder::wExtension(const QString& extension) { mBlueprint.mExtension = extension; return *this; }
System::Builder& System::Builder::wCommand(const QString& label, const QString& command) { mBlueprint.mCommands[label] = command; return *this; }
System::Builder& System::Builder::wPlatform(const QString& platform) { mBlueprint.mPlatform = platform; return *this; }
System::Builder& System::Builder::wTheme(const QString& theme) { mBlueprint.mTheme = theme; return *this; }

//===============================================================================================================
// Setting
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Setting::Setting() {}

Setting::Setting(const QString& type, const QString& name, const QString& value) :
    mType(QMetaType::fromName(type.toLatin1())),
    mName(name),
    mValue(value)
{}

Setting::Setting(const QMetaType& type, const QString& name, const QString& value) :
    mType(type),
    mName(name),
    mValue(value)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Setting::type() const { return QString::fromLatin1(mType.name()); }
QString Setting::value() const { return mValue; }
QString Setting::name() const { return mName; }
bool Setting::isValid() const { return mType.isValid() && !mName.isEmpty() && !mValue.isNull() && QMetaType::canConvert(QSTRING_TYPE, mType); }

}
