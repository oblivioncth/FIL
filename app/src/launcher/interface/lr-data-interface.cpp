// Unit Include
#include "lr-data-interface.h"

// Qx Includes
#include <qx/core/qx-string.h>

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
    return Qx::String::mapArg(ERR_STRINGS[t],{
        {M_DOC_TYPE, doc.identifier().docTypeString()},
        {M_DOC_NAME, doc.identifier().docName()},
        {M_DOC_PARENT, doc.install()->name()}
    });
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
size_t qHash(const IDataDoc::Identifier& key, size_t seed) noexcept
{
    return qHashMulti(seed, key.mDocType, key.mDocName);
}

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
IDataDoc::Identifier::Identifier(Type docType, const QString& docName) :
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
IDataDoc::IDataDoc(IInstall* install, const QString& docPath, const QString& docName) :
      mInstall(install),
      mDocumentPath(docPath),
      mName(docName)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
IInstall* IDataDoc::install() const { return mInstall; }
QString IDataDoc::path() const { return mDocumentPath; }
IDataDoc::Identifier IDataDoc::identifier() const { return Identifier(type(), mName); }
void IDataDoc::postCheckout() {}
void IDataDoc::preCommit() {}

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
// IErrorable
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
//IErrorable::IErrorable() {}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
//IErrorable::~IErrorable() {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
//bool IErrorable::hasError() const { return mError.isValid(); }
//Qx::Error IErrorable::error() const { return mError; }

//===============================================================================================================
// IUpdatableDoc
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
IUpdatableDoc::IUpdatableDoc(IInstall* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions) :
    IDataDoc(install, docPath, docName),
    mUpdating(false),
    mUpdateOptions(updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
void IUpdatableDoc::postCheckout() { mUpdating = true; }

//===============================================================================================================
// IPlatformDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
IPlatformDoc::IPlatformDoc(IInstall* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions) :
    IUpdatableDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
IDataDoc::Type IPlatformDoc::type() const { return Type::Platform; }

//===============================================================================================================
// IPlaylistDoc
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
IPlaylistDoc::IPlaylistDoc(IInstall* install, const QString& docPath, const QString& docName, const Import::UpdateOptions& updateOptions) :
    IUpdatableDoc(install, docPath, docName, updateOptions)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
IDataDoc::Type IPlaylistDoc::type() const { return Type::Playlist; }

}

