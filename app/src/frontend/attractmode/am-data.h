#ifndef ATTRACTMODE_DATA_H
#define ATTRACTMODE_DATA_H

// Qx Includes
#include <qx/io/qx-textstreamreader.h>
#include <qx/io/qx-textstreamwriter.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "am-items.h"
#include "../fe-data.h"

namespace Am
{

class DocKey
{
    friend class Install;
private:
    DocKey() {};
    DocKey(const DocKey&) = default;
};

class CommonDocReader : public Fe::DataDoc::Reader
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
    virtual Fe::DocHandlingError readTargetDoc() = 0;

public:
    Fe::DocHandlingError readInto() override;
};

class CommonDocWriter : public Fe::DataDoc::Writer
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
    Fe::DocHandlingError writeOutOf() override;
};

class ConfigDoc : public Fe::DataDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Class Variables--------------------------------------------------------------------------------------------------
public:
    static inline const QString TAGLINE = u"# Generated by Attract-Mode v"_s;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    ConfigDoc(Install* const parent, const QString& filePath, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    QString versionedTagline();
};


class ConfigDoc::Reader : public CommonDocReader
{
//-Class Variables----------------------------------------------------------------------------------------------------
protected:
    static inline const QRegularExpression KEY_VALUE_REGEX =
            QRegularExpression(R"((?<key>\w+)(?!\S)[^\S\r\n]*(?<value>(?:\S+(?:[^\S\r\n]+\S+)*)*))");

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Reader(ConfigDoc* targetDoc);

//-Class Functions-------------------------------------------------------------------------------------------------
protected:
    bool splitKeyValue(const QString& line, QString& key, QString& value);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    bool checkDocValidity(bool& isValid) override;
};

class ConfigDoc::Writer : public CommonDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    Writer(ConfigDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    virtual bool writeConfigDoc() = 0;
    bool writeSourceDoc() override;
};

class Taglist : public Fe::DataDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Writer;

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

class Taglist::Writer : public CommonDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(Taglist* sourceDoc);

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

class Romlist : public Fe::UpdateableDoc
{
    /* This class looks like it should inherit PlatformDoc, but it isn't truly one in the context of an Am install
     * since those are represented by tag lists, and if it did there would be the issue that once modified it would
     * be added into the modified platforms list, which isn't accurate given said context. It's more of a config
     * doc since its usage is internal to Am::Install
     */

//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Class Variables----------------------------------------------------------------------------------------------------
private:
    static inline const QString HEADER = u"#Name;Title;Emulator;CloneOf;Year;Manufacturer;Category;Players;Rotation;Control;Status;"_s
                                         u"DisplayCount;DisplayType;AltRomname;AltTitle;Extra;Buttons;Series;Language;Region;Rating"_s;

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
    DataDoc::Type type() const override;
    bool isEmpty() const override;

    const QHash<QUuid, std::shared_ptr<RomEntry>>& finalEntries() const;

    bool containsGame(QUuid gameId) const;
    bool containsAddApp(QUuid addAppId) const;

    void addSet(const Fp::Set& set, const Fe::ImageSources& images);

    void finalize() override;
};

class Romlist::Reader : public CommonDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Reader(Romlist* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    QHash<QUuid, std::shared_ptr<RomEntry>>& targetDocExistingRomEntries();
    bool checkDocValidity(bool& isValid) override;
    Fe::DocHandlingError readTargetDoc() override;
    void parseRomEntry(const QString& rawEntry);
    void addFieldToBuilder(RomEntry::Builder& builder, QString field, quint8 index);
};

class Romlist::Writer : public CommonDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(Romlist* sourceDoc);

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
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Writer;

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

    void addSet(const Fp::Set& set, const Fe::ImageSources& images) override;
};

class PlatformInterface::Writer : public Fe::PlatformDoc::Writer
{
    // Shell for writing the taglist of the interface

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Taglist::Writer mTaglistWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(PlatformInterface* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    Fe::DocHandlingError writeOutOf() override;
};

class PlaylistInterface : public Fe::PlaylistDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Writer;

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

    void setPlaylistData(const Fp::Playlist& playlist) override;
};

class PlaylistInterface::Writer : public Fe::PlaylistDoc::Writer
{
    // Shell for writing the taglist of the interface

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Taglist::Writer mTaglistWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(PlaylistInterface* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Fe::DocHandlingError writeOutOf() override;
};

class Emulator : public ConfigDoc
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Reader;
    class Writer;

//-Inner Classes-------------------------------------------------------------------------------------------------------
public:
    class Keys
    {
    public:
        static inline const QString EXECUTABLE = u"executable"_s;
        static inline const QString ARGS = u"args"_s;
        static inline const QString WORK_DIR = u"workdir"_s;
        static inline const QString ROM_PATH = u"rompath"_s;
        static inline const QString ROM_EXT = u"romext"_s;
        static inline const QString SYSTEM = u"system"_s;
        static inline const QString INFO_SOURCE = u"info_source"_s;
        static inline const QString EXIT_HOTKEY = u"exit_hotkey"_s;

        static inline const QString ARTWORK = u"artwork"_s;
        class Artwork
        {
        public:
            static inline const QString FLYER = u"flyer"_s;
            static inline const QString MARQUEE = u"marquee"_s;
            static inline const QString SNAP = u"snap"_s;
            static inline const QString WHEEL = u"wheel"_s;
        };
    };

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = Fp::NAME;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QString mExecutable;
    QString mArgs;
    QString mWorkDir;
    QString mRomPath;
    QString mRomExt;
    QString mSystem;
    QString mInfoSource;
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
    QString infoSource() const;
    QString exitHotkey() const;
    EmulatorArtworkEntry artworkEntry(QString type) const;
    QList<EmulatorArtworkEntry> artworkEntries() const;

    void setExecutable(QString executable);
    void setArgs(QString args);
    void setWorkDir(QString workDir);
    void setRomPath(QString romPath);
    void setRomExt(QString romExt);
    void setSystem(QString system);
    void setInfoSource(QString infoSource);
    void setExitHotkey(QString exitHotkey);
    void setArtworkEntry(EmulatorArtworkEntry entry);
};

class EmulatorReader : public ConfigDoc::Reader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    EmulatorReader(Emulator* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Fe::DocHandlingError readTargetDoc() override;
    void parseKeyValue(const QString& key, const QString& value);
    void parseExecutable(const QString& value);
    void parseArgs(const QString& value);
    void parseWorkDir(const QString& value);
    void parseRomPath(const QString& value);
    void parseRomExt(const QString& value);
    void parseSystem(const QString& value);
    void parseInfoSource(const QString& value);
    void parseExitHotkey(const QString& value);
    void parseArtwork(const QString& value);

    Emulator* targetEmulator(); // TODO: Example of what isn't needed if readers/writers are made into templates
};

class Emulator::Writer : public ConfigDoc::Writer
{
//-Class Values-------------------------------------------------------------------------------------------------------
private:
    static const int STD_KEY_FIELD_WIDTH = 21;
    static const int ARTWORK_KEY_FIELD_WIDTH = 11;
    static const int ARTWORK_TYPE_FIELD_WIDTH = 16;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Writer(Emulator* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeConfigDoc() override;
    void writeStandardKeyValue(const QString& key, const QString& value);
    void writeArtworkEntry(const EmulatorArtworkEntry& entry);

    Emulator* sourceEmulator(); // TODO: Example of what isn't needed if readers/writers are made into templates
};

}

#endif // ATTRACTMODE_DATA_H
