#ifndef FE_DATA
#define FE_DATA

#include <QFile>

#include "fe-items.h"
#include "../flashpoint/fp-items.h"

namespace Fe
{
//-Enums----------------------------------------------------------------------------------------------------------
enum ImportMode {OnlyNew, NewAndExisting};

//-Structs---------------------------------------------------------------------------------------------------------
struct UpdateOptions
{
    ImportMode importMode;
    bool removeObsolete;
};

//-Classes-----------------------------------------------------------------------------------------------------------
class DataDoc
{
    friend class Install;
//-Class Enums---------------------------------------------------------------------------------------------------------
protected:
    enum Type {Platform, Playlist, Config};
    enum StandardError {DocAlreadyOpen, DocCantOpen, NotParentDoc, CantRemoveBackup, CantCreateBackup, DocTypeMismatch, DocWriteFailed};

//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Identifier
    {
        friend bool operator== (const Identifier& lhs, const Identifier& rhs) noexcept;
        friend uint qHash(const Identifier& key, uint seed) noexcept;

    private:
        Type mDocType;
        QString mDocName;

    public:
        Identifier(Type docType, QString docName);

        Type docType() const;
        QString docTypeString() const;
        QString docName() const;
    };

    class Key
    {
        friend class Install;
    private:
        Key() {};
        Key(const Key&) = default;
    };

//-Class Variables-----------------------------------------------------------------------------------------------------
private:
    static inline const QHash<Type, const QString> TYPE_STRINGS = {
        {Platform, "Platform"},
        {Playlist, "Playlist"},
        {Config, "Config"}
    };

    // Message Macros
    static inline const QString M_DOC_TYPE = "<docType>";
    static inline const QString M_DOC_NAME = "<docName>";
    static inline const QString M_DOC_PARENT = "<docParent>";

    // Standard Errors
    static inline const QHash<StandardError, const QString> STD_ERRORS = {
        {DocAlreadyOpen, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is already open"},
        {DocCantOpen, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") cannot be opened"},
        {NotParentDoc, "The target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") is not a" + M_DOC_PARENT + "document."},
        {CantRemoveBackup, "The existing backup of the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") could not be removed."},
        {CantCreateBackup, "Could not create a backup of the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ")."},
        {DocTypeMismatch, "The document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") contained an element that belongs to a different document type than expected."},
        {DocWriteFailed, "Writing to the target document (" + M_DOC_TYPE + " | " + M_DOC_NAME + ") failed."}
    };

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    Install* const mParent;
    std::unique_ptr<QFile> mDocumentFile;
    QString mName;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDoc(Install* parent, std::unique_ptr<QFile> docFile, QString docName);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    virtual Type type() const = 0;
    QString filePath() const;

public:
    Install* parent() const;
    Identifier identifier() const;
    Qx::GenericError standardError(StandardError error, Qx::GenericError::ErrorLevel errorLevel = Qx::GenericError::Critical) const;

    void clearFile();
};

class DataDocReader
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mTargetDocument;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    DataDocReader(DataDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    virtual Qx::GenericError readInto() = 0;
};

class DataDocWriter
{
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    DataDoc* mSourceDocument;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    DataDocWriter(DataDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    virtual Qx::GenericError writeOutOf() = 0;
};


class PlatformDoc : public DataDoc
{
    friend class Install;

//-Class Variables-----------------------------------------------------------------------------------------------------
public:


//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    UpdateOptions mUpdateOptions;

    QHash<QUuid, std::shared_ptr<Game>> mGamesFinal;
    QHash<QUuid, std::shared_ptr<Game>> mGamesExisting;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsFinal;
    QHash<QUuid, std::shared_ptr<AddApp>> mAddAppsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlatformDoc(Install* parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions, const Key&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const;

protected:
    virtual std::shared_ptr<Game> prepareGame(const FP::Game& game) = 0;
    virtual std::shared_ptr<AddApp> prepareAddApp(const FP::AddApp& game) = 0;

    virtual void cleanup() = 0;

public:
    const QHash<QUuid, std::shared_ptr<Game>>& getFinalGames() const;
    const QHash<QUuid, std::shared_ptr<AddApp>>& getFinalAddApps() const;

    bool containsGame(QUuid gameID) const;
    bool containsAddApp(QUuid addAppId) const;

    void addGame(FP::Game game);
    void addAddApp(FP::AddApp app);

    void finalize();
};

class PlatformDocReader : public DataDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformDocReader(PlatformDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:

private:
};

class PlatformDocWriter : public DataDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlatformDocWriter(PlatformDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:

private:
};

class PlaylistDoc : public DataDoc
{
    friend class Install;

//-Class Variables-----------------------------------------------------------------------------------------------------
public:

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    UpdateOptions mUpdateOptions;

    std::shared_ptr<PlaylistHeader> mPlaylistHeader;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesFinal;
    QHash<QUuid, std::shared_ptr<PlaylistGame>> mPlaylistGamesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit PlaylistDoc(Install* parent, std::unique_ptr<QFile> docFile, QString docName, UpdateOptions updateOptions, const Key&);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    Type type() const;

protected:
    virtual std::shared_ptr<PlaylistHeader> preparePlaylistHeader(const FP::Playlist& playlist) = 0;
    virtual std::shared_ptr<PlaylistGame> preparePlaylistGame(const FP::PlaylistGame& game) = 0;

    virtual void cleanup();

public:
    const std::shared_ptr<PlaylistHeader>& getPlaylistHeader() const;
    const QHash<QUuid, std::shared_ptr<PlaylistGame>>& getFinalPlaylistGames() const;

    bool containsPlaylistGame(QUuid gameID) const;

    void setPlaylistHeader(FP::Playlist playlist);
    void addPlaylistGame(FP::PlaylistGame playlistGame);

    void finalize();
};

class PlaylistDocReader : public DataDocReader
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlaylistDocReader(PlaylistDoc* targetDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:

private:
};

class PlaylistDocWriter : public DataDocWriter
{
//-Constructor--------------------------------------------------------------------------------------------------------
public:
    PlaylistDocWriter(PlaylistDoc* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:

private:
};

//-Functions---------------------------------------------------------------------------------------------------------


}
#endif // FE_DATA
