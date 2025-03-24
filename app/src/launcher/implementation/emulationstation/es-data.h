#ifndef EMULATIONSTATION_DATA_H
#define EMULATIONSTATION_DATA_H

// Qx Includes
#include <qx/io/qx-textstreamreader.h>
#include <qx/io/qx-textstreamwriter.h>

// Project Includes
#include "launcher/abstract/lr-data.h"
#include "launcher/implementation/emulationstation/es-registration.h"
#include "launcher/implementation/emulationstation/es-items.h"

namespace Es
{

class Gamelist : public Lr::PlatformDoc<LauncherId>
{
    friend class GamelistReader;
    friend class GamelistWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QString mSystemName;
    UpdatableContainer<Game> mGames;
    QList<Folder> mFolders;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Gamelist(Install* install, const QString& xmlPath, const QString& fullSystemName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    const Game* processSet(const Fp::Set& set) override;

public:
    bool isEmpty() const override;
    bool containsGame(const QUuid& gameId) const override;
    bool containsAddApp(const QUuid& addAppId) const override;

    QString fullSystemName() const;
    QString shortSystemName() const;
};

class GamelistReader : public Lr::XmlDocReader<Gamelist>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    GamelistReader(Gamelist* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parseGame();
    void parseFolder();
};

class GamelistWriter : public Lr::XmlDocWriter<Gamelist>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    GamelistWriter(Gamelist* sourceDoc);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeGame(const Game& game);
    bool writeFolder(const Folder& folder);
    bool updateDummyFiles();
};

class Collection : public Lr::PlaylistDoc<LauncherId>
{
    friend class CollectionReader;
    friend class CollectionWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    UpdatableContainer<CollectionEntry> mEntries;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Collection(Install* install, const QString& path, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    static inline const QString NAME_PREFIX = u"[FP]"_s;

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static QString originalNameToName(const QString& original);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    void setPlaylistData(const Fp::Playlist& playlist) override;
    bool containsPlaylistGame(const QUuid& gameId) const override;
    QString name() const;
};

class CollectionReader : public Lr::DataDocReader<Collection>
{
//-Class Variables--------------------------------------------------------------------------------------------------
private:
    static inline const QString ERR_INVALID_ENTRY = u"An entry did not follow the expected format"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Qx::TextStreamReader mReader;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    CollectionReader(Collection* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readInto() override;
};

class CollectionWriter : public Lr::DataDocWriter<Collection>
{
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    Qx::TextStreamWriter mWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    CollectionWriter(Collection* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError writeOutOf() override;
};

class Systemlist : public Lr::DataDoc<LauncherId>
{
    friend class SystemlistReader;
    friend class SystemlistWriter;
//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = u"Systemlist"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, System> mSystems; // Key'ed with full name

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Systemlist(Install* install, const QString& path);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    Type type() const override;

    const QHash<QString, System>& systems() const;

    void removeSystem(const QString& fullName);
    void insertSystem(const System& system);
};

class SystemlistReader : public Lr::XmlDocReader<Systemlist>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    SystemlistReader(Systemlist* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parseSystem();
};

class SystemlistWriter : public Lr::XmlDocWriter<Systemlist>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    SystemlistWriter(Systemlist* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeSystem(const System& system);
};

class Settings : public Lr::DataDoc<LauncherId>
{
    friend class SettingsReader;
    friend class SettingsWriter;
//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = u"Settings"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QString mRomDirectory;
    QStringList mCollectionSystemsCustom;
    QList<Setting> mOtherSettings;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Settings(Install* install, const QString& path);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    Type type() const override;

    QString romDirectory() const;
    bool containsCustomCollection(const QString& collectionName) const;
    void addCustomCollection(const QString& collectionName);
};

class SettingsReader : public Lr::XmlDocReader<Settings>
{
//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString ERR_INVALID = u"Invalid setting (type: %1 | name: %2 | value: %3)."_s;
    static inline const QString ERR_KNOWN_CONVERT = u"Failed to convert known setting (type: %1 | name: %2 | value: %3)."_s;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    SettingsReader(Settings* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    Lr::DocHandlingError readTargetDoc() override;
    void parseSetting();
};

class SettingsWriter : public Lr::XmlDocWriter<Settings>
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    SettingsWriter(Settings* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    bool writeSourceDoc() override;
    bool writeSetting(const Setting& setting);
};

}

#endif // EMULATIONSTATION_DATA_H
