#ifndef LAUNCHBOXINSTALL_H
#define LAUNCHBOXINSTALL_H

#include <QString>
#include <QDir>
#include <QSet>
#include "qx-io.h"

namespace LB {

class LaunchBoxInstall
{
//-Inner Classes-------------------------------------------------------------------------------------------------
public:
    class XMLMainElement_Game
    {
    public:

    };

    class XMLMainElement_AdditionalApp
    {
    public:

    };

    class XMLMainElement_PlaylistHeader
    {
    public:

    };

    class XMLMainElement_PlaylistGame
    {
    public:

    };

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

   QStringList getExistingPlatformsList() const;
   QStringList getExistingPlaylistsList() const;

};

}
#endif // LAUNCHBOXINSTALL_H
