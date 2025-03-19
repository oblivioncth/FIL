// Unit Includes
#include "image.h"

// Qt Includes
#include <QImageWriter>

// Qx Includes
#include <qx/core/qx-progressgroup.h>
#include <qx/core/qx-genericerror.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "launcher/interface/lr-items-interface.h"
#include "launcher/interface/lr-install-interface.h"
#include "import/backup.h"

namespace Import
{

/* TODO: This and its interaction with Worker need to be significantly cleaned up, especially
 * in regards to cancellation handling and error reporting. Maybe use std::expected.
 *
 * It's a mess right now just so the movement of these facilities to this class could get up
 * and running as fast as possible.
 */
//===============================================================================================================
// ImageTransferError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
ImageTransferError::ImageTransferError(Type t, const QString& src, const QString& dest) :
    mType(t),
    mSourcePath(src),
    mDestinationPath(dest)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool ImageTransferError::isValid() const { return mType != NoError; }
ImageTransferError::Type ImageTransferError::type() const { return mType; }

//Private:
Qx::Severity ImageTransferError::deriveSeverity() const { return Qx::Err; }
quint32 ImageTransferError::deriveValue() const { return mType; }
QString ImageTransferError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString ImageTransferError::deriveSecondary() const { return IMAGE_RETRY_PROMPT; }

QString ImageTransferError::deriveDetails() const
{
    QString det;
    if(!mSourcePath.isEmpty())
        det += SRC_PATH_TEMPLATE.arg(mSourcePath) + '\n';
    if(!mDestinationPath.isEmpty())
    {
        if(!det.isEmpty())
            det += '\n';
        det += DEST_PATH_TEMPLATE.arg(mDestinationPath) + '\n';
    }

    return det;
}

QString ImageTransferError::deriveCaption() const { return CAPTION_IMAGE_ERR; }

//===============================================================================================================
// ImageManager
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
ImageManager::ImageManager(Fp::Install* fp, Lr::IInstall* lr, bool& canceledFlag) :
    mFlashpoint(fp),
    mLauncher(lr),
    mDownload(false),
    mMode(ImageMode::Copy),
    mCanceled(canceledFlag)
{}

//-Class Functions-------------------------------------------------------------
//Private:

//Public:

//-Instance Functions-------------------------------------------------------------
//Private:
ImageTransferError ImageManager::transferImage(bool symlink, const QString& sourcePath, const QString& destPath)
{
    /* TODO: Ideally the error handlers here don't need to include "Retry?" text and therefore need less use of QString::arg(); however, this largely
     * would require use of a button labeled "Ignore All" so that the errors could presented as is without a prompt, with the prompt being inferred
     * through the button choices "Retry", "Ignore", and "Ignore All", but currently the last is not a standard button and due to how Qx::GenericError
     * is implemented custom buttons aren't feasible. Honestly maybe just try adding it to Qt and see if its accepted.
     */

    // Image info
    QFileInfo destinationInfo(destPath);
    QDir destinationDir(destinationInfo.absolutePath());
    bool destinationOccupied = destinationInfo.exists() && (destinationInfo.isFile() || destinationInfo.isSymLink());

    // Return if source in unexpectedly missing (i.e. download failure)
    if(!QFile::exists(sourcePath))
        return ImageTransferError(ImageTransferError::ImageSourceUnavailable, sourcePath);

    // Return if image is already up-to-date
    if(destinationOccupied)
    {
        if(destinationInfo.isSymLink() && symlink)
            return ImageTransferError();
        else if(!destinationInfo.isSymLink() && !symlink)
        {
            QFile source(sourcePath);
            QFile destination(destPath);
            QString sourceChecksum;
            QString destinationChecksum;

            // TODO: Probably better to just byte-wise compare
            if(!Qx::calculateFileChecksum(sourceChecksum, source, QCryptographicHash::Md5).isFailure() &&
                !Qx::calculateFileChecksum(destinationChecksum, destination, QCryptographicHash::Md5).isFailure() &&
                sourceChecksum.compare(destinationChecksum, Qt::CaseInsensitive) == 0)
                return ImageTransferError();
        }
        // Image is always updated if changing between Link/Copy
    }

    // Ensure destination path exists
    if(!destinationDir.mkpath(u"."_s))
        return ImageTransferError(ImageTransferError::CantCreateDirectory, QString(), destinationDir.absolutePath());

    // Transfer image
    BackupError bErr = BackupManager::instance()->safeReplace(sourcePath, destPath, symlink);
    if(bErr)
    {
        if(bErr.type() == BackupError::FileWontBackup)
            return ImageTransferError(ImageTransferError::ImageWontBackup, QString(), destPath);
        else if(bErr.type() == BackupError::FileWontReplace)
            return ImageTransferError(symlink ? ImageTransferError::ImageWontLink : ImageTransferError::ImageWontCopy, QString(), destPath);
        else
            qFatal("Unhandled image transfer error type.");
    }

    // Return null error on success
    return ImageTransferError();
}

bool ImageManager::performImageJobs(const QList<ImageMap>& jobs, bool symlink, Qx::ProgressGroup* pg)
{
    // Setup for image transfers
    ImageTransferError imageTransferError; // Error return reference
    std::shared_ptr<int> response = std::make_shared<int>();
    *response = QMessageBox::NoToAll; // Default to choice "NoToAll" in case the signal is not correctly connected using Qt::BlockingQueuedConnection
    bool ignoreAllTransferErrors = false; // NoToAll response tracker

    for(const ImageMap& imageJob : jobs)
    {
        while((imageTransferError = transferImage(symlink, imageJob.sourcePath, imageJob.destPath)).isValid() && !ignoreAllTransferErrors)
        {
            // Notify GUI Thread of error
            emit blockingErrorOccured(response, imageTransferError,
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll);

            // Check response
            if(*response == QMessageBox::No)
                break;
            else if(*response == QMessageBox::NoToAll)
                ignoreAllTransferErrors = true;
        }

        // Update progress dialog value
        if(mCanceled)
            return false;
        else if(pg)
            pg->incrementValue();
    }

    return true;
}

//Public:
void ImageManager::setDownload(bool download) { mDownload = download; }
void ImageManager::setMode(ImageMode mode) { mMode = mode; }
void ImageManager::setProgressGroups(Qx::ProgressGroup* download, Qx::ProgressGroup* image, Qx::ProgressGroup* icon)
{
    mDownloadProgress = download;
    mImageProgress = image;
    mIconProgress = icon;
}

void ImageManager::prepareGameImages(const Lr::Game& game)
{
    const Fp::Toolkit* tk = mFlashpoint->toolkit();

    // Get image information
    QFileInfo logoLocalInfo(tk->entryImageLocalPath(Fp::ImageType::Logo, game.id()));
    QFileInfo ssLocalInfo(tk->entryImageLocalPath(Fp::ImageType::Screenshot, game.id()));
    QString checkedLogoPath = (logoLocalInfo.exists() || mDownload) ? logoLocalInfo.absoluteFilePath() : QString();
    QString checkedScreenshotPath = (ssLocalInfo.exists() || mDownload) ? ssLocalInfo.absoluteFilePath() : QString();

    // Setup image downloads if applicable
    if(mDownload)
    {
        if(!logoLocalInfo.exists())
        {
            QUrl logoRemotePath = tk->entryImageRemotePath(Fp::ImageType::Logo, game.id());
            mDownloadManager.appendTask(Qx::DownloadTask{logoRemotePath, logoLocalInfo.absoluteFilePath()});
        }
        else
            mDownloadProgress->decrementMaximum(); // Already exists, remove download step from progress bar

        if(!ssLocalInfo.exists())
        {
            QUrl ssRemotePath = tk->entryImageRemotePath(Fp::ImageType::Screenshot, game.id());
            mDownloadManager.appendTask(Qx::DownloadTask{ssRemotePath, ssLocalInfo.absoluteFilePath()});
        }
        else
            mDownloadProgress->decrementMaximum(); // Already exists, remove download step from progress bar
    }

    // Handle image transfer
    if(mMode == ImageMode::Copy || mMode == ImageMode::Link)
    {
        if(!checkedLogoPath.isEmpty())
            mTransferJobs.append({checkedLogoPath, mLauncher->getDestinationImagePath(game, Fp::ImageType::Logo)});
        else
            mImageProgress->decrementMaximum(); // Can't transfer image that doesn't/won't exist

        if(!checkedScreenshotPath.isEmpty())
            mTransferJobs.append({checkedLogoPath, mLauncher->getDestinationImagePath(game, Fp::ImageType::Screenshot)});
        else
            mImageProgress->decrementMaximum(); // Can't transfer image that doesn't/won't exist
    }
}

Qx::DownloadManagerReport ImageManager::downloadImages()
{
    if(!mDownload || !mDownloadManager.hasTasks())
        return Qx::DownloadManagerReport();

    // Update progress dialog label
    emit progressStepChanged(STEP_DOWNLOADING_IMAGES);

    // Configure manager
    mDownloadManager.setMaxSimultaneous(2);
    mDownloadManager.setOverwrite(false); // Should be no attempts to overwrite, but here just in case
    mDownloadManager.setStopOnError(false); // Get as many images as possible;
    mDownloadManager.setSkipEnumeration(true); // Since progress is being tracked by task count, pre-enumeration of download size is unnecessary
    mDownloadManager.setTransferTimeout(5000); // 5 seconds max to start downloading image before moving on

    // Make connections
    connect(&mDownloadManager, &Qx::SyncDownloadManager::sslErrors, this, [&](Qx::Error errorMsg, bool* ignore) {
        std::shared_ptr<int> response = std::make_shared<int>();
        emit blockingErrorOccured(response, errorMsg, QMessageBox::Yes | QMessageBox::Abort);
        *ignore = *response == QMessageBox::Yes;
    });

    connect(&mDownloadManager, &Qx::SyncDownloadManager::authenticationRequired, this, &ImageManager::authenticationRequired);
    connect(&mDownloadManager, &Qx::SyncDownloadManager::proxyAuthenticationRequired, this, &ImageManager::authenticationRequired);

    connect(&mDownloadManager, &Qx::SyncDownloadManager::downloadFinished, this, [this]() { // clazy:exclude=lambda-in-connect
        mDownloadProgress->incrementValue();
    });

    // Start download
    return mDownloadManager.processQueue();
}

bool ImageManager::importImages()
{
    emit progressStepChanged(STEP_IMPORTING_IMAGES);

    // Provide launcher with bulk reference locations
    if(mMode == ImageMode::Reference)
    {
        Import::ImagePaths bulkSources(QDir::toNativeSeparators(mFlashpoint->entryLogosDirectory().absolutePath()),
                                       QDir::toNativeSeparators(mFlashpoint->entryScreenshotsDirectory().absolutePath()));

        mLauncher->processBulkImageSources(bulkSources);
    }

    // Perform transfers if required
    if(mMode == ImageMode::Copy || mMode == ImageMode::Link)
    {
        /*
         * Account for potential mismatch between assumed and actual job count.
         * For example, this may happen with infinity if a game hasn't been clicked on, as the logo
         * will have been downloaded but not the screenshot
         */
        if(static_cast<quint64>(mTransferJobs.size()) != mImageProgress->maximum())
            mImageProgress->setMaximum(mTransferJobs.size());

        if(!performImageJobs(mTransferJobs, mMode == ImageMode::Link, mImageProgress))
            return false;

        mTransferJobs.clear();
    }
    else if(!mTransferJobs.isEmpty())
        qFatal("the launcher provided image transfers when the mode wasn't link/copy");

    return true;
}

Qx::Error ImageManager::importIcons(bool& canceled, const QStringList& platforms, const QList<Fp::Playlist>& playlists)
{
    canceled = false;

    QList<ImageMap> jobs;
    QString mainDest = mLauncher->platformCategoryIconPath();
    std::optional<QDir> platformDestDir = mLauncher->platformIconsDirectory();
    std::optional<QDir> playlistDestDir = mLauncher->playlistIconsDirectory();

    const Fp::Toolkit* tk = mFlashpoint->toolkit();

    // Main Job
    if(!mainDest.isEmpty())
        jobs.emplace_back(ImageMap{.sourcePath = u":/flashpoint/icon.png"_s, .destPath = mainDest});

    // Platform jobs
    if(platformDestDir)
    {
        QDir pdd = platformDestDir.value();
        for(const QString& p : platforms)
        {
            QString src = tk->platformLogoPath(p);
            if(QFile::exists(src))
                jobs.emplace_back(ImageMap{.sourcePath = src,
                                           .destPath = pdd.absoluteFilePath(p + ".png")});
        }
    }

    // Create temp directory for playlist jobs
    QTemporaryDir iconInflateDir; // Needed at this scope

    // Playlist jobs
    if(playlistDestDir)
    {
        // Validate temp dir
        if(!iconInflateDir.isValid())
            return Qx::GenericError(Qx::Critical, 13501, u"Failed to create directory for playlist icons inflation."_s, iconInflateDir.errorString());

        // Setup playlist image jobs
        QImageWriter iw;
        QDir pdd = playlistDestDir.value();
        for(const Fp::Playlist& p : playlists)
        {
            const QImage& icon = p.icon();
            if(icon.isNull())
                continue;

            /* NOTE: This is LaunchBox specific since it's currently the only FE to support icons. If this changes a general solution is needed
             * Like allowing the launcher to filter out specific icons
             *
             * Don't copy the favorites icon as LB already has its own.
             */
            if(p.title().trimmed() == u"Favorites"_s)
                continue;

            /* NOTE: This may not work for all launchers
             *
             * Use translated name for destination since that's what the launcher is expecting
             */
            QString sFilename = p.title() + ".png";
            QString dFilename = mLauncher->translateDocName(p.title(), Lr::IDataDoc::Type::Playlist) + ".png";;
            QString source = iconInflateDir.filePath(sFilename);
            QString dest = pdd.absoluteFilePath(dFilename);

            iw.setFileName(source);
            if(!iw.write(icon))
                return Qx::GenericError(Qx::Critical, 13502, u"Failed to inflate playlist icon"_s, iw.errorString());

            jobs.emplace_back(ImageMap{.sourcePath = source, .destPath = dest});
        }
    }

    // Perform
    if(!jobs.isEmpty())
    {
        if(!performImageJobs(jobs, false, mIconProgress)) // Always copy
        {
            canceled = true;
            return Qx::Error();
        }
    }

    return Qx::Error();
}

}
