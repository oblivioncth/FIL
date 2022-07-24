// Unit Include
#include "fe-install.h"

// Qt Includes
#include <QFileInfo>

// Qx Includes
#include <qx/windows/qx-filedetails.h>

namespace Fe
{

//===============================================================================================================
// Install
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
Install::Install(QString installPath) :
    InstallFoundation(installPath)
{}

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QMap<QString, Install::Entry>& Install::registry() { static QMap<QString, Entry> registry; return registry; }

void Install::registerInstall(QString name, Entry entry) { registry()[name] = entry; }

std::shared_ptr<Install> Install::acquireMatch(const QString& installPath)
{
    // Check all installs against path and return match if found
    QMap<QString, Entry>::const_iterator i;

    for(i = registry().constBegin(); i != registry().constEnd(); ++i)
    {
        Entry entry = i.value();
        std::shared_ptr<Install> possibleMatch = entry.factory->produce(installPath);

        if(possibleMatch->isValid())
            return possibleMatch;
    }

    // Return nullptr on failure to find match
    return nullptr;
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Protected:
void Install::nullify()
{
    // Redundant with base version, but here to make it clear its part of the main Install interface
    InstallFoundation::nullify();
}

QString Install::translateDocName(const QString& originalName) const
{
    // Redundant with base version, but here to make it clear its part of the main Install interface
    return InstallFoundation::translateDocName(originalName);
}

//Public:
void Install::softReset()
{
    // Redundant with base version, but here to make it clear its part of the main Install interface
    InstallFoundation::softReset();
}

bool Install::supportsImageMode(ImageMode imageMode) const { return preferredImageModeOrder().contains(imageMode); }

QString Install::versionString() const
{
    Qx::FileDetails exeDetails = Qx::FileDetails::readFileDetails(executablePath());

    QString fileVersionStr = exeDetails.stringTable().fileVersion;
    QString productVersionStr = exeDetails.stringTable().productVersion;

    if(!fileVersionStr.isEmpty())
        return fileVersionStr;
    else if(!productVersionStr.isEmpty())
        return productVersionStr;
    else
        return "Unknown Version";
}

/* These functions can be overridden by children as needed.
 * Work within them should be kept as minimal as possible since they are not accounted
 * for by the import progress indicator.
 */
Qx::GenericError Install::preImport(const ImportDetails& details)
{
    mImportDetails = std::make_unique<ImportDetails>(details);
    return Qx::GenericError();
}

Qx::GenericError Install::postImport() { return {}; }
Qx::GenericError Install::prePlatformsImport() { return {}; }
Qx::GenericError Install::postPlatformsImport() { return {}; }
Qx::GenericError Install::preImageProcessing(QList<ImageMap>& workerTransfers, ImageSources bulkSources) { return {}; }
Qx::GenericError Install::postImageProcessing() { return {}; }
Qx::GenericError Install::prePlaylistsImport() { return {}; }
Qx::GenericError Install::postPlaylistsImport() { return {}; }
/* */

Qx::GenericError Install::checkoutPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, QString name)
{
    // Translate to frontend doc name
    QString translatedName = translateDocName(name);

    // Get initialized blank doc and reader
    std::shared_ptr<PlatformDocReader> docReader = preparePlatformDocCheckout(returnBuffer, translatedName);

    // Open document
    Qx::GenericError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::checkoutPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, QString name)
{
    // Translate to frontend doc name
    QString translatedName = translateDocName(name);

    // Get initialized blank doc and reader
    std::shared_ptr<PlaylistDocReader> docReader = preparePlaylistDocCheckout(returnBuffer, translatedName);

    // Open document
    Qx::GenericError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Qx::GenericError Install::commitPlatformDoc(std::unique_ptr<PlatformDoc> document)
{
    // Doc should belong to this install
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlatformDocWriter> docWriter = preparePlatformDocCommit(document);

    // Write
    Qx::GenericError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Qx::GenericError Install::commitPlaylistDoc(std::unique_ptr<PlaylistDoc> document)
{
    // Doc should belong to this install
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlaylistDocWriter> docWriter = preparePlaylistDocCommit(document);

    // Write
    Qx::GenericError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

}
