#include "launchboxinstall.h"
#include <QFileInfo>
#include <QDir>

namespace LB
{

//===============================================================================================================
// LAUNCHBOX INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
LaunchBoxInstall::LaunchBoxInstall(QString installPath)
{
    // Ensure instance will be valid
    if(!pathIsValidLaunchBoxInstall(installPath))
        assert("Cannot create a LaunchBoxInstall instance with an invalid installPath. Check first with LaunchBoxInstall::pathIsValidLaunchBoxInstall(QString).");

    // Initialize files and directories;
    mRootDirectory = QDir(installPath);
    mPlatformsDirectory = QDir(installPath + '/' + PLATFORMS_PATH);
    mPlaylistsDirectory = QDir(installPath + '/' + PLAYLISTS_PATH);
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
bool LaunchBoxInstall::pathIsValidLaunchBoxInstall(QString installPath)
{
    QFileInfo platformsFolder(installPath + "/" + PLATFORMS_PATH);
    QFileInfo playlistsFolder(installPath + "/" + PLATFORMS_PATH);
    QFileInfo mainEXE(installPath + "/" + MAIN_EXE_PATH);

    return platformsFolder.exists() && platformsFolder.isDir() &&
           playlistsFolder.exists() && playlistsFolder.isDir() &&
           mainEXE.exists() && mainEXE.isExecutable();
}

//-Instance Functions----------------------------------------------------------------------------------------------
//Public:
Qx::IO::IOOpReport LaunchBoxInstall::populateExistingPlatforms() { return Qx::IO::getDirFileList(mExistingPlatformsList, mPlatformsDirectory, false, {"xml"}); }
Qx::IO::IOOpReport LaunchBoxInstall::populateExistingPlaylists() { return Qx::IO::getDirFileList(mExistingPlatformsList, mPlatformsDirectory, false, {"xml"}); }
QStringList LaunchBoxInstall::getExistingPlatformsList() const { return mExistingPlatformsList; }
QStringList LaunchBoxInstall::getExistingPlaylistsList() const { return mExistingPlaylistList; }


}
