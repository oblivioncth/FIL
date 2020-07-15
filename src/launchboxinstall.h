#ifndef LAUNCHBOXINSTALL_H
#define LAUNCHBOXINSTALL_H

#include <QString>
#include <QDir>
#include <QSet>
#include "qx-io.h"

class LaunchBoxInstall
{
//-Class Variables-----------------------------------------------------------------------------------------------
public:
    static inline const QString PLATFORMS_PATH = "Data/Platforms";
    static inline const QString PLAYLISTS_PATH = "Data/Playlists";
    static inline const QString MAIN_EXE_PATH = "LaunchBox.exe";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mPlatformsDirectory;
    QDir mPlaylistsDirectory;

    // XML Information
    QStringList mExistingPlatformsList;
    QStringList mExistingPlaylistList;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    LaunchBoxInstall(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
public:
   static bool pathIsValidLaunchBoxInstall(QString installPath);


//-Instance Functions------------------------------------------------------------------------------------------------------
public:
   Qx::IO::IOOpReport populateExistingPlatforms();
   Qx::IO::IOOpReport populateExistingPlaylists();

   QStringList getExistingPlatformsList();
   QStringList getExistingPlaylistsList();

};

#endif // LAUNCHBOXINSTALL_H
