#ifndef IMPORT_PROPERTIES_H
#define IMPORT_PROPERTIES_H

// Qx Includes
#include <qx/core/qx-property.h>

// Qx Includes
#include <qx/core/qx-versionnumber.h>

// libfp Includes
#include <fp/fp-db.h>

// Project Includes
#include "import/settings.h"
#include "project_vars.h"

/* TODO: PROBABLY OK NOW The number of properties here has gotten somewhat out of hand.
 * Mainwindow should probably just be given access to something like
 * const QBindable<const std::shared_ptr<Fp::Install>> (and same for
 * launcher) so that it can setup its own bindings/properties directly
 * off of that. Since it would be read only, that still lets Controller
 * have control of the instances.
 */

namespace Lr { class IInstall; }
namespace Fp { class Install; }

namespace Import
{

class Properties
{
//-Class Variables---------------------------------------------------------------
private:
    // Flashpoint version check
    static inline const Qx::VersionNumber TARGET_FP_VERSION_PREFIX = Qx::VersionNumber::fromString(PROJECT_TARGET_FP_VER_PFX_STR);

//-Instance Variables-------------------------------------------------------------
private:
    bool mHasLinkPerms;
    Qx::Property<std::unique_ptr<Lr::IInstall>> mLauncher;
    Qx::Property<std::unique_ptr<Fp::Install>> mFlashpoint;
    Qx::Property<bool> mLauncherReady;
    Qx::Property<bool> mFlashpointReady;
    Qx::Property<bool> mBothTargetsReady;
    Qx::Property<bool> mFlashpointTargetSeries;
    Qx::Property<QList<ImageMode>> mImageModeOrder;
    Qx::Property<bool> mImageDownloadable;
    Qx::Property<QString> mLauncherInfo;
    Qx::Property<QString> mFlashpointInfo;
    Qx::Property<QMap<int, Fp::Db::TagCategory>> mTagMap;
    Qx::Property<QList<Importee>> mPlatforms;
    Qx::Property<QList<Importee>> mPlaylists;

//-Constructor-------------------------------------------------------------
public:
    Properties();

//-Class Functions-------------------------------------------------------------
private:
    static bool testForLinkPermissions();

public:
    static bool installMatchesTargetSeries(const Fp::Install& fpInstall);

//-Instance Functions-------------------------------------------------------------
private:
    void gatherTargetData();

public:
    bool hasLinkPermissions() const;
    bool isLauncherReady() const;
    bool isFlashpointReady() const;
    bool isBothTargetsReady() const;
    bool isFlashpointTargetSeries() const;
    const Qx::Bindable<QList<ImageMode>> bindableImageModeOrder() const;
    QList<ImageMode> imageModeOrder() const;
    bool isImageDownloadable() const;
    QString launcherInfo() const;
    QString flashpointInfo() const;
    const Qx::Bindable<QMap<int, Fp::Db::TagCategory>> bindableTagMap() const;
    QMap<int, Fp::Db::TagCategory> tagMap() const;
    const Qx::Bindable<QList<Importee>> bindablePlatforms() const;
    QList<Importee> platforms() const;
    const Qx::Bindable<QList<Importee>> bindablePlaylists() const;
    QList<Importee> playlists() const;

    void setLauncher(std::unique_ptr<Lr::IInstall>&& launcher);
    void setFlashpoint(std::unique_ptr<Fp::Install>&& flashpoint);
    void refreshInstallData();

    Lr::IInstall* launcher();
    Fp::Install* flashpoint();
};

}

#endif // IMPORT_PROPERTIES_H
