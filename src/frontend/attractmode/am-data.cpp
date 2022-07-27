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
    Fe::DataDocReader(targetDoc),
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
Qx::GenericError CommonDocReader::readInto()
{
    // Error template
    Qx::GenericError error(Qx::GenericError::Critical, mStdReadErrorStr);

    // Open file
    Qx::IoOpReport openError =  mStreamReader.openFile();
    if(openError.isFailure())
        return error.setSecondaryInfo(openError.outcomeInfo());

    // Check that doc is valid
    bool isValid = false;
    if(!checkDocValidity(isValid))
        return error.setSecondaryInfo(mStreamReader.status().outcomeInfo());
    else if(!isValid)
    {
        QString errReason = Fe::docHandlingErrorString(mTargetDocument, Fe::DocHandlingError::DocInvalidType);
        return error.setSecondaryInfo(errReason);
    }

    // Read doc
    bool readSuccessful = readTargetDoc();

    // Close file
    mStreamReader.closeFile();

    // Return outcome
    return readSuccessful ? Qx::GenericError() : error.setSecondaryInfo(mStreamReader.status().outcomeInfo());
}

//===============================================================================================================
// CommonDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
CommonDocWriter::CommonDocWriter(Fe::DataDoc* sourceDoc) :
    Fe::DataDocWriter(sourceDoc),
    mStreamWriter(sourceDoc->path(), Qx::WriteMode::Truncate)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError CommonDocWriter::writeOutOf()
{
    // Error template
    Qx::GenericError error(Qx::GenericError::Critical, mStdWriteErrorStr);

    // Open file
    Qx::IoOpReport openError =  mStreamWriter.openFile();
    if(openError.isFailure())
        return error.setSecondaryInfo(openError.outcomeInfo());

    // Write doc
    bool writeSuccess = writeSourceDoc();

    // Close file
    mStreamWriter.closeFile();

    // Return outcome
    return writeSuccess ? Qx::GenericError() : error.setSecondaryInfo(mStreamWriter.status().outcomeInfo());
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
// ConfigDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
ConfigDocReader::ConfigDocReader(ConfigDoc* targetDoc) :
    CommonDocReader(targetDoc)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
bool ConfigDocReader::checkDocValidity(bool& isValid)
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
// ConfigDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ConfigDocWriter::ConfigDocWriter(ConfigDoc* sourceDoc) :
    CommonDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool ConfigDocWriter::writeSourceDoc()
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
// TaglistWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
TaglistWriter::TaglistWriter(Taglist* sourceDoc) :
    CommonDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool TaglistWriter::writeSourceDoc()
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
    Fe::PlatformDoc(parent, listPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Romlist::isEmpty() const
{
    return mEntriesExisting.isEmpty() && mEntriesFinal.isEmpty();
}

const QHash<QUuid, std::shared_ptr<RomEntry>>& Romlist::finalEntries() const { return mEntriesFinal; }

bool Romlist::containsGame(QUuid gameId) const { return mEntriesExisting.contains(gameId) || mEntriesFinal.contains(gameId); }
bool Romlist::containsAddApp(QUuid addAppId) const { return mEntriesExisting.contains(addAppId) || mEntriesFinal.contains(addAppId); };

void Romlist::addGame(const Fp::Game& game, const Fe::ImageSources& images)
{
    // Convert to romlist entry
    std::shared_ptr<RomEntry> romEntry = std::make_shared<RomEntry>(game);

    // Add entry
    addUpdateableItem(mEntriesExisting, mEntriesFinal, romEntry);

    // Allow install to process images as necessary
    parent()->processDirectGameImages(romEntry.get(), images);
}

void Romlist::addAddApp(const Fp::AddApp& app)
{
    // Convert to romlist entry
    std::shared_ptr<RomEntry> romEntry = std::make_shared<RomEntry>(app);

    // Add entry
    addUpdateableItem(mEntriesExisting, mEntriesFinal, romEntry);
}

void Romlist::finalize()
{
    finalizeUpdateableItems(mEntriesExisting, mEntriesFinal);
    Fe::PlatformDoc::finalize();
}

//===============================================================================================================
// RomlistReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
RomlistReader::RomlistReader(Romlist* targetDoc) :
    CommonDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
QHash<QUuid, std::shared_ptr<RomEntry>>& RomlistReader::targetDocExistingRomEntries()
{
    return static_cast<Romlist*>(mTargetDocument)->mEntriesExisting;
}

bool RomlistReader::checkDocValidity(bool& isValid)
{
    // See if first line is the romlist header
    QString header = mStreamReader.readLine();
    isValid = header == Romlist::HEADER;

    // Return status
    return !mStreamReader.hasError();
}

bool RomlistReader::readTargetDoc()
{
    // Read all romlist entries
    while(!mStreamReader.atEnd())
    {
        QString rawEntry = readLineIgnoringComments();
        parseRomEntry(rawEntry);
    }

    // Return stream status
    return !mStreamReader.hasError();
}

void RomlistReader::parseRomEntry(const QString& rawEntry)
{
    // Prepare builder and iterator
    RomEntryBuilder reb;
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

void RomlistReader::addFieldToBuilder(RomEntryBuilder& builder, QString field, quint8 index)
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
// RomlistWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
RomlistWriter::RomlistWriter(Romlist* sourceDoc) :
    CommonDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool RomlistWriter::writeSourceDoc()
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

bool RomlistWriter::writeRomEntry(const RomEntry& romEntry)
{
    writeEntryField(romEntry.name().toString(QUuid::WithoutBraces));
    writeEntryField(romEntry.title());
    writeEntryField(romEntry.emulator());
    writeEntryField(romEntry.cloneOf());
    writeEntryField(romEntry.year().toString());
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

void RomlistWriter::writeEntryField(const QString& entryField, bool writeSeperator)
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

void PlatformInterface::addGame(const Fp::Game& game, const Fe::ImageSources& images)
{
    if(!hasError())
    {
        // Add game ID to platform tag list
        mPlatformTaglist.appendTag(game.id().toString(QUuid::WithoutBraces));

        // Forward game insertion to main Romlist
        static_cast<Install*>(parent())->mRomlist->addGame(game, images);

        // Create game overview
        QString overview = game.originalDescription();

        if(!overview.isEmpty())
        {
            bool written = mOverviewWriter.writeOverview(game.id(), overview);

            if(written)
                parent()->addRevertableFile(mOverviewWriter.currentFilePath());
            else
            {
                mError = Qx::GenericError(Qx::GenericError::Critical,
                                          Fe::docHandlingErrorString(this, Fe::DocHandlingError::DocWriteFailed),
                                          mOverviewWriter.fileErrorString());
            }
        }
    }
}

void PlatformInterface::addAddApp(const Fp::AddApp& app)
{
    if(!hasError())
    {
        // Add add app ID to platform tag list
        mPlatformTaglist.appendTag(app.id().toString(QUuid::WithoutBraces));

        /* Forward add app insertion to main Romlist
         *
         * Only include "playable" add apps since the others aren't needed and just clutter the UI
         */
        if(app.isPlayable())
            static_cast<Install*>(parent())->mRomlist->addAddApp(app);
    }
}

//===============================================================================================================
// PlatformInterfaceWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlatformInterfaceWriter::PlatformInterfaceWriter(PlatformInterface* sourceDoc) :
    Fe::DataDocWriter(sourceDoc),
    Fe::PlatformDocWriter(sourceDoc),
    mTaglistWriter(&sourceDoc->mPlatformTaglist)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError PlatformInterfaceWriter::writeOutOf() { return mTaglistWriter.writeOutOf(); }

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

void PlaylistInterface::setPlaylistHeader(const Fp::Playlist& playlist) {} // No playlist header for AttractMode

void PlaylistInterface::addPlaylistGame(const Fp::PlaylistGame& playlistGame)
{
    mPlaylistTaglist.appendTag(playlistGame.gameId().toString(QUuid::WithoutBraces));
};

//===============================================================================================================
// PlaylistInterfaceWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
PlaylistInterfaceWriter::PlaylistInterfaceWriter(PlaylistInterface* sourceDoc) :
    Fe::DataDocWriter(sourceDoc),
    Fe::PlaylistDocWriter(sourceDoc),
    mTaglistWriter(&sourceDoc->mPlaylistTaglist)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError PlaylistInterfaceWriter::writeOutOf() { return mTaglistWriter.writeOutOf(); }

//===============================================================================================================
// CrudeMainConfig
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CrudeMainConfig::CrudeMainConfig(Install* const parent, const QString& filePath, const DocKey&) :
    ConfigDoc(parent, filePath, STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool CrudeMainConfig::isEmpty() const { return mEntries.isEmpty(); };

Fe::DataDoc::Type CrudeMainConfig::type() const { return Type::Config; }

bool CrudeMainConfig::containsEntry(QUuid entryId) { return mEntries.contains(entryId); }
bool CrudeMainConfig::containsEntry(QString type, QString name) { return containsEntry(CrudeMainConfigEntry::equivalentId(type, name)); }

bool CrudeMainConfig::containsEntryWithContent(QString type, const QString& partialContent)
{
    for(auto itr = mEntries.constBegin(); itr != mEntries.constEnd(); itr++)
    {
        const CrudeMainConfigEntry& entry = itr.value();
        if(entry.type() == type && entry.contents().contains(partialContent))
            return true;
    }

    // No hit
    return false;
}

void CrudeMainConfig::addEntry(const CrudeMainConfigEntry& entry) { mEntries.insert(entry.id(), entry); }

//===============================================================================================================
// CrudeMainConfigReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CrudeMainConfigReader::CrudeMainConfigReader(CrudeMainConfig* targetDoc) :
    ConfigDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
QMap<QUuid, CrudeMainConfigEntry>& CrudeMainConfigReader::targetDocEntries()
{
    return static_cast<CrudeMainConfig*>(mTargetDocument)->mEntries;
}

bool CrudeMainConfigReader::readTargetDoc()
{
    // Read all config entries
    QStringList currentEntry;

    while(!mStreamReader.atEnd())
    {
        QString line = readLineIgnoringComments();
        if(line.isEmpty() && !currentEntry.isEmpty())
        {
            parseConfigEntry(currentEntry);
            currentEntry.clear();
        }
        else
            currentEntry.append(line);
    }

    // Return stream status
    return !mStreamReader.hasError();
}

void CrudeMainConfigReader::parseConfigEntry(QStringList rawEntry)
{
    // Prepare builder
    CrudeMainConfigEntryBuilder cceb;

    // Identify entry
    QStringList identifiers = rawEntry.first().split(' ');
    QString type = identifiers.first();
    QString name = identifiers.count() > 1 ? identifiers.at(1) : QString();

    // Remove identifier line
    rawEntry.removeFirst();

    // Draft entry
    cceb.wTypeAndName(type, name);
    cceb.wContents(rawEntry);

    // Build Entry and add to document
    CrudeMainConfigEntry existingEntry = cceb.build();
    targetDocEntries()[existingEntry.id()] = existingEntry;
}

//===============================================================================================================
// CrudeMainConfigWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CrudeMainConfigWriter::CrudeMainConfigWriter(CrudeMainConfig* sourceDoc) :
    ConfigDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool CrudeMainConfigWriter::writeConfigDoc()
{
    // Write all config entries
    for(const CrudeMainConfigEntry& entry : qAsConst(static_cast<CrudeMainConfig*>(mSourceDocument)->mEntries))
    {
        if(!writeConfigEntry(entry))
            return false;
    }

    // Return true on success
    return true;
}

bool CrudeMainConfigWriter::writeConfigEntry(const CrudeMainConfigEntry& configEntry)
{
    // Write identifier
    mStreamWriter << configEntry.type() << " " << configEntry.name() << "\n";

    // Write contents
    const QStringList contents = configEntry.contents();
    for(const QString& line : contents)
         mStreamWriter << line << '\n';

    // Write spacer after entry
    mStreamWriter << '\n';

    // Return status
    return !mStreamWriter.hasError();
}

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
// EmulatorReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
EmulatorReader::EmulatorReader(Emulator* targetDoc) :
    ConfigDocReader(targetDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool EmulatorReader::readTargetDoc()
{
    while(!mStreamReader.atEnd())
    {
        QString line = mStreamReader.readLine();
        int firstSpace = line.indexOf(' ');
        QString key = firstSpace != -1 ? line.first(firstSpace) : line;
        QString value = firstSpace != -1 ? Qx::String::trimLeading(line.mid(firstSpace)) : "";
        parseKeyValue(key, value);
    }

    // Return stream status
    return !mStreamReader.hasError();
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
    else if(key == Emulator::Keys::Artwork::NAME)
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
    int firstSpace = value.indexOf(' ');

    if(firstSpace != -1)
    {
        type = value.first(firstSpace);
        rawPaths = Qx::String::trimLeading(value.mid(firstSpace));
    }
    else
        type = value;

    EmulatorArtworkEntryBuilder eaeb;
    eaeb.wType(type);
    eaeb.wPaths(!rawPaths.isEmpty() ? rawPaths.split(';') : QStringList());

    targetEmulator()->setArtworkEntry(eaeb.build());
}

Emulator* EmulatorReader::targetEmulator() { return static_cast<Emulator*>(mTargetDocument); }

//===============================================================================================================
// EmulatorWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
EmulatorWriter::EmulatorWriter(Emulator* sourceDoc) :
    ConfigDocWriter(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
bool EmulatorWriter::writeConfigDoc()
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

void EmulatorWriter::writeStandardKeyValue(const QString& key, const QString& value)
{
    mStreamWriter.setFieldWidth(STD_KEY_FIELD_WIDTH);
    mStreamWriter << key;
    mStreamWriter.setFieldWidth(0);
    mStreamWriter << value << '\n';
}

void EmulatorWriter::writeArtworkEntry(const EmulatorArtworkEntry& entry)
{
    mStreamWriter.setFieldWidth(ARTWORK_KEY_FIELD_WIDTH);
    mStreamWriter << Emulator::Keys::Artwork::NAME;
    mStreamWriter.setFieldWidth(ARTWORK_TYPE_FIELD_WIDTH);
    mStreamWriter << entry.type();
    mStreamWriter.setFieldWidth(0);
    mStreamWriter << entry.paths().join(';');
    mStreamWriter << '\n';
}

Emulator* EmulatorWriter::sourceEmulator() { return static_cast<Emulator*>(mSourceDocument); }

}
