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
    QHash<QUuid, std::shared_ptr<Game>> mGamesExisting;
    QHash<QUuid, std::shared_ptr<Game>> mGamesFinal;
    QList<Folder> mFolders;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Gamelist(Install* install, const QString& xmlPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    std::shared_ptr<Game> processSet(const Fp::Set& set) override;

public:
    bool isEmpty() const override;
    void finalize() override;
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
};

class Collection : public Lr::PlaylistDoc<LauncherId>
{
    friend class CollectionReader;
    friend class CollectionWriter;
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QSet<QString> mEntriesExisting;
    QSet<QString> mEntriesFinal;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Collection(Install* install, const QString& path, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    void finalize() override;
    void setPlaylistData(const Fp::Playlist& playlist) override;
};

class CollectionReader : public Lr::DataDocReader<Collection>
{
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
//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    QHash<QString, System> mSystems;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit Systemlist(Install* install, const QString& path, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    Type type() const override;
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

}

#endif // EMULATIONSTATION_DATA_H
