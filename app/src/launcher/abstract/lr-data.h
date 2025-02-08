#ifndef LR_DATA_H
#define LR_DATA_H

// Project Includes
#include "launcher/interface/lr-data-interface.h"
#include "launcher/abstract/lr-registration.h"

/* NOTE: These classes are convenience versions of the ones in the 'interface' folder that are templated
 * so that some of the types involved will be the launcher specific versions instead of a generic base
 * pointer (meaning less manual casting is required while using them), and so that a few of their methods
 * are automatically implemented according to the types at play.
 *
 * Generally, these are the classes that should be derived form to add a frontend
 */

namespace Lr
{

/* We just repeat the templated 'parent' methods here in order to avoid needing virtual inheritance for IDataDoc if
 * only DataDoc had it and then all derived templates types derived from DataDoc as well as the specific derived
 * interface type. Maybe its not worth the clutter, but doing it this way for now.
 */


/* This was going to have another parameter for Type (doc type), but it created trouble when using
 * the readers/writers and dealing with diamond inheritance where one of the branches already defined
 * type().
 */
template<LauncherId Id>
class DataDoc : public IDataDoc
{
protected:
    using InstallT = Id::InstallT;
//-Constructor------------------------------------------------------------------------------------------------------
protected:
    DataDoc(InstallT* install, const QString& docPath, QString docName);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~DataDoc() = default;

//-Instance Functions-----------------------------------------------------------------------------------------------
public:
    InstallT* install() const;

    // IMPLEMENT
    using IDataDoc::type;
    using IDataDoc::isEmpty;
};

template<class DocT>
class DataDocReader : public IDataDoc::Reader
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDocReader(DocT* targetDoc);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~DataDocReader()  = default;

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    DocT* target() const;

public:
    // IMPLEMENT
    using IDataDoc::Reader::readInto;
};

template<class DocT>
class DataDocWriter : public IDataDoc::Writer
{
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    DataDocWriter(DocT* sourceDoc);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~DataDocWriter() = default;

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    DocT* source() const;

public:
    // IMPLEMENT
    using IDataDoc::Writer::writeOutOf;
};

template<LauncherId Id>
class UpdateableDoc : public IUpdateableDoc
{
protected:
    using InstallT = Id::InstallT;
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit UpdateableDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    InstallT* install() const;

    // IMPLEMENT
    using IUpdateableDoc::isEmpty;

    // OPTIONALLY RE-IMPELEMENT
    using IUpdateableDoc::finalize;
};

template<LauncherId Id>
class PlatformDoc : public IPlatformDoc
{
protected:
    using InstallT = Id::InstallT;
    using GameT = Id::GameT;
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlatformDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    // IMPLEMENT
    virtual std::shared_ptr<GameT> processSet(const Fp::Set& set) = 0;

public:
    InstallT* install() const;

    void addSet(const Fp::Set& set, Import::ImagePaths& images) override;

    // IMPLEMENT
    using IPlatformDoc::isEmpty;
    using IPlatformDoc::containsGame; // NOTE: UNUSED
    using IPlatformDoc::containsAddApp; // NOTE: UNUSED

    // OPTIONALLY RE-IMPELEMENT
    using IPlatformDoc::finalize;
};

template<LauncherId Id>
class PlaylistDoc : public IPlaylistDoc
{
protected:
    using InstallT = Id::InstallT;
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlaylistDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    InstallT* install() const;

    // IMPLEMENT
    using IPlaylistDoc::isEmpty;
    using IPlaylistDoc::containsPlaylistGame; // NOTE: UNUSED
    using IPlaylistDoc::setPlaylistData;

    // OPTIONALLY RE-IMPELEMENT
    using IPlaylistDoc::finalize;
};


template<LauncherId Id>
class BasicPlatformDoc : public PlatformDoc<Id>
{
protected:
    using InstallT = Id::InstallT;
    using GameT = Id::GameT;
    using AddAppT = Id::AddAppT;
    using IErrorable::mError;
    using IUpdateableDoc::finalizeUpdateableItems;
    using IUpdateableDoc::addUpdateableItem;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QHash<QUuid, std::shared_ptr<GameT>> mGamesFinal;
    QHash<QUuid, std::shared_ptr<GameT>> mGamesExisting;
    QHash<QUuid, std::shared_ptr<AddAppT>> mAddAppsFinal;
    QHash<QUuid, std::shared_ptr<AddAppT>> mAddAppsExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlatformDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    // IMPLEMENT
    virtual std::shared_ptr<GameT> prepareGame(const Fp::Game& game) = 0;
    virtual std::shared_ptr<AddAppT> prepareAddApp(const Fp::AddApp& game) = 0;

public:
    InstallT* install() const;

    const QHash<QUuid, std::shared_ptr<GameT>>& finalGames() const;
    const QHash<QUuid, std::shared_ptr<AddAppT>>& finalAddApps() const;
    bool containsGame(QUuid gameId) const override; // NOTE: UNUSED
    bool containsAddApp(QUuid addAppId) const override; // NOTE: UNUSED

    std::shared_ptr<GameT> processSet(const Fp::Set& set) override;
    void finalize() override;

    // OPTIONALLY RE-IMPELEMENT
    virtual bool isEmpty() const override;
};

template<LauncherId Id>
class BasicPlaylistDoc : public PlaylistDoc<Id>
{
protected:
    using InstallT = Id::InstallT;
    using PlaylistHeaderT = Id::PlaylistHeaderT;
    using PlaylistGameT = Id::PlaylistGameT;
    using IErrorable::mError;
    using IUpdateableDoc::finalizeUpdateableItems;
    using IUpdateableDoc::addUpdateableItem;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    std::shared_ptr<PlaylistHeaderT> mPlaylistHeader;
    QHash<QUuid, std::shared_ptr<PlaylistGameT>> mPlaylistGamesFinal;
    QHash<QUuid, std::shared_ptr<PlaylistGameT>> mPlaylistGamesExisting;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlaylistDoc(InstallT* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    // IMPLEMENT
    virtual std::shared_ptr<PlaylistHeaderT> preparePlaylistHeader(const Fp::Playlist& playlist) = 0;
    virtual std::shared_ptr<PlaylistGameT> preparePlaylistGame(const Fp::PlaylistGame& game) = 0;

public:
    InstallT* install() const;

    const std::shared_ptr<PlaylistHeaderT>& playlistHeader() const;
    const QHash<QUuid, std::shared_ptr<PlaylistGameT>>& finalPlaylistGames() const;
    bool containsPlaylistGame(QUuid gameId) const override;
    void setPlaylistData(const Fp::Playlist& playlist) override;
    void finalize() override;

    // OPTIONALLY RE-IMPELEMENT
    virtual bool isEmpty() const override;
};

template<class DocT>
class XmlDocReader : public DataDocReader<DocT>
{
protected:
    using DataDocReader<DocT>::target;
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QFile mXmlFile;
    QXmlStreamReader mStreamReader;
    QString mRootElement;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    XmlDocReader(DocT* targetDoc, const QString& root);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    // IMPLEMENT
    virtual DocHandlingError readTargetDoc() = 0;

protected:
    DocHandlingError streamStatus() const;

public:
    DocHandlingError readInto() override;
};

template<class DocT>
class XmlDocWriter : public DataDocWriter<DocT>
{
protected:
    using DataDocWriter<DocT>::source;
//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    QFile mXmlFile;
    QXmlStreamWriter mStreamWriter;
    QString mRootElement;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    XmlDocWriter(DocT* sourceDoc, const QString& root);

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    void writeCleanTextElement(const QString& qualifiedName, const QString& text);
    void writeOtherFields(const QHash<QString, QString>& otherFields);
    DocHandlingError streamStatus() const;

    // IMPLEMENT
    virtual bool writeSourceDoc() = 0;

public:
    DocHandlingError writeOutOf() override;
};

}

#include "lr-data.tpp"
#endif // LR_DATA_H
