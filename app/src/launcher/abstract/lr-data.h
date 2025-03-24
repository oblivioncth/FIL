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
    DataDoc(InstallT* install, const QString& docPath, const QString& docName);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~DataDoc() = default;

//-Instance Functions-----------------------------------------------------------------------------------------------
public:
    InstallT* install() const;

    // IMPLEMENT
    using IDataDoc::type;
    using IDataDoc::isEmpty;

    // OPTIONALLY RE-IMPELEMENT
    using IDataDoc::postCheckout; // Does nothing by default
    using IDataDoc::preCommit; // Does nothing by default
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
public:
    DocT* target() const;

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
public:
    DocT* source() const;

    // IMPLEMENT
    using IDataDoc::Writer::writeOutOf;
};

template<LauncherId Id>
class UpdatableDoc : public IUpdatableDoc
{
protected:
    using InstallT = Id::InstallT;
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit UpdatableDoc(InstallT* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    InstallT* install() const;

    // IMPLEMENT
    using IUpdatableDoc::isEmpty;

    // OPTIONALLY RE-IMPELEMENT
    using IDataDoc::postCheckout;
    using IDataDoc::preCommit; // Does nothing by default
};

template<LauncherId Id>
class PlatformDoc : public IPlatformDoc
{
protected:
    using InstallT = Id::InstallT;
    using GameT = Id::GameT;
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlatformDoc(InstallT* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    // IMPLEMENT
    virtual const GameT* processSet(const Fp::Set& set) = 0; // Returned pointer must remain valid until at least the next time this method is called

public:
    InstallT* install() const;

    const Game* addSet(const Fp::Set& set) override;  // Might just want to have this overridden directly as GameT will naturally upcast to Game*

    // IMPLEMENT
    using IPlatformDoc::isEmpty;
    using IPlatformDoc::containsGame; // NOTE: UNUSED
    using IPlatformDoc::containsAddApp; // NOTE: UNUSED

    // OPTIONALLY RE-IMPELEMENT
    using IDataDoc::postCheckout;
    using IDataDoc::preCommit; // Does nothing by default
};

template<LauncherId Id>
class PlaylistDoc : public IPlaylistDoc
{
protected:
    using InstallT = Id::InstallT;
//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit PlaylistDoc(InstallT* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    InstallT* install() const;

    // IMPLEMENT
    using IPlaylistDoc::isEmpty;
    using IPlaylistDoc::containsPlaylistGame; // NOTE: UNUSED
    using IPlaylistDoc::setPlaylistData;

    // OPTIONALLY RE-IMPELEMENT
    using IDataDoc::postCheckout;
    using IDataDoc::preCommit; // Does nothing by default
};


template<LauncherId Id>
class BasicPlatformDoc : public PlatformDoc<Id>
{
protected:
    using InstallT = Id::InstallT;
    using GameT = Id::GameT;
    using AddAppT = Id::AddAppT;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    IUpdatableDoc::UpdatableContainer<GameT> mGames;
    IUpdatableDoc::UpdatableContainer<AddAppT> mAddApps;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlatformDoc(InstallT* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    // IMPLEMENT
    virtual GameT prepareGame(const Fp::Game& game) = 0;
    virtual AddAppT prepareAddApp(const Fp::AddApp& game) = 0;

public:
    InstallT* install() const;

    bool containsGame(const QUuid& gameId) const override; // NOTE: UNUSED
    bool containsAddApp(const QUuid& addAppId) const override; // NOTE: UNUSED

    const GameT* processSet(const Fp::Set& set) override;

    // OPTIONALLY RE-IMPELEMENT
    virtual bool isEmpty() const override;
    using IDataDoc::postCheckout;
    using IDataDoc::preCommit; // Does nothing by default
};

template<LauncherId Id>
class BasicPlaylistDoc : public PlaylistDoc<Id>
{
protected:
    using InstallT = Id::InstallT;
    using PlaylistHeaderT = Id::PlaylistHeaderT;
    using PlaylistGameT = Id::PlaylistGameT;

//-Instance Variables--------------------------------------------------------------------------------------------------
protected:
    PlaylistHeaderT mPlaylistHeader;
    IUpdatableDoc::UpdatableContainer<PlaylistGameT> mPlaylistGames;

//-Constructor--------------------------------------------------------------------------------------------------------
protected:
    explicit BasicPlaylistDoc(InstallT* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions);

//-Instance Functions--------------------------------------------------------------------------------------------------
protected:
    // IMPLEMENT
    virtual PlaylistHeaderT preparePlaylistHeader(const Fp::Playlist& playlist) = 0;
    virtual PlaylistGameT preparePlaylistGame(const Fp::PlaylistGame& game) = 0;

public:
    InstallT* install() const;

    bool containsPlaylistGame(const QUuid& gameId) const override;
    void setPlaylistData(const Fp::Playlist& playlist) override;

    // OPTIONALLY RE-IMPELEMENT
    virtual bool isEmpty() const override;
    using IDataDoc::postCheckout;
    using IDataDoc::preCommit; // Does nothing by default
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
    XmlDocReader(DocT* targetDoc, const QString& root = {});

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
    XmlDocWriter(DocT* sourceDoc, const QString& root = {});

//-Instance Functions-------------------------------------------------------------------------------------------------
protected:
    void writeCleanTextElement(const QString& qualifiedName, const QString& text);
    void writeCleanTextElement(const QString& qualifiedName, const QString& text, const QXmlStreamAttributes& attributes);
    void writeOtherFields(const Item& item);
    DocHandlingError streamStatus() const;

    // IMPLEMENT
    virtual bool writeSourceDoc() = 0;

public:
    DocHandlingError writeOutOf() override;
};

}

#include "lr-data.tpp"
#endif // LR_DATA_H
