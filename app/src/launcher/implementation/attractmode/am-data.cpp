// Unit Include
#include "am-data.h"

// Qt Includes
#include <QtDebug>

// Qx Includes
#include <qx/core/qx-string.h>

// Project Includes
#include "launcher/implementation/attractmode/am-install.h"

namespace Am
{


//===============================================================================================================
// ConfigDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ConfigDoc::ConfigDoc(Install* install, const QString& filePath, QString docName) :
    Lr::DataDoc<LauncherId>(install, filePath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
QString ConfigDoc::versionedTagline()
{
    return TAGLINE + install()->versionString();
}



//===============================================================================================================
// Taglist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Taglist::Taglist(Install* install, const QString& listPath, QString docName) :
    Lr::DataDoc<LauncherId>(install, listPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Taglist::isEmpty() const
{
    return mTags.isEmpty();
}

bool Taglist::containsTag(QStringView tag) const { return mTags.contains(tag); }
void Taglist::appendTag(const QString& tag) { mTags.append(tag); }

//===============================================================================================================
// Taglist::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Taglist::Writer::Writer(Taglist* sourceDoc) :
    CommonDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Taglist::Writer::writeSourceDoc()
{
    // Write tags
    for(const QString& tag : std::as_const(source()->mTags))
        mStreamWriter << tag << '\n';

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// PlatformTaglist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Private:
PlatformTaglist::PlatformTaglist(Install* install, const QString& listPath, QString docName) :
    Am::Taglist(install, listPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::IDataDoc::Type PlatformTaglist::type() const { return Lr::IDataDoc::Type::Platform; }

//===============================================================================================================
// PlaylistTaglist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Private:
PlaylistTaglist::PlaylistTaglist(Install* install, const QString& listPath, QString docName) :
    Am::Taglist(install, listPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::IDataDoc::Type PlaylistTaglist::type() const { return Lr::IDataDoc::Type::Playlist; }

//===============================================================================================================
// Romlist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Romlist::Romlist(Install* install, const QString& listPath, QString docName, const Import::UpdateOptions& updateOptions) :
    Lr::UpdatableDoc<LauncherId>(install, listPath, docName, updateOptions),
    mEntries(this)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::IDataDoc::Type Romlist::type() const { return Lr::IDataDoc::Type::Config; }

bool Romlist::isEmpty() const { return mEntries.isEmpty(); }

bool Romlist::containsGame(const QUuid& gameId) const { return mEntries.contains(gameId); }
bool Romlist::containsAddApp(const QUuid& addAppId) const { return mEntries.contains(addAppId); }

const RomEntry* Romlist::processSet(const Fp::Set& set)
{
    // Prepare and romlist entry
    const RomEntry* mainRomEntry = mEntries.insert(RomEntry(set.game()));

    // Handle additional apps
    for(const Fp::AddApp& addApp : set.addApps())
    {
        // Ignore if not playable
        if(addApp.isPlayable())
            mEntries.insert(RomEntry(addApp, set.game()));
    }

    return mainRomEntry;
}

//===============================================================================================================
// Romlist::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Romlist::Reader::Reader(Romlist* targetDoc) :
    CommonDocReader<Romlist>(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool Romlist::Reader::checkDocValidity(bool& isValid)
{
    // See if first line is the romlist header
    QString header = mStreamReader.readLine();
    isValid = header == Romlist::HEADER;

    // Return status
    return !mStreamReader.hasError();
}

Lr::DocHandlingError Romlist::Reader::readTargetDoc()
{
    // Read all romlist entries
    while(!mStreamReader.atEnd())
    {
        QString rawEntry = readLineIgnoringComments();
        parseRomEntry(rawEntry);
    }

    // Only can have stream errors
    return Lr::DocHandlingError();
}

void Romlist::Reader::parseRomEntry(const QString& rawEntry)
{
    // Prepare builder and iterator
    RomEntry::Builder reb;
    QString::const_iterator itr = rawEntry.constBegin();
    QString::const_iterator lastCharPos = rawEntry.constEnd() - 1;

    // Parse support
    bool inQuotes = false;
    QString currentField;
    quint8 currentFieldIndex = 0;

    // Parse entry
    while(itr != rawEntry.constEnd())
    {
        const QChar& c = *itr;
        bool lastChar = itr == lastCharPos;

        if(c == '"') // Quote change
            inQuotes = !inQuotes;

        if((c == ';' && !inQuotes) || lastChar) // Field separator
        {
            addFieldToBuilder(reb, currentField, currentFieldIndex);
            currentField.clear();
            currentFieldIndex++;
        }
        else if(c != '"')
            currentField += c;

        // Advance
        itr++;
    }

    // Ensure parsing ended out of quotes
    if(inQuotes)
        qWarning("Missing terminating '\"' character for ROM entry %s", qPrintable(rawEntry));

    // Build Entry and add to document
    target()->mEntries.insert(reb.build());
}

void Romlist::Reader::addFieldToBuilder(RomEntry::Builder& builder, QString field, quint8 index)
{
    switch(index)
    {
        case 0:
            builder.wName(field);
            break;
        case 1:
            builder.wTitle(field);
            break;
        case 2:
            builder.wEmulator(field);
            break;
        case 3:
            builder.wCloneOf(field);
            break;
        case 4:
            builder.wYear(field);
            break;
        case 5:
            builder.wManufacturer(field);
            break;
        case 6:
            builder.wCategory(field);
            break;
        case 7:
            builder.wPlayers(field);
            break;
        case 8:
            builder.wRotation(field);
            break;
        case 9:
            builder.wControl(field);
            break;
        case 10:
            builder.wStatus(field);
            break;
        case 11:
            builder.wDisplayCount(field);
            break;
        case 12:
            builder.wDisplayType(field);
            break;
        case 13:
            builder.wAltRomName(field);
            break;
        case 14:
            builder.wAltTitle(field);
            break;
        case 15:
            builder.wExtra(field);
            break;
        case 16:
            builder.wButtons(field);
            break;
        case 17:
            builder.wSeries(field);
            break;
        case 18:
            builder.wLanguage(field);
            break;
        case 19:
            builder.wRegion(field);
            break;
        case 20:
            builder.wRating(field);
            break;
        default:
            qWarning("Unhandled RomEntry field");
    }
}

//===============================================================================================================
// Romlist::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Romlist::Writer::Writer(Romlist* sourceDoc) :
    CommonDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool Romlist::Writer::writeSourceDoc()
{
    // Write rom list header
    mStreamWriter.writeLine(Romlist::HEADER);

    // Write all rom entries
    if(!source()->mEntries.forEachFinal([this](const RomEntry& re){ return writeRomEntry(re); }))
        return false;

    // Return true on success
    return true;
}

bool Romlist::Writer::writeRomEntry(const RomEntry& romEntry)
{
    writeEntryField(romEntry.name().toString(QUuid::WithoutBraces));
    writeEntryField(romEntry.title());
    writeEntryField(romEntry.emulator());
    writeEntryField(romEntry.cloneOf());
    writeEntryField(romEntry.year().toString(Qt::ISODate));
    writeEntryField(romEntry.manufacturer());
    writeEntryField(romEntry.category());
    writeEntryField(romEntry.players());
    writeEntryField(QString::number(romEntry.rotation()));
    writeEntryField(romEntry.control());
    writeEntryField(romEntry.status());
    writeEntryField(QString::number(romEntry.displayCount()));
    writeEntryField(romEntry.displayType());
    writeEntryField(romEntry.altRomName());
    writeEntryField(romEntry.altTitle());
    writeEntryField(romEntry.extra());
    writeEntryField(romEntry.buttons());
    writeEntryField(romEntry.series());
    writeEntryField(romEntry.language());
    writeEntryField(romEntry.region());
    writeEntryField(romEntry.rating(), false);
    mStreamWriter << '\n';

    return !mStreamWriter.hasError();
}

void Romlist::Writer::writeEntryField(const QString& entryField, bool writeSeperator)
{
    mStreamWriter << '"' << entryField << '"';
    if(writeSeperator)
        mStreamWriter << ';';
}

//===============================================================================================================
// BulkOverviewWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
BulkOverviewWriter::BulkOverviewWriter(const QDir& overviewDir) :
    mOverviewDir(overviewDir),
    mFile()
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
QString BulkOverviewWriter::currentFilePath() { return mFile.fileName(); }
QString BulkOverviewWriter::fileErrorString() { return mFile.errorString(); }

bool BulkOverviewWriter::writeOverview(const Overview& overview)
{
    // Set file to overview path
    QString fileName = overview.gameId().toString(QUuid::WithoutBraces) + u".txt"_s;
    mFile.setFileName(mOverviewDir.absoluteFilePath(fileName));

    // Open file, always truncate
    mFile.open(QSaveFile::WriteOnly); // Write only implies truncate

    // Write overview
    mFile.write(overview.text().toUtf8());

    // Save and return status
    return mFile.commit();
}

//===============================================================================================================
// PlatformInterface
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformInterface::PlatformInterface(Install* install, const QString& platformTaglistPath, QString platformName,
                                     const QDir& overviewDir) :
    Lr::PlatformDoc<LauncherId>(install, {}, platformName, {}),
    mPlatformTaglist(install, platformTaglistPath, platformName),
    mOverviewDir(overviewDir)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
const RomEntry* PlatformInterface::processSet(const Fp::Set& set)
{
    //-Handle game----------------------------------------------------------
    const Fp::Game& game = set.game();

    // Add game ID to platform tag list
    mPlatformTaglist.appendTag(game.id().toString(QUuid::WithoutBraces));

    // Create game overview
    QString overviewText = game.originalDescription();
    if(!overviewText.isEmpty())
        mOverviews.emplaceBack(game.id(), overviewText);

    //-Handle add apps-------------------------------------------------------

    // Add add app IDs to platform tag list
    for(const Fp::AddApp& addApp : set.addApps())
    {
        /* Ignore non-playable add apps to avoid useless clutter in AM
         * TODO: Consider doing this in Import Worker to make it a standard since
         * LB doesn't actually need the non-playable entries either. Importing them
         * is basically a leftover from an earlier CLIFp version that required them
         * for games to work (i.e. before auto mode).
         */
        if(addApp.isPlayable())
            mPlatformTaglist.appendTag(addApp.id().toString(QUuid::WithoutBraces));
    }

    //-Forward game insertion to main Romlist--------------------------------
    return install()->mRomlist->processSet(set);
}

//Public:
bool PlatformInterface::isEmpty() const { return mPlatformTaglist.isEmpty(); }

bool PlatformInterface::containsGame(const QUuid& gameId) const
{
    /* Check main romlist for ID. Could check the taglist instead, which would be more "correct" since only the current
     * platform should contain the ID, but this doesn't matter given correct design and the lookup is performed via
     * the romlist's internal hash, which is faster than checking a list for the presence of the ID.
     */
    return install()->mRomlist->containsGame(gameId);
}

bool PlatformInterface::containsAddApp(const QUuid& addAppId) const
{
    /* Check main romlist for ID. Could check the taglist instead, which would be more "correct" since only the current
     * platform should contain the ID, but this doesn't matter given correct design and the lookup is performed via
     * the romlist's internal hash, which is faster than checking a list for the presence of the ID.
     */
    return install()->mRomlist->containsAddApp(addAppId);
};

//===============================================================================================================
// PlatformInterfaceWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformInterfaceWriter::PlatformInterfaceWriter(PlatformInterface* sourceDoc) :
    Lr::DataDocWriter<PlatformInterface>(sourceDoc),
    mTaglistWriter(&sourceDoc->mPlatformTaglist),
    mOverviewWriter(sourceDoc->mOverviewDir) // TODO: Maybe a better way to pass this then it just sitting in the doc?
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError PlatformInterfaceWriter::writeOutOf()
{
    // Write tag list
    if(auto err = mTaglistWriter.writeOutOf())
        return err;

    // Write overviews
    for(const auto& o : std::as_const(source()->mOverviews))
    {
        // This uses QSaveFile as a form of "safe replace" write, so we don't need to manually back-up
        if(!mOverviewWriter.writeOverview(o))
            return Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocWriteFailed, mOverviewWriter.fileErrorString());
    }

    return {};
}

//===============================================================================================================
// PlaylistInterface
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistInterface::PlaylistInterface(Install* install, const QString& playlistTaglistPath, QString playlistName) :
    Lr::PlaylistDoc<LauncherId>(install, {}, playlistName, {}),
    mPlaylistTaglist(install, playlistTaglistPath, playlistName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool PlaylistInterface::isEmpty() const { return mPlaylistTaglist.isEmpty(); }

bool PlaylistInterface::containsPlaylistGame(const QUuid& gameId) const
{
    return mPlaylistTaglist.containsTag(gameId.toString(QUuid::WithoutBraces));
}

void PlaylistInterface::setPlaylistData(const Fp::Playlist& playlist)
{
    for(const auto& pl : playlist.playlistGames())
        mPlaylistTaglist.appendTag(pl.gameId().toString(QUuid::WithoutBraces));
}

//===============================================================================================================
// PlaylistInterfaceWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistInterfaceWriter::PlaylistInterfaceWriter(PlaylistInterface* sourceDoc) :
    Lr::DataDocWriter<PlaylistInterface>(sourceDoc),
    mTaglistWriter(&sourceDoc->mPlaylistTaglist)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Lr::DocHandlingError PlaylistInterfaceWriter::writeOutOf() { return mTaglistWriter.writeOutOf(); }

//===============================================================================================================
// Emulator
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Emulator::Emulator(Install* install, const QString& filePath) :
    ConfigDoc(install, filePath, STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Emulator::isEmpty() const { return false; } // Can have blank fields, but always has field keys
Lr::IDataDoc::Type Emulator::type() const { return Lr::IDataDoc::Type::Config; }

QString Emulator::executable() const { return mExecutable; }
QString Emulator::args() const { return mArgs; }
QString Emulator::workDir() const { return mWorkDir; }
QString Emulator::romPath() const { return mRomPath; }
QString Emulator::romExt() const { return mRomExt; }
QString Emulator::system() const { return mSystem; }
QString Emulator::infoSource() const { return mInfoSource; }
QString Emulator::exitHotkey() const { return mExitHotkey; }
EmulatorArtworkEntry Emulator::artworkEntry(const QString& type) const { return mArtworkEntries.value(type); }
QList<EmulatorArtworkEntry> Emulator::artworkEntries() const { return mArtworkEntries.values(); }

void Emulator::setExecutable(const QString& executable) { mExecutable = executable; }
void Emulator::setArgs(const QString& args) { mArgs = args; }
void Emulator::setWorkDir(const QString& workDir) { mWorkDir = workDir; }
void Emulator::setRomPath(const QString& romPath) { mRomPath = romPath; }
void Emulator::setRomExt(const QString& romExt) { mRomExt = romExt; }
void Emulator::setSystem(const QString& system) { mSystem = system; }
void Emulator::setInfoSource(const QString& infoSource) { mInfoSource = infoSource; }
void Emulator::setExitHotkey(const QString& exitHotkey) { mExitHotkey = exitHotkey; }
void Emulator::setArtworkEntry(const EmulatorArtworkEntry& entry) { mArtworkEntries[entry.type()] = entry; }

//===============================================================================================================
// Emulator::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
EmulatorReader::EmulatorReader(Emulator* targetDoc) :
    ConfigDoc::Reader<Emulator>(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Lr::DocHandlingError EmulatorReader::readTargetDoc()
{
    while(!mStreamReader.atEnd())
    {
        QString line = mStreamReader.readLine();
        QString key;
        QString value;
        splitKeyValue(line, key, value);
        parseKeyValue(key, value);
    }

    // Only can have stream related errors
    return Lr::DocHandlingError();
}

void EmulatorReader::parseKeyValue(const QString& key, const QString& value)
{
    if(key == Emulator::Keys::EXECUTABLE)
        parseExecutable(value);
    else if(key == Emulator::Keys::ARGS)
        parseArgs(value);
    else if(key == Emulator::Keys::WORK_DIR)
        parseWorkDir(value);
    else if(key == Emulator::Keys::ROM_PATH)
        parseRomPath(value);
    else if(key == Emulator::Keys::ROM_EXT)
        parseRomExt(value);
    else if(key == Emulator::Keys::SYSTEM)
        parseSystem(value);
    else if(key == Emulator::Keys::INFO_SOURCE)
        parseInfoSource(value);
    else if(key == Emulator::Keys::EXIT_HOTKEY)
        parseExitHotkey(value);
    else if(key == Emulator::Keys::ARTWORK)
        parseArtwork(value);
}

void EmulatorReader::parseExecutable(const QString& value) { target()->setExecutable(value); }
void EmulatorReader::parseArgs(const QString& value) { target()->setArgs(value); }
void EmulatorReader::parseWorkDir(const QString& value) { target()->setWorkDir(value); }
void EmulatorReader::parseRomPath(const QString& value)
{
    target()->setRomPath(value == uR"("")"_s ? u""_s : value);
}
void EmulatorReader::parseRomExt(const QString& value)
{
    target()->setRomPath(value == uR"("")"_s ? u""_s : value);
}
void EmulatorReader::parseSystem(const QString& value) { target()->setSystem(value); }
void EmulatorReader::parseInfoSource(const QString& value) { target()->setInfoSource(value); }
void EmulatorReader::parseExitHotkey(const QString& value) { target()->setExitHotkey(value); }
void EmulatorReader::parseArtwork(const QString& value)
{
    QString type;
    QString rawPaths;
    splitKeyValue(value, type, rawPaths);

    EmulatorArtworkEntry::Builder eaeb;
    eaeb.wType(type);
    eaeb.wPaths(!rawPaths.isEmpty() ? rawPaths.split(';') : QStringList());

    target()->setArtworkEntry(eaeb.build());
}

//===============================================================================================================
// Emulator::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Emulator::Writer::Writer(Emulator* sourceDoc) :
    ConfigDoc::Writer<Emulator>(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool Emulator::Writer::writeConfigDoc()
{
    // Set field alignment
    mStreamWriter.setFieldAlignment(QTextStream::AlignLeft);

    // Write main key/values
    writeStandardKeyValue(Emulator::Keys::EXECUTABLE, source()->executable());
    writeStandardKeyValue(Emulator::Keys::ARGS, source()->args());
    writeStandardKeyValue(Emulator::Keys::WORK_DIR, source()->workDir());
    writeStandardKeyValue(Emulator::Keys::ROM_PATH, source()->romPath());
    writeStandardKeyValue(Emulator::Keys::ROM_EXT, source()->romExt());
    writeStandardKeyValue(Emulator::Keys::SYSTEM, source()->system());
    writeStandardKeyValue(Emulator::Keys::INFO_SOURCE, source()->infoSource());
    writeStandardKeyValue(Emulator::Keys::EXIT_HOTKEY, source()->exitHotkey());

    // Write artwork entries
    const QList<EmulatorArtworkEntry> artworkEntries = source()->artworkEntries();
    for(const EmulatorArtworkEntry& entry : artworkEntries)
        writeArtworkEntry(entry);

    // Return stream status
    return !mStreamWriter.hasError();
}

void Emulator::Writer::writeStandardKeyValue(const QString& key, const QString& value)
{
    mStreamWriter.setFieldWidth(STD_KEY_FIELD_WIDTH);
    mStreamWriter << key;
    mStreamWriter.setFieldWidth(0);
    mStreamWriter << value << '\n';
}

void Emulator::Writer::writeArtworkEntry(const EmulatorArtworkEntry& entry)
{
    mStreamWriter.setFieldWidth(ARTWORK_KEY_FIELD_WIDTH);
    mStreamWriter << Emulator::Keys::ARTWORK;
    mStreamWriter.setFieldWidth(ARTWORK_TYPE_FIELD_WIDTH);
    mStreamWriter << entry.type();
    mStreamWriter.setFieldWidth(0);
    mStreamWriter << entry.paths().join(';');
    mStreamWriter << '\n';
}

}
