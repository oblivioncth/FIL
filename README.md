# OFILb (Obby's Flashpoint Importer for LaunchBox)
OFILb (pronounced "Awful-B") is an importer tool for [LaunchBox](https://www.launchbox-app.com/) that allows one to add games and playlists from [BlueMaxima's Flashpoint](https://bluemaxima.org/flashpoint/) project to their collection. It is fully automated and only requires the user to provide the paths to the LaunchBox/Flashpoint installs, choose which Platforms/Playlists they wish to import, and select between a few import mode options. Once the import is started the current progress is displayed and any errors that occur are shown to the user, with resolvable errors including a prompt for what the user would like to do. After the process has completed LaunchBox can be started and the games from Flashpoint can be played like those from any other Platform.

For Platforms, the importer is capable of importing each game along with any additional apps, images, and most of the metadata fields (i.e. Title, Description, etc, see below).
## Function
This utility makes use of its sister project [CLIFp (Command-line Interface for Flashpoint)](https://github.com/oblivioncth/CLIFp) to allow LaunchBox to actually start and exit the games correctly. It is automatically deployed into your Flashpoint installation (updated if necessary) at the end of a successful import and the latest version of CLIFp will be included in each release of this utility so it is not generally something the end-user needs to concern themselves with.

Before making any changes to your LaunchBox collection any XML files that will be altered are automatically backed up (only one backup is maintained at once so any previous backup will be overwritten) and if any unrecoverable errors occur during the import any partial changes are reverted and the backups are restored; however, while LaunchBox itself also makes periodic backups of your XML data **it is strongly suggested that  you consider making a manual backup of your LaunchBox\Data folder to be safe.** No responsibility is held for the loss of data due to use of this tool.

OFILb can safely be used multiple times on the same collection to update the selected Platforms and Playlists if that have already been imported previously. The method with which to handle existing entries is selected within the program before each import.

The import time will vary, correlated with how many Platforms/Playlists you have selected. Importing the entire collection usually takes 5-10 minutes but can take longer with a more basic PC.

You will still be able to use the standard Flashpoint launcher as normal after completing an import.

# Compatability
### Flashpoint Infinity/Flashpoint Ultimate
This tool was made with the express purpose of using it with Flashpoint Ultimate (i.e. all games/animations pre-downloaded), but since the 0.2 rewrite of CLIFp it should work with Infinity as well. Just note that use with Infinity is less rigorously tested.

### General
While testing for 100% compatibility is infeasible given the size of Flashpoint, OFILb was designed with full compatibility in mind; however, the importer itself only provides access to the games and their related playlists within Flashpoint, not the animations, since LaunchBox is primarily a games frontend.

The ":message:" feature of Flashpoint, commonly used to automatically show usage instructions for some games before they are started, is supported. The entries that use it are added as additional-apps to their respective games as they once were when Flashpoint came packaged with LaunchBox. All messages are displayed in a pop-up dialog via CLIFp.

Viewing extras (which are simply a folder) is also supported and the corresponding additional apps that open these folders will be added when importing a platform.

Since Flashpoint originally used LaunchBox as its launcher most fields within Flashpoint have a one-to-one equivalent (or close enough equivalent) LaunchBox field. That being said there are a few fields that are unique to Flashpoint that do not have matching field and so they are simply excluded during the import, resulting in a relatively minor loss of information for each game in your collection.

### Version Matching
Each release of this application targets a specific version or versions of BlueMaxima's Flashpoint and while newer releases will sometimes contain general improvements to functionality, they will largely be created to match the changes made between each Flashpoint release and therefore maintain compatibility. These matches are shown below:
| OFILb Version |Included CLIFp Version | Target Flashpoint Version |
|--|--|--|
| 0.1 | 0.1 | 8.1 ("Spirit of Adventure") |
| 0.1.1 | 0.1.1 | 8.2 ("Approaching Planet Nine") |
| 0.1.2, 0.1.2.1 | 0.3 | 8.2 ("Approaching Planet Nine") |
| 0.1.3 | 0.3.2 | 9.0 ("Glorious Sunset") |

Using a version of OFILb that does not target the version of Flashpoint you wish to use it with is highly discouraged as some features may not work correctly or at all and in some cases the utility may fail to function entirely or even damage the Flashpoint install it is used with.

### Metadata Fields

Currently the following fields in LaunchBox will be populated for each game, which is limited by what is available within Flashpoint:

- Title
- Series
- Developer
- Publisher
- Platform
- Sort Title
- Date Added
- Date Modified
- Broken Flag
- Play Mode
- Status
- Region
- Notes
- Source
- Release Date
- Version

## Usage
### Primary Usage
 1. Ensure Flashpoint and LaunchBox are both not running
 2. Manually specify or browse for the path to your LaunchBox install, the utility will let you know if there are any problems. If everything is OK the icon next to the install path will change to a green check
 3. Manually specify or browse for the path to your Flashpoint install, the utility will let you know if there are any problems. If everything is OK the icon next to the install path will change to a green check
 4. The lists of available Platforms and Playlists will quickly load
 5. Select which Platforms and Playlists you want to import. Existing entries that are considered an update will be highlighted in green
5. If any entries you have selected are for updates you may select update mode settings. These are described with the nearby Help button in the program, but here is a basic overview of their differences:
    - (Exclusive) New Only - Only adds new games
    - (Exclusive) New & Existing - Adds new games and updates the non-user specific metadata for games already in your collection
    - (Applies to either of the above) Remove Missing - Removes any games from your collection for the selected Platforms that are no longer in Flashpoint
6. Select a method to handle game images. These are described with the nearby Help button in the program, but here is a basic overview of their differences:
    - LaunchBox Copy - Copies all relevant images from Flashpoint into your LaunchBox install
    - LaunchBox Symlink - Creates a symbolic link to all relevant images from Flashpoint into your LaunchBox install
    - Flashpoint Symlink - Moves all relevant images from Flashpoint into your LaunchBox install and then creates a symbolic link to each of them back in their original locations within your Flashpoint install
7. Press the "Start Import" button

The symbolic link related options for handling images require the importer to be run as an administrator or for you to enable [Developer mode](https://www.howtogeek.com/292914/what-is-developer-mode-in-windows-10/#:~:text=How%20to%20Enable%20Developer%20Mode,be%20put%20into%20Developer%20Mode.) within Windows 10

**Example:**

![OFILb Example Usage](https://i.imgur.com/2MZKwrJ.png)

### Other Features
 - If for whatever reason you want to only deploy or update CLIFp there is an option for doing so in the Tools menu
 - You can select whether or not you want to include "Explicit" games in each import session using the relevant check-able option in the Tools menu
 - The playlist import feature is "smart" in the sense that it won't include games that you aren't importing. So if you only want to import the Flash platform for example and a couple playlists, you wont have to worry about useless entries in the playlist that point to games from other platforms
 
## Limitations
 - Although general compatibility is quite high, compatibility with every single title cannot be assured. Issues with a title or group of titles will be fixed as they are discovered
 - The "smart" feature of the Playlist import portion of the tool has the drawback that only games that were included in the same import will be considered for that playlist. If you previously imported a Platform and now want to import a Playlist that contains games from that Platform you must make sure you select it again for it to be updated/re-imported in order for those games to be added to that Playlist. This is to avoid significantly decreasing Playlist import speed, but a solution to also scan for existing game imports when parsing playlists will likely be added as an opt-in option in the future.

## Source
This tool was written in C++ 17 along with Qt 5 and currently only targets Windows Vista and above; however, this tool can easily be ported to Linux with minimal changes, though to what end I am not sure since this is for a Windows application. The source includes an easy-to-use .pro file if you wish to build the application in Qt Creator and the available latest release was compiled in Qt Creator using MSVC 2019 and a static compilation of Qt 5.15.0. Other than a C++ 17 capable compiler and Qt 5.15.x+ all files required to compile this software are included, with the exception of a standard make file.

All functions/variables under the "Qx" (QExtended) namespace belong to a small, personal library I maintain to always have access to frequently used functionality in my projects. A pre-compiled static version of this library is provided with the source for this tool. If anyone truly needs it, I can provide the source for this library as well.
