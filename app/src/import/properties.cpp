// Unit Include
#include "properties.h"

// Qx Includes
#include <qx/widgets/qx-common-widgets.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "launcher/interface/lr-install-interface.h"

namespace Import
{

//===============================================================================================================
// Properties
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Properties::Properties() :
    mHasLinkPerms(testForLinkPermissions()),
    mLauncher(nullptr),
    mFlashpoint(nullptr)
{
    mLauncherReady.setBinding([this]{ return mLauncher.value() && mLauncher->isValid(); });
    mFlashpointReady.setBinding([this]{ return mFlashpoint.value() && mFlashpoint->isValid(); });
    mBothTargetsReady.setBinding([this]{ return mLauncherReady && mFlashpointReady; });
    mBothTargetsReady.addLifetimeNotifier([this]{
        if(mBothTargetsReady)
            gatherTargetData();
        else
        {
            // Clear out selection lists
            mPlatforms = QList<Importee>();
            mPlaylists = QList<Importee>();
        }
    });
    mFlashpointTargetSeries.setBinding([this]{ return mFlashpointReady && installMatchesTargetSeries(*mFlashpoint.value()); });
    mImageModeOrder.setBinding([this]{
        /* Even though technically we only need the launcher, check for both installs to prevent the selection
         * from moving until its section is available
         */
        static QList<Import::ImageMode> defOrder{Import::ImageMode::Link, Import::ImageMode::Reference, Import::ImageMode::Copy};
        bool def = !mBothTargetsReady;
        auto order = def ? defOrder : mLauncher->preferredImageModeOrder();
        if(!mHasLinkPerms)
            order.removeAll(Import::ImageMode::Link);

        return order;
    });
    mImageDownloadable.setBinding([this]{
        return mFlashpointReady && mFlashpoint->preferences().onDemandImages;
    }                                                           );
    mLauncherInfo.setBinding([this]{ return mLauncherReady ? mLauncher->name() + ' ' + mLauncher->versionString() : QString(); });
    mFlashpointInfo.setBinding([this]{ return mFlashpointReady ? mFlashpoint->versionInfo()->fullString() : QString(); });
    mTagMap.setBinding([this]{ return mFlashpointReady ? mFlashpoint->database()->tags() : QMap<int, Fp::Db::TagCategory>(); });
}

//-Class Functions-------------------------------------------------------------
//Private:
bool Properties::testForLinkPermissions()
{
    QTemporaryDir testLinkDir;
    if(testLinkDir.isValid())
    {
        QFile testLinkTarget(testLinkDir.filePath(u"linktarget.tmp"_s));

        if(testLinkTarget.open(QIODevice::WriteOnly))
        {
            testLinkTarget.close();
            std::error_code symlinkError;
            std::filesystem::create_symlink(testLinkTarget.fileName().toStdString(), testLinkDir.filePath(u"testlink.tmp"_s).toStdString(), symlinkError);

            if(!symlinkError)
                return true;
        }
    }

    // Default
    return false;
}

bool Properties::installMatchesTargetSeries(const Fp::Install& fpInstall)
{
    Qx::VersionNumber fpVersion = fpInstall.versionInfo()->version();
    return TARGET_FP_VERSION_PREFIX.isPrefixOf(fpVersion) ||
           TARGET_FP_VERSION_PREFIX.normalized() == fpVersion; // Accounts for if FP doesn't use a trailing zero for major releases
}

//-Instance Functions-------------------------------------------------------------
//Private:
void Properties::gatherTargetData()
{
    // IO Error check instance
    Qx::Error existingCheck;

    // Get list of existing platforms and playlists
    existingCheck = mLauncher->refreshExistingDocs();

    // IO Error Check
    if(existingCheck.isValid())
    {
        Qx::postBlockingError(existingCheck);
        mLauncher = nullptr;
        return;
    }

    /* We set the platform/playlist properties here instead of using a binding because gatherLauncherData()
     * might need to be called in contexts where there is no trivial way to cause the binding to re-evaluate
     * without adding a hacky bool property specifically for that purpose.
     */
    QList<Import::Importee> plats;
    for(const QString& p : mFlashpoint->database()->platformNames())
        plats.append({.name = p, .existing = mLauncher->containsPlatform(p)});

    QList<Import::Importee> plays;
    for(const QString& p : mFlashpoint->playlistManager()->playlistTitles())
        plays.append({.name = p, .existing = mLauncher->containsPlaylist(p)});

    mPlatforms.setValue(std::move(plats));
    mPlaylists.setValue(std::move(plays));
}

//Public:
bool Properties::hasLinkPermissions() const { return mHasLinkPerms; }
bool Properties::isLauncherReady() const { return mLauncherReady; }
bool Properties::isFlashpointReady() const { return mFlashpointReady; }
bool Properties::isBothTargetsReady() const { return mBothTargetsReady; }
bool Properties::isFlashpointTargetSeries() const { return mFlashpointTargetSeries; }
const Qx::Bindable<QList<ImageMode>> Properties::bindableImageModeOrder() const { return mImageModeOrder; }
QList<ImageMode> Properties::imageModeOrder() const { return mImageModeOrder; }
bool Properties::isImageDownloadable() const { return mImageDownloadable; }
QString Properties::launcherInfo() const { return mLauncherInfo; }
QString Properties::flashpointInfo() const { return mFlashpointInfo; }
const Qx::Bindable<QMap<int, Fp::Db::TagCategory>> Properties::bindableTagMap() const { return mTagMap; }
QMap<int, Fp::Db::TagCategory> Properties::tagMap() const { return mTagMap; }
const Qx::Bindable<QList<Importee>> Properties::bindablePlatforms() const { return mPlatforms; }
QList<Importee> Properties::platforms() const { return mPlatforms; }
const Qx::Bindable<QList<Importee>> Properties::bindablePlaylists() const { return mPlaylists; }
QList<Importee> Properties::playlists() const { return mPlaylists; }

void Properties::setLauncher(std::unique_ptr<Lr::IInstall>&& launcher) { mLauncher = std::move(launcher); }
void Properties::setFlashpoint(std::unique_ptr<Fp::Install>&& flashpoint) { mFlashpoint = std::move(flashpoint); }
void Properties::refreshInstallData() { gatherTargetData(); }

Lr::IInstall* Properties::launcher() { Q_ASSERT(*mLauncher); return (*mLauncher).get(); }
Fp::Install* Properties::flashpoint() { Q_ASSERT(*mFlashpoint); return (*mFlashpoint).get(); };

}
