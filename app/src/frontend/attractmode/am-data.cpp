// Unit Include
#include "am-data.h"

// Qt Includes
#include <QtDebug>

// Qx Includes
#include <qx/core/qx-string.h>

// Project Includes
#include "am-install.h"

namespace Am
{
//===============================================================================================================
// CommonDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
CommonDocReader::CommonDocReader(Fe::DataDoc* targetDoc) :
    Fe::DataDoc::Reader(targetDoc),
    mStreamReader(targetDoc->path())
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
bool CommonDocReader::lineIsComment(const QString& line) { return line.front() == '#'; }

QString CommonDocReader::readLineIgnoringComments(qint64 maxlen)
{
    QString line;

    do
        line = mStreamReader.readLine(maxlen);
    while(!line.isEmpty() && line.front() == '#'); // Must check for empty string due to QString::front() constraints

    return line;
}

//Public:
Fe::DocHandlingError CommonDocReader::readInto()
{
    // Open file
    Qx::IoOpReport openError =  mStreamReader.openFile();
    if(openError.isFailure())
        return Fe::DocHandlingError(*mTargetDocument, Fe::DocHandlingError::DocCantOpen, openError.outcomeInfo());

    // Check that doc is valid
    bool isValid = false;
    if(!checkDocValidity(isValid))
        return Fe::DocHandlingError(*mTargetDocument, Fe::DocHandlingError::DocWriteFailed, mStreamReader.status().outcomeInfo());
    else if(!isValid)
        return Fe::DocHandlingError(*mTargetDocument, Fe::DocHandlingError::DocInvalidType);

    // Read doc
    Fe::DocHandlingError parseError = readTargetDoc();

    // Close file
    mStreamReader.closeFile();

    // Return outcome
    if(parseError.isValid())
        return parseError;
    else if(mStreamReader.hasError())
        return Fe::DocHandlingError(*mTargetDocument, Fe::DocHandlingError::DocWriteFailed, mStreamReader.status().outcomeInfo());
    else
        return Fe::DocHandlingError();
}

//===============================================================================================================
// CommonDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
CommonDocWriter::CommonDocWriter(Fe::DataDoc* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    mStreamWriter(sourceDoc->path(), Qx::WriteMode::Truncate)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Fe::DocHandlingError CommonDocWriter::writeOutOf()
{
    // Open file
    Qx::IoOpReport openError =  mStreamWriter.openFile();
    if(openError.isFailure())
        return Fe::DocHandlingError(*mSourceDocument, Fe::DocHandlingError::DocCantOpen, openError.outcomeInfo());

    // Write doc
    bool writeSuccess = writeSourceDoc();

    // Close file
    mStreamWriter.closeFile();

    // Return outcome
    return writeSuccess ? Fe::DocHandlingError() :
                          Fe::DocHandlingError(*mSourceDocument, Fe::DocHandlingError::DocWriteFailed, mStreamWriter.status().outcomeInfo());
}

//===============================================================================================================
// ConfigDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ConfigDoc::ConfigDoc(Install* const parent, const QString& filePath, QString docName) :
    Fe::DataDoc(parent, filePath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Protected:
QString ConfigDoc::versionedTagline()
{
    QString verString = static_cast<Install*>(parent())->versionString();
    return TAGLINE + verString;
}

//===============================================================================================================
// ConfigDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
ConfigDoc::Reader::Reader(ConfigDoc* targetDoc) :
    CommonDocReader(targetDoc)
{}

//-Class Functions-------------------------------------------------------------------------------------------------
//Protected:
bool ConfigDoc::Reader::splitKeyValue(const QString& line, QString& key, QString& value)
{
    /* TODO: The result from this function is currently unused due to no easy way to raise a custom
     * error with the stream reader in this class (and how the current paradigm is to return bools
     * for each step and then use the reader status if one is found). If used properly this should
     * never error, but ideally it should be checked for anyway. Might need to have all read functions
     * return Qx::GenericError to allow non stream related errors to be returned.
     */

    // Null out return buffers
    key = QString();
    value = QString();

    QRegularExpressionMatch keyValueCheck = KEY_VALUE_REGEX.match(line);
    if(keyValueCheck.hasMatch())
    {
        key = keyValueCheck.captured("key");
        value = keyValueCheck.captured("value");
        return true;
    }
    else
    {
        qWarning() << Q_FUNC_INFO << "Invalid key value string";
        return false;
    }
}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
bool ConfigDoc::Reader::checkDocValidity(bool& isValid)
{
    // Check for config "header"
    QString firstLine = mStreamReader.readLine();
    QString secondLine = mStreamReader.readLine();

    bool hasTagline = firstLine.left(ConfigDoc::TAGLINE.length()) == ConfigDoc::TAGLINE;

    isValid = hasTagline && lineIsComment(secondLine);

    // Return status
    return !mStreamReader.hasError();
}

//===============================================================================================================
// ConfigDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ConfigDoc::Writer::Writer(ConfigDoc* sourceDoc) :
    CommonDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool ConfigDoc::Writer::writeSourceDoc()
{
    // Write config doc "header"
    mStreamWriter.writeLine(static_cast<ConfigDoc*>(mSourceDocument)->versionedTagline());
    mStreamWriter.writeLine("#");

    if(mStreamWriter.hasError())
        return false;

    // Perform custom writing
    return writeConfigDoc();
}

//===============================================================================================================
// Taglist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Taglist::Taglist(Install* const parent, const QString& listPath, QString docName) :
    Fe::DataDoc(parent, listPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Taglist::isEmpty() const
{
    return mTags.isEmpty();
}

bool Taglist::containsTag(QString tag) const { return mTags.contains(tag); }
void Taglist::appendTag(QString tag) { mTags.append(tag); }

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
    // Get actual tag list
    Taglist* sourceList =  static_cast<Taglist*>(mSourceDocument);

    // Write tags
    for(const QString& tag : qAsConst(sourceList->mTags))
        mStreamWriter << tag << "\n";

    // Return error status
    return !mStreamWriter.hasError();
}

//===============================================================================================================
// PlatformTaglist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Private:
PlatformTaglist::PlatformTaglist(Install* const parent, const QString& listPath, QString docName) :
    Am::Taglist(parent, listPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Fe::DataDoc::Type PlatformTaglist::type() const { return Fe::DataDoc::Type::Platform; }

//===============================================================================================================
// PlaylistTaglist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Private:
PlaylistTaglist::PlaylistTaglist(Install* const parent, const QString& listPath, QString docName) :
    Am::Taglist(parent, listPath, docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Fe::DataDoc::Type PlaylistTaglist::type() const { return Fe::DataDoc::Type::Playlist; }

//===============================================================================================================
// Romlist
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Romlist::Romlist(Install* const parent, const QString& listPath, QString docName, Fe::UpdateOptions updateOptions,
                         const DocKey&) :
    Fe::UpdateableDoc(parent, listPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Fe::DataDoc::Type Romlist::type() const { return Fe::DataDoc::Type::Config; }

bool Romlist::isEmpty() const
{
    return mEntriesExisting.isEmpty() && mEntriesFinal.isEmpty();
}

const QHash<QUuid, std::shared_ptr<RomEntry>>& Romlist::finalEntries() const { return mEntriesFinal; }

bool Romlist::containsGame(QUuid gameId) const { return mEntriesExisting.contains(gameId) || mEntriesFinal.contains(gameId); }
bool Romlist::containsAddApp(QUuid addAppId) const { return mEntriesExisting.contains(addAppId) || mEntriesFinal.contains(addAppId); };

void Romlist::addSet(const Fp::Set& set, const Fe::ImageSources& images)
{
    // Convert to romlist entry
    std::shared_ptr<RomEntry> mainRomEntry = std::make_shared<RomEntry>(set.game());

    // Add entry
    addUpdateableItem(mEntriesExisting, mEntriesFinal, mainRomEntry);

    // Handle additional apps
    for(const Fp::AddApp& addApp : set.addApps())
    {
        // Ignore if not playable
        if(addApp.isPlayable())
        {
            // Convert to romlist entry
            std::shared_ptr<RomEntry> subRomEntry = std::make_shared<RomEntry>(addApp, set.game());

            // Add entry
            addUpdateableItem(mEntriesExisting, mEntriesFinal, subRomEntry);
        }
    }

    // Allow install to process images as necessary
    parent()->processDirectGameImages(mainRomEntry.get(), images);
}


void Romlist::finalize()
{
    finalizeUpdateableItems(mEntriesExisting, mEntriesFinal);
    Fe::UpdateableDoc::finalize();
}

//===============================================================================================================
// Romlist::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Romlist::Reader::Reader(Romlist* targetDoc) :
    CommonDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
QHash<QUuid, std::shared_ptr<RomEntry>>& Romlist::Reader::targetDocExistingRomEntries()
{
    return static_cast<Romlist*>(mTargetDocument)->mEntriesExisting;
}

bool Romlist::Reader::checkDocValidity(bool& isValid)
{
    // See if first line is the romlist header
    QString header = mStreamReader.readLine();
    isValid = header == Romlist::HEADER;

    // Return status
    return !mStreamReader.hasError();
}

Fe::DocHandlingError Romlist::Reader::readTargetDoc()
{
    // Read all romlist entries
    while(!mStreamReader.atEnd())
    {
        QString rawEntry = readLineIgnoringComments();
        parseRomEntry(rawEntry);
    }

    // Only can have stream errors
    return Fe::DocHandlingError();
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
        qWarning() << Q_FUNC_INFO << "Missing terminating '\"' character for ROM entry" << rawEntry;

    // Build Entry and add to document
    std::shared_ptr<RomEntry> existingEntry = reb.buildShared();
    targetDocExistingRomEntries()[existingEntry->id()] = existingEntry;
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
            qWarning() << Q_FUNC_INFO << "Unhandled RomEntry field";
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
    for(const std::shared_ptr<RomEntry>& entry : qAsConst(static_cast<Romlist*>(mSourceDocument)->finalEntries()))
    {
        if(!writeRomEntry(*entry))
            return false;
    }

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

bool BulkOverviewWriter::writeOverview(const QUuid& gameId, const QString& overview)
{
    // Set file to overview path
    QString fileName = gameId.toString(QUuid::WithoutBraces) + QStringLiteral(".txt");
    mFile.setFileName(mOverviewDir.absoluteFilePath(fileName));

    // Open file, always truncate
    mFile.open(QSaveFile::WriteOnly); // Write only implies truncate

    // Write overview
    mFile.write(overview.toUtf8());

    // Save and return status
    return mFile.commit();
}

//===============================================================================================================
// PlatformInterface
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformInterface::PlatformInterface(Install* const parent, const QString& platformTaglistPath, QString platformName,
                                     const QDir& overviewDir,const DocKey&) :
    Fe::PlatformDoc(parent, {}, platformName, {}),
    mPlatformTaglist(parent, platformTaglistPath, platformName),
    mOverviewWriter(overviewDir)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool PlatformInterface::isEmpty() const { return mPlatformTaglist.isEmpty(); };

bool PlatformInterface::containsGame(QUuid gameId) const
{
    /* Check main romlist for ID. Could check the taglist instead, which would be more "correct" since only the current
     * platform should contain the ID, but this doesn't matter given correct design and the lookup is performed via
     * the romlist's internal hash, which is faster than checking a list for the presence of the ID.
     */
    return static_cast<Install*>(parent())->mRomlist->containsGame(gameId);
}

bool PlatformInterface::containsAddApp(QUuid addAppId) const
{
    /* Check main romlist for ID. Could check the taglist instead, which would be more "correct" since only the current
     * platform should contain the ID, but this doesn't matter given correct design and the lookup is performed via
     * the romlist's internal hash, which is faster than checking a list for the presence of the ID.
     */
    return static_cast<Install*>(parent())->mRomlist->containsAddApp(addAppId);
};

void PlatformInterface::addSet(const Fp::Set& set, const Fe::ImageSources& images)
{
    if(!hasError())
    {
        //-Handle game----------------------------------------------------------
        const Fp::Game& game = set.game();

        // Add game ID to platform tag list
        mPlatformTaglist.appendTag(game.id().toString(QUuid::WithoutBraces));

        // Create game overview
        QString overview = game.originalDescription();

        if(!overview.isEmpty())
        {
            bool written = mOverviewWriter.writeOverview(game.id(), overview);

            if(written)
                parent()->addRevertableFile(mOverviewWriter.currentFilePath());
            else
                mError = Fe::DocHandlingError(*this, Fe::DocHandlingError::DocWriteFailed, mOverviewWriter.fileErrorString());
        }

        //-Handle add apps-------------------------------------------------------

        // Add add app IDs to platform tag list
        for(const Fp::AddApp& addApp : set.addApps())
        {
            /* Ignore non-playable add apps to avoid useless clutter in AM
             * TODO: Consider doing this in Import Worker to make it a standard since
             * LB doesn't actually need the non-playable entries either. Importing them
             * is basically a leftover from an earlier CLIFp version
             */
            if(addApp.isPlayable())
                mPlatformTaglist.appendTag(addApp.id().toString(QUuid::WithoutBraces));
        }

        //-Forward game insertion to main Romlist--------------------------------
        static_cast<Install*>(parent())->mRomlist->addSet(set, images);
    }
}

//===============================================================================================================
// PlatformInterface::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformInterface::Writer::Writer(PlatformInterface* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    Fe::PlatformDoc::Writer(sourceDoc),
    mTaglistWriter(&sourceDoc->mPlatformTaglist)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Fe::DocHandlingError PlatformInterface::Writer::writeOutOf() { return mTaglistWriter.writeOutOf(); }

//===============================================================================================================
// PlaylistInterface
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistInterface::PlaylistInterface(Install* const parent, const QString& playlistTaglistPath, QString playlistName,
                                     const DocKey&) :
    Fe::PlaylistDoc(parent, {}, playlistName, {}),
    mPlaylistTaglist(parent, playlistTaglistPath, playlistName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool PlaylistInterface::isEmpty() const { return mPlaylistTaglist.isEmpty(); };

bool PlaylistInterface::containsPlaylistGame(QUuid gameId) const
{
    return mPlaylistTaglist.containsTag(gameId.toString(QUuid::WithoutBraces));
}

void PlaylistInterface::setPlaylistData(const Fp::Playlist& playlist)
{
    for(const auto& pl : playlist.playlistGames())
        mPlaylistTaglist.appendTag(pl.gameId().toString(QUuid::WithoutBraces));
}

//===============================================================================================================
// PlaylistInterface::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistInterface::Writer::Writer(PlaylistInterface* sourceDoc) :
    Fe::DataDoc::Writer(sourceDoc),
    Fe::PlaylistDoc::Writer(sourceDoc),
    mTaglistWriter(&sourceDoc->mPlaylistTaglist)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Fe::DocHandlingError PlaylistInterface::Writer::writeOutOf() { return mTaglistWriter.writeOutOf(); }

//===============================================================================================================
// Emulator
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Emulator::Emulator(Install* const parent, const QString& filePath, const DocKey&) :
    ConfigDoc(parent, filePath, STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Emulator::isEmpty() const { return false; }; // Can have blank fields, but always has field keys
Fe::DataDoc::Type Emulator::type() const { return Fe::DataDoc::Type::Config; };

QString Emulator::executable() const { return mExecutable; }
QString Emulator::args() const { return mArgs; }
QString Emulator::workDir() const { return mWorkDir; }
QString Emulator::romPath() const { return mRomPath; }
QString Emulator::romExt() const { return mRomExt; }
QString Emulator::system() const { return mSystem; }
QString Emulator::infoSource() const { return mInfoSource; }
QString Emulator::exitHotkey() const { return mExitHotkey; }
EmulatorArtworkEntry Emulator::artworkEntry(QString type) const { return mArtworkEntries.value(type); }
QList<EmulatorArtworkEntry> Emulator::artworkEntries() const { return mArtworkEntries.values(); }

void Emulator::setExecutable(QString executable) { mExecutable = executable; }
void Emulator::setArgs(QString args) { mArgs = args; }
void Emulator::setWorkDir(QString workDir) { mWorkDir = workDir; }
void Emulator::setRomPath(QString romPath) { mRomPath = romPath; }
void Emulator::setRomExt(QString romExt) { mRomExt = romExt; }
void Emulator::setSystem(QString system) { mSystem = system; }
void Emulator::setInfoSource(QString infoSource) { mInfoSource = infoSource; }
void Emulator::setExitHotkey(QString exitHotkey) { mExitHotkey = exitHotkey; }
void Emulator::setArtworkEntry(EmulatorArtworkEntry entry) { mArtworkEntries[entry.type()] = entry; }

//===============================================================================================================
// Emulator::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
EmulatorReader::EmulatorReader(Emulator* targetDoc) :
    ConfigDoc::Reader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
Fe::DocHandlingError EmulatorReader::readTargetDoc()
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
    return Fe::DocHandlingError();
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

void EmulatorReader::parseExecutable(const QString& value) { targetEmulator()->setExecutable(value); }
void EmulatorReader::parseArgs(const QString& value) { targetEmulator()->setArgs(value); }
void EmulatorReader::parseWorkDir(const QString& value) { targetEmulator()->setWorkDir(value); }
void EmulatorReader::parseRomPath(const QString& value)
{
    targetEmulator()->setRomPath(value == R"("")" ? "" : value);
}
void EmulatorReader::parseRomExt(const QString& value)
{
    targetEmulator()->setRomPath(value == R"("")" ? "" : value);
}
void EmulatorReader::parseSystem(const QString& value) { targetEmulator()->setSystem(value); }
void EmulatorReader::parseInfoSource(const QString& value) { targetEmulator()->setInfoSource(value); }
void EmulatorReader::parseExitHotkey(const QString& value) { targetEmulator()->setExitHotkey(value); }
void EmulatorReader::parseArtwork(const QString& value)
{
    QString type;
    QString rawPaths;
    splitKeyValue(value, type, rawPaths);

    EmulatorArtworkEntry::Builder eaeb;
    eaeb.wType(type);
    eaeb.wPaths(!rawPaths.isEmpty() ? rawPaths.split(';') : QStringList());

    targetEmulator()->setArtworkEntry(eaeb.build());
}

Emulator* EmulatorReader::targetEmulator() { return static_cast<Emulator*>(mTargetDocument); }

//===============================================================================================================
// Emulator::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Emulator::Writer::Writer(Emulator* sourceDoc) :
    ConfigDoc::Writer(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool Emulator::Writer::writeConfigDoc()
{
    // Set field alignment
    mStreamWriter.setFieldAlignment(QTextStream::AlignLeft);

    // Write main key/values
    writeStandardKeyValue(Emulator::Keys::EXECUTABLE, sourceEmulator()->executable());
    writeStandardKeyValue(Emulator::Keys::ARGS, sourceEmulator()->args());
    writeStandardKeyValue(Emulator::Keys::WORK_DIR, sourceEmulator()->workDir());
    writeStandardKeyValue(Emulator::Keys::ROM_PATH, sourceEmulator()->romPath());
    writeStandardKeyValue(Emulator::Keys::ROM_EXT, sourceEmulator()->romExt());
    writeStandardKeyValue(Emulator::Keys::SYSTEM, sourceEmulator()->system());
    writeStandardKeyValue(Emulator::Keys::INFO_SOURCE, sourceEmulator()->infoSource());
    writeStandardKeyValue(Emulator::Keys::EXIT_HOTKEY, sourceEmulator()->exitHotkey());

    // Write artwork entries
    const QList<EmulatorArtworkEntry> artworkEntries = sourceEmulator()->artworkEntries();
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

Emulator* Emulator::Writer::sourceEmulator() { return static_cast<Emulator*>(mSourceDocument); }

}
