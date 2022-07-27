#ifndef ATTRACTMODE_DATA_H
#define ATTRACTMODE_DATA_H

// Qx Includes
#include <qx/io/qx-textstreamreader.h>
#include <qx/io/qx-textstreamwriter.h>

// Project Includes
#include "am-items.h"
#include "../fe-data.h"
#include "../../clifp.h"

namespace Am
{

class DocKey
{
    friend class Install;
private:
    DocKey() {};
    DocKey(const DocKey&) = default;
};

class CommonDocReader : public Fe::DataDocReader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Qx::TextStreamReader mStreamReader;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    CommonDocReader(Fe::DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    bool lineIsComment(const QString& line);
    QString readLineIgnoringComments(qint64 maxlen = 0);
    virtual bool checkDocValidity(bool& isValid) = 0;
    virtual bool readTargetDoc() = 0;

public:
    Qx::GenericError readInto() override;
};

class CommonDocWriter : public Fe::DataDocWriter
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Qx::TextStreamWriter mStreamWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    CommonDocWriter(Fe::DataDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    virtual bool writeSourceDoc() = 0;

public:
    Qx::GenericError writeOutOf() override;
};

class ConfigDoc : public Fe::DataDoc
{
    friend class ConfigDocReader;
    friend class ConfigDocWriter;

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    static inline const QString TAGLINE = "# Generated by Attract-Mode v";

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    ConfigDoc(Install* const parent, const QString& filePath, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    QString versionedTagline();
};


class ConfigDocReader : public CommonDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    ConfigDocReader(ConfigDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    bool checkDocValidity(bool& isValid) override;
};

class ConfigDocWriter : public CommonDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    ConfigDocWriter(ConfigDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    virtual bool writeConfigDoc() = 0;
    bool writeSourceDoc() override;
};

class Taglist : public Fe::DataDoc
{
    friend class TaglistWriter;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QStringList mTags;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Taglist(Install* const parent, const QString& listPath, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;

    bool containsTag(QString tag) const;
    void appendTag(QString tag);
};

class TaglistWriter : public CommonDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    TaglistWriter(Taglist* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
};

class PlatformTaglist : public Taglist
{
    friend class PlatformInterface;
//-Constructor--------------------------------------------------------------------------------------------------------
private:
    PlatformTaglist(Install* const parent, const QString& listPath, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    Type type() const override;
};

class PlaylistTaglist : public Taglist
{
    friend class PlaylistInterface;
//-Constructor--------------------------------------------------------------------------------------------------------
private:
    PlaylistTaglist(Install* const parent, const QString& listPath, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    Type type() const override;
};

class Romlist : public Fe::PlatformDoc
{
    friend class RomlistReader;
    friend class RomlistWriter;

//-Class Variables----------------------------------------------------------------------------------------------------
private:
    static inline const QString HEADER = "#Name;Title;Emulator;CloneOf;Year;Manufacturer;Category;Players;Rotation;Control;Status;"
                                         "DisplayCount;DisplayType;AltRomname;AltTitle;Extra;Buttons;Series;Language;Region;Rating";

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QUuid, std::shared_ptr<RomEntry>> mEntriesExisting;
    QHash<QUuid, std::shared_ptr<RomEntry>> mEntriesFinal;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Romlist(Install* const parent, const QString& listPath, QString docName, Fe::UpdateOptions updateOptions,
                     const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;

    const QHash<QUuid, std::shared_ptr<RomEntry>>& finalEntries() const;

    bool containsGame(QUuid gameId) const override;
    bool containsAddApp(QUuid addAppId) const override;

    void addGame(const Fp::Game& game, const Fe::ImageSources& images) override;
    void addAddApp(const Fp::AddApp& app) override;

    void finalize() override;
};

class RomlistReader : public CommonDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    RomlistReader(Romlist* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    QHash<QUuid, std::shared_ptr<RomEntry>>& targetDocExistingRomEntries();
    bool checkDocValidity(bool& isValid) override;
    bool readTargetDoc() override;
    void parseRomEntry(const QString& rawEntry);
    void addFieldToBuilder(RomEntryBuilder& builder, QString field, quint8 index);
};

class RomlistWriter : public CommonDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    RomlistWriter(Romlist* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeRomEntry(const RomEntry& romEntry);
    void writeEntryField(const QString& entryField, bool writeSeperator = true);
};

class BulkOverviewWriter
{
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QDir mOverviewDir;
    QSaveFile mFile;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    BulkOverviewWriter(const QDir& overviewDir);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    QString currentFilePath();
    QString fileErrorString();
    bool writeOverview(const QUuid& gameId, const QString& overview);
};

class PlatformInterface : public Fe::PlatformDoc
{
    friend class PlatformInterfaceWriter;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    PlatformTaglist mPlatformTaglist;
    BulkOverviewWriter mOverviewWriter;
    /* NOTE: Would just use Qx::writeStringToFile() but that is slower due to lots of checks/error handling, whereas
     * this needs to be as fast as possible
     */

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformInterface(Install* const parent, const QString& platformTaglistPath, QString platformName,
                               const QDir& overviewDir, const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;

    bool containsGame(QUuid gameId) const override;
    bool containsAddApp(QUuid addAppId) const override;

    void addGame(const Fp::Game& game, const Fe::ImageSources& images) override;
    void addAddApp(const Fp::AddApp& app) override;
};

class PlatformInterfaceWriter : public Fe::PlatformDocWriter
{
    // Shell for writing the taglist of the interface

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    TaglistWriter mTaglistWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformInterfaceWriter(PlatformInterface* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    Qx::GenericError writeOutOf() override;
};

class PlaylistInterface : public Fe::PlaylistDoc
{
    friend class PlaylistInterfaceWriter;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    PlaylistTaglist mPlaylistTaglist;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlaylistInterface(Install* const parent, const QString& playlistTaglistPath, QString playlistName,
                               const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;

    bool containsPlaylistGame(QUuid gameId) const override;

    void setPlaylistHeader(const Fp::Playlist& playlist) override;
    void addPlaylistGame(const Fp::PlaylistGame& playlistGame) override;
};

class PlaylistInterfaceWriter : public Fe::PlaylistDocWriter
{
    // Shell for writing the taglist of the interface

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    TaglistWriter mTaglistWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlaylistInterfaceWriter(PlaylistInterface* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Qx::GenericError writeOutOf() override;
};

class CrudeMainConfig : public ConfigDoc
{
    friend class CrudeMainConfigReader;
    friend class CrudeMainConfigWriter;

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = "attract";

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QUuid, CrudeMainConfigEntry> mEntries;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit CrudeMainConfig(Install* const parent, const QString& filePath, const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    Type type() const override;

    bool containsEntry(QUuid entryId);
    bool containsEntry(QString type, QString name);
    bool containsEntryWithContent(QString type, const QString& partialContent);
    void addEntry(const CrudeMainConfigEntry& entry);
};

class CrudeMainConfigReader : public ConfigDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    CrudeMainConfigReader(CrudeMainConfig* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    QHash<QUuid, CrudeMainConfigEntry>& targetDocEntries();
    bool readTargetDoc() override;

    void parseConfigEntry(QStringList rawEntry);
};

class CrudeMainConfigWriter : public ConfigDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    CrudeMainConfigWriter(CrudeMainConfig* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeConfigDoc() override;
    bool writeConfigEntry(const CrudeMainConfigEntry& configEntry);
};

class Emulator : public ConfigDoc
{
    friend class EmulatorReader;
    friend class EmulatorWriter;

//-Inner Classes-------------------------------------------------------------------------------------------------------
public:
    class Keys
    {
    public:
        static inline const QString EXECUTABLE = "executable";
        static inline const QString ARGS = "args";
        static inline const QString WORK_DIR = "workdir";
        static inline const QString ROM_PATH = "rompath";
        static inline const QString ROM_EXT = "romext";
        static inline const QString SYSTEM = "system";
        static inline const QString EXIT_HOTKEY = "exit_hotkey";

        class Artwork
        {
        public:
            static inline const QString NAME = "artwork";

            static inline const QString FLYER = "flyer";
            static inline const QString MARQUEE = "marquee";
            static inline const QString SNAP = "snap";
            static inline const QString WHEEL = "wheel";
        };
    };

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = CLIFp::NAME;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QString mExecutable;
    QString mArgs;
    QString mWorkDir;
    QString mRomPath;
    QString mRomExt;
    QString mSystem;
    QString mExitHotkey;
    QHash<QString, EmulatorArtworkEntry> mArtworkEntries;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Emulator(Install* const parent, const QString& filePath, const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    Type type() const override;

    QString executable() const;
    QString args() const;
    QString workDir() const;
    QString romPath() const;
    QString romExt() const;
    QString system() const;
    QString exitHotkey() const;
    EmulatorArtworkEntry artworkEntry(QString type) const;
    QList<EmulatorArtworkEntry> artworkEntries() const;

    void setExecutable(QString executable);
    void setArgs(QString args);
    void setWorkDir(QString workDir);
    void setRomPath(QString romPath);
    void setRomExt(QString romExt);
    void setSystem(QString system);
    void setExitHotkey(QString exitHotkey);
    void setArtworkEntry(EmulatorArtworkEntry entry);
};

class EmulatorReader : public ConfigDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    EmulatorReader(Emulator* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool readTargetDoc() override;
    void parseKeyValue(const QString& key, const QString& value);
    void parseExecutable(const QString& value);
    void parseArgs(const QString& value);
    void parseWorkDir(const QString& value);
    void parseRomPath(const QString& value);
    void parseRomExt(const QString& value);
    void parseSystem(const QString& value);
    void parseExitHotkey(const QString& value);
    void parseArtwork(const QString& value);

    Emulator* targetEmulator(); // TODO: Example of what isn't needed if readers/writers are made into templates
};

class EmulatorWriter : public ConfigDocWriter
{
//-Class Values-------------------------------------------------------------------------------------------------------
private:
    static const int STD_KEY_FIELD_WIDTH = 21;
    static const int ARTWORK_KEY_FIELD_WIDTH = 11;
    static const int ARTWORK_TYPE_FIELD_WIDTH = 16;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    EmulatorWriter(Emulator* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeConfigDoc() override;
    void writeStandardKeyValue(const QString& key, const QString& value);
    void writeArtworkEntry(const EmulatorArtworkEntry& entry);

    Emulator* sourceEmulator(); // TODO: Example of what isn't needed if readers/writers are made into templates
};

}

#endif // ATTRACTMODE_DATA_H
