// Unit Include
#include "lr-data-interface.h"

// Project Includes
#include "launcher/interface/lr-install-interface.h"

namespace Lr
{
//===============================================================================================================
// DocHandlingError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
DocHandlingError::DocHandlingError() :
    mType(NoError)
{}

DocHandlingError::DocHandlingError(const IDataDoc& doc, Type t, const QString& s) :
    mType(t),
    mErrorStr(generatePrimaryString(doc, t)),
    mSpecific(s)
{}

//-Class Functions-------------------------------------------------------------
//Private:
QString DocHandlingError::generatePrimaryString(const IDataDoc& doc, Type t)
{
    // TODO: Use Qx for this
    QString formattedError = ERR_STRINGS[t];
    formattedError.replace(M_DOC_TYPE, doc.identifier().docTypeString());
    formattedError.replace(M_DOC_NAME, doc.identifier().docName());
    formattedError.replace(M_DOC_PARENT, doc.install()->name());

    return formattedError;
}

//-Instance Functions-------------------------------------------------------------
//Public:
bool DocHandlingError::isValid() const { return mType != NoError; }
QString DocHandlingError::errorString() const { return mErrorStr; }
QString DocHandlingError::specific() const { return mSpecific; }
DocHandlingError::Type DocHandlingError::type() const { return mType; }

//Private:
Qx::Severity DocHandlingError::deriveSeverity() const { return Qx::Critical; }
quint32 DocHandlingError::deriveValue() const { return mType; }
QString DocHandlingError::derivePrimary() const { return mErrorStr; }
QString DocHandlingError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// IDataDoc::Identifier
//===============================================================================================================

//-Operators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const IDataDoc::Identifier& lhs, const IDataDoc::Identifier& rhs) noexcept
{
    return lhs.mDocType == rhs.mDocType && lhs.mDocName == rhs.mDocName;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const IDataDoc::Identifier& key, uint seed) noexcept
{
    seed = qHash(key.mDocType, seed);
    seed = qHash(key.mDocName, seed);

    return seed;
}

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
IDataDoc::Identifier::Identifier(Type docType, QString docName) :
    mDocType(docType),
    mDocName(docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
IDataDoc::Type IDataDoc::Identifier::docType() const { return mDocType; }
QString IDataDoc::Identifier::docTypeString() const { return TYPE_STRINGS.value(mDocType); }
QString IDataDoc::Identifier::docName() const { return mDocName; }

//===============================================================================================================
// IDataDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
IDataDoc::IDataDoc(IInstall* install, const QString& docPath, QString docName) :
      mInstall(install),
      mDocumentPath(docPath),
      mName(docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
IInstall* IDataDoc::install() const { return mInstall; }
QString IDataDoc::path() const { return mDocumentPath; }
IDataDoc::Identifier IDataDoc::identifier() const { return Identifier(type(), mName); }

//===============================================================================================================
// IDataDoc::Reader
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
IDataDoc::Reader::Reader(IDataDoc* targetDoc) :
    mTargetDocument(targetDoc)
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
IDataDoc::Reader::~Reader() {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
IDataDoc* IDataDoc::Reader::target() const { return mTargetDocument; }

//===============================================================================================================
// IDataDoc::Writer
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
IDataDoc::Writer::Writer(IDataDoc* sourceDoc) :
    mSourceDocument(sourceDoc)
{}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
IDataDoc::Writer::~Writer() {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
IDataDoc* IDataDoc::Writer::source() const { return mSourceDocument; }

//===============================================================================================================
// Errorable
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
IErrorable::IErrorable() {}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
IErrorable::~IErrorable() {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
bool IErrorable::hasError() const { return mError.isValid(); }
Qx::Error IErrorable::error() const { return mError; }

//===============================================================================================================
// UpdateableDoc
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
IUpdateableDoc::IUpdateableDoc(IInstall* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    IDataDoc(install, docPath, docName),
    mUpdateOptions(updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
void IUpdateableDoc::finalize() {} // Does nothing for base class

//===============================================================================================================
// PlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
IPlatformDoc::IPlatformDoc(IInstall* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    IUpdateableDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
IDataDoc::Type IPlatformDoc::type() const { return Type::Platform; }

//===============================================================================================================
// PlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
IPlaylistDoc::IPlaylistDoc(IInstall* install, const QString& docPath, QString docName, const Import::UpdateOptions& updateOptions) :
    IUpdateableDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
IDataDoc::Type IPlaylistDoc::type() const { return Type::Playlist; }

}

