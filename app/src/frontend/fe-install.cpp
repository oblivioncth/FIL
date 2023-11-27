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
Install::Install(const QString& installPath) :
    InstallFoundation(installPath)
{}

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QMap<QString, Install::Entry>& Install::registry() { static QMap<QString, Entry> registry; return registry; }

void Install::registerInstall(const QString& name, const Entry& entry) { registry()[name] = entry; }

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
        return u"Unknown Version"_s;
}

/* These functions can be overridden by children as needed.
 * Work within them should be kept as minimal as possible since they are not accounted
 * for by the import progress indicator.
 */
Qx::Error Install::preImport(const ImportDetails& details)
{
    mImportDetails = std::make_unique<ImportDetails>(details);
    return Qx::Error();
}

Qx::Error Install::postImport() { return {}; }
Qx::Error Install::prePlatformsImport() { return {}; }
Qx::Error Install::postPlatformsImport() { return {}; }

Qx::Error Install::preImageProcessing(QList<ImageMap>& workerTransfers, const ImageSources& bulkSources)
{
    Q_UNUSED(workerTransfers);
    Q_UNUSED(bulkSources);
    return {};
}

Qx::Error Install::postImageProcessing() { return {}; }
Qx::Error Install::prePlaylistsImport() { return {}; }
Qx::Error Install::postPlaylistsImport() { return {}; }

QString Install::translateDocName(const QString& originalName, DataDoc::Type type) const
{
    // Redundant with base version, but here to make it clear its part of the main Install interface
    return InstallFoundation::translateDocName(originalName, type);
}

Fe::DocHandlingError Install::checkoutPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, const QString& name)
{
    // Translate to frontend doc name
    QString translatedName = translateDocName(name, DataDoc::Type::Platform);

    // Get initialized blank doc and reader
    std::shared_ptr<PlatformDoc::Reader> docReader = preparePlatformDocCheckout(returnBuffer, translatedName);

    // Open document
    Fe::DocHandlingError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Fe::DocHandlingError Install::checkoutPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, const QString& name)
{
    // Translate to frontend doc name
    QString translatedName = translateDocName(name, DataDoc::Type::Playlist);

    // Get initialized blank doc and reader
    std::shared_ptr<PlaylistDoc::Reader> docReader = preparePlaylistDocCheckout(returnBuffer, translatedName);

    // Open document
    Fe::DocHandlingError readErrorStatus = checkoutDataDocument(returnBuffer.get(), docReader);

    // Set return null on failure
    if(readErrorStatus.isValid())
        returnBuffer.reset();

    // Return status
    return readErrorStatus;
}

Fe::DocHandlingError Install::commitPlatformDoc(std::unique_ptr<PlatformDoc> document)
{
    // Doc should belong to this install
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlatformDoc::Writer> docWriter = preparePlatformDocCommit(document);

    // Write
    Fe::DocHandlingError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

Fe::DocHandlingError Install::commitPlaylistDoc(std::unique_ptr<PlaylistDoc> document)
{
    // Doc should belong to this install
    assert(document->parent() == this);

    // Prepare writer
    std::shared_ptr<PlaylistDoc::Writer> docWriter = preparePlaylistDocCommit(document);

    // Write
    Fe::DocHandlingError writeErrorStatus = commitDataDocument(document.get(), docWriter);

    // Ensure document is cleared
    document.reset();

    // Return write status and let document ptr auto delete
    return writeErrorStatus;
}

QString Install::platformCategoryIconPath() const { return QString(); } // Unsupported in default implementation
std::optional<QDir> Install::platformIconsDirectory() const { return std::nullopt; }; // Unsupported in default implementation
std::optional<QDir> Install::playlistIconsDirectory() const { return std::nullopt; }; // Unsupported in default implementation

}
