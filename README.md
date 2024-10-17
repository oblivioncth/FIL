# FIL (Flashpoint Importer for Launchers)
<img align="left" src="https://i.imgur.com/WZbXSO2.png" width=25%>

FIL  is an importer tool for several launchers/frontends that allows one to add platforms and playlists from [Flashpoint Archive](https://flashpointarchive.org/) to their collection. It is fully automated and only requires the user to provide the paths to their launcher and Flashpoint installs, choose which Platforms/Playlists they wish to import, and select between a few import mode options. Once the import is started the current progress is displayed and any errors that occur are shown to the user, with resolvable errors including a prompt for what the user would like to do. After the process has completed, the specified launcher can be started and the games from Flashpoint can be played like those from any other Platform.

For Platforms, the importer is capable of importing each game/animation along with any additional apps, images, and most of the metadata fields (i.e. Title, Description, etc, see below).

Checkout **[Usage (Primary)](#usage-primary)** to get started.

[![Dev Builds](https://github.com/oblivioncth/FIL/actions/workflows/build-project.yml/badge.svg?branch=dev)](https://github.com/oblivioncth/FIL/actions/workflows/build-project.yml)

## Function
This utility makes use of its sister project [CLIFp (Command-line Interface for Flashpoint)](https://github.com/oblivioncth/CLIFp) to allow launchers to actually start and exit the games correctly. It is automatically deployed into your Flashpoint installation (updated if necessary) at the end of a successful import and the latest version of CLIFp will be included in each release of this utility so it is not generally something the end-user needs to concern themselves with.

Before making any changes to your collection, any datafiles that will be altered are automatically backed up (only one backup is maintained at once so any previous backup will be overwritten) and if any unrecoverable errors occur during the import any partial changes are reverted and the backups are restored; however, **it is strongly suggested that  you consider making a manual backup of your launcher's configuration to be safe.** No responsibility is held for the loss of data due to use of this tool.

FIL can safely be used multiple times on the same collection to update the selected Platforms and Playlists if that have already been imported previously. The method with which to handle existing entries is selected within the program before each import.

The import time will vary, correlated with how many Platforms/Playlists you have selected, but more significantly the image mode you choose, which is expanded on later. Importing the entire collection usually takes 5-10 minutes with the recommended settings but can take longer with a more basic PC. The vast majority of the processing time is due to the plethora of images that have to be copied/symlinked when games processed so the speed of your storage device is the most significant factor. Running the importer for updates should be significantly faster it first checks to see if the source image from the new import source is actually different than your current one before copying/linking it.

You will still be able to use the standard Flashpoint launcher as normal after completing an import.

# Compatability

### Supported Launchers
 - [LaunchBox](https://www.launchbox-app.com/)
 - [AttractMode](http://attractmode.org/)

### Flashpoint
While testing for 100% compatibility is infeasible given the size of Flashpoint, FIL was designed with full compatibility in mind.

The ":message:" feature of Flashpoint, commonly used to automatically show usage instructions for some games before they are started, is supported. The entries that use it are added as additional-apps to their respective games as they once were when Flashpoint came packaged with LaunchBox. All messages are displayed in a pop-up dialog via CLIFp.

Viewing extras (which are simply a folder) is also supported and the corresponding additional apps that open these folders will be added when importing a platform.

Each metadata field (i.e. Title, Author, etc.) is matched to the closest equivalent of a given launcher, or a custom field if there is no near equivalent and the launcher supports them; otherwise, the field will be omitted.

Both Flashpoint Ultimate and Flashpoint Infinity are supported.

### Version Matching
Each release of this application targets a specific version series of Flashpoint Archive, which are composed of a major and minor version number, and are designed to work with all Flashpoint updates within that series. For example, a FIL release that targets Flashpoint 10.1 is intended to be used with any version of flashpoint that fits the scheme `10.1.x.x`, such as `10.1`, `10.1.0.3`, `10.1.2`, etc, but **not** `10.2`.

Using a version of FIL that does not target the version of Flashpoint you wish to use it with is highly discouraged as some features may not work correctly or at all and in some cases the utility may fail to function entirely or even damage the Flashpoint install it is used with.

The title of each [release](github.com/oblivioncth/FIL/releases) will indicate which version of Flashpoint it targets.

## Launcher Specific Details
*If enough frontends are added this section will likely be converted into a wiki.*

--------------------------------------------------------------------------------------------------
**LaunchBox**

The import strategy for LaunchBox results in a setup that is straightforward and very similar to when Flashpoint Archive used LaunchBox as its frontend. Platforms to platforms, playlists to playlists, games to games, additional apps to additional apps, and so forth.

Each platform is grouped within the platform category "Flashpoint".

All entry metadata is converted to its nearest LaunchBox equivalent, with nearly all fields being covered. One minor exception is the Flashpoint "Language" field, as it is added as a LaunchBox Custom Field, which requires a premium license to see.

Everything should work out-of-the-box after an import.

--------------------------------------------------------------------------------------------------
**AttractMode**

Summary:
 - Everything is considered to be tied to the platform/system "Flashpoint", as well as an emulator by the same name
 - All selections are imported to a master "Flashpoint" romlist
 - A tag list is created for each Platform and Playlist with the prefixes "[Platform]" and "[Playlist]" respectively
 - Game descriptions are added as overviews
 - After each import, if a Display titled "Flashpoint" is not present in your config, one will be created with sensible defaults
 - A Flashpoint system marquee is provided
 - Additional applications are added as romlist entries using the following naming scheme for their title `[parent_game_name] |> [add_app_name]`
 - Title images are added as 'flyers' and screenshots are added as 'snaps'
 - Everything should work out-of-the-box after an import

Details:

The default Display entry will only be created if it's missing, allowing you to customize it as you see fit afterwards; however, the Platform/Playlist specific filters will always be updated to match your selections from the most recent import. Alternatively you can simply make your own Display entry under a different name and leave the default alone (as well as potentially.

The default sort of all Display filters uses the 'AltTitle' field, which is based on Flashpoint's 'sortTitle' field. This guarantees the that all games appear in the same order as they do within Flashpoint and that  additional applications appear directly under their parent games.

The romlist fields are mapped as follows (AttractMode `->` Flashpoint):

 - Name `->` Title ID
 - Title `->` Title
 - Platform `->` Platform
 - Emulator `->` "Flashpoint"
 - CloneOf `->` Parent Title ID (if an additional app)
 - Year `->` Release date-time (date portion only)
 - Manufacturer `->` Developer
 - Players `->` Play Mode
 - Status `->` Status
 - AltTitle `->` Sort Title (use for correct sorting)
 - Series `->` Series
 - Language `->` Language

Any fields not listed are unused or set to a general default.

All of the default AttractMode layouts don't work particularly well with Flashpoint. The main issues are:

 - Not enough space for many titles, especially additional apps
 - Some layouts showing the "AltTitle" of each entry beside them, wasting further space since that field is used for sorting purposes in this use case and isn't really intended to be displayed.
 - Most layouts stretch the images instead of preserving their aspect ratio. This can be changed for some layouts, though since layout settings are global this will affect all of your displays so I did not configure the importer to make this change automatically.
 - No layouts actually display overviews

For this reason it is recommended to use a third-party layout that avoids these issues as best as possible. I cannot recommend one as at this time I do not use AttractMode personally. In the future I may try creating a simple one that is ideal for Flashpoint, thought I cannot promise this. If someone wants to share one they end up creating or recommend an existing one that works well that would be appreciated.

Given that AttractMode is highly customizable and designed to encourage each user to have a unique-to-them setup, ultimately you can do whatever you want with the resultant romlist, tag lists, and overviews. The default Display/Filters are just for getting started.

## Usage (Primary)

 **Before using FIL, be sure to have ran Flashpoint through its regular launcher at least once**

 1. Download and run the latest [release](https://github.com/oblivioncth/FIL/releases) (the static variant is recommended)
 2. Ensure Flashpoint and the launcher are both not running
 3. Manually specify or browse for the path to your launcher install, the utility will let you know if there are any problems. If everything is OK the icon next to the install path will change to a green check
 4. Manually specify or browse for the path to your Flashpoint install, the utility will let you know if there are any problems. If everything is OK the icon next to the install path will change to a green check
 5. The lists of available Platforms and Playlists will quickly load
 6. Select which Platforms and Playlists you want to import. Existing entries that are considered an update will be highlighted in green
 7. If importing Playlists, select a Playlist Game Mode. These are described with the nearby Help button in the program, but here is a basic overview of their differences:
	 - **Selected Platforms Only** - Only games that are present within the selected platforms will be included
	 - **Force All** - All games in the playlist will be included, importing portions of unselected platforms as required
 8. If any entries you have selected are for updates you may select update mode settings. These are described with the nearby Help button in the program, but here is a basic overview of their differences:
    - (Exclusive) **New Only** - Only adds new games
    - (Exclusive) **New & Existing** - Adds new games and updates the non-user specific metadata for games already in your collection
    - (Applies to either of the above) **Remove Missing** - Removes any games from your collection for the selected Platforms that are no longer in Flashpoint
 9. Select a method to handle game images. These are described with the nearby Help button in the program, but here is a basic overview of their differences:
    - **Copy** - Copies all relevant images from Flashpoint into your launcher install (slow import)
    - **Reference** - Changes your launcher install configuration to directly use the Flashpoint images in-place (slow image refresh)
    - **Symlink** - Creates a symbolic link to all relevant images from Flashpoint into your launcher install. Overall the best option

 10. Press the "Start Import" button

The symbolic link related options for handling images require the importer to be run as an administrator or for you to enable [Developer mode](https://www.howtogeek.com/292914/what-is-developer-mode-in-windows-10/#:~:text=How%20to%20Enable%20Developer%20Mode,be%20put%20into%20Developer%20Mode.) within Windows 10

**Example:**

![FIL Example Usage](https://i.imgur.com/YrlecCK.png)

## Usage (Tools)

### Tag Filter
The tag filter editor allows you to customize which titles will be imported based on their tags.

![Tag Filter](https://i.imgur.com/EzEd0H1.png)

Tags are listed alphabetically, nested under their categories names so that you can select or unselect an entire category easily. Exclusions take precedence, so if a title features a single tag that you have unselected it will not be included in the import.

All tags are included by default.

### Image Downloading
Only available when using Flashpoint Infinity, the "Force Download Images" option will download the cover art and screenshot for each imported title if they have not yet been retrieved through normal use of Infinity.

**WARNING:** The Flashpoint Infinity client was only designed to download images gradually while scrolling through titles within its interface, and so the Flashpoint image server has bandwidth restrictions that severely limit the practicality of downloading a large number of images in bulk. Therefore, it is recommended to only use this feature when using Infinity to access a small subset of the Flashpoint collection, such as a specific playlist, or curated list of favorites. Otherwise, if having all game images available in your launcher is important to you, you should be using Ultimate, or be prepared to wait an **extremely** long time.

### Animations
Since most launchers are game oriented, animations are ignored by default. If you wish to include them you can do so by selecting the "Include Animations" option.

### CLIFp Distribution
This tool automatically handles installing/updating the command-line interface Flashpoint client as needed; however, if for whatever reason you deem it necessary/useful to manually insert a copy of FIL's bundled CLIFp version, you can do so using the "Deploy CLIFp" option.

## Other Features
 - The playlist import feature is "smart" in the sense that it won't include games that you aren't importing. So if you only want to import the Flash platform for example and a couple playlists, you wont have to worry about useless entries in the playlist that point to games from other platforms you didn't import. This of course does not apply if you are using the "Force All" playlist game mode.

## Limitations
 - Although general compatibility is quite high, compatibility with every single title cannot be assured. Issues with a title or group of titles will be fixed as they are discovered.
 - The "smart" feature of the Playlist import portion of the tool has the drawback that only games that were included in the same import will be considered for that playlist. If you previously imported a Platform and now want to import a Playlist that contains games from that Platform you must make sure you select it again for it to be updated/re-imported in order for those games to be added to that Playlist. Alternatively, you can use the "Force All" playlist game mode, but this will also possibly add new platforms you did not previously import.
- If you are using Infinity you will be able to play any game that is imported, even if it hasn't been played yet in Flashpoint; images for the games however will not be present until they've been seen/loaded in Flashpoint at least once and require the importer to be ran again afterwards, unless you use the "Force Download Images" option.

## Source

### Summary

 - C++20
 - CMake >= 3.24.0
 - Targets Windows 10 and above

### Dependencies
- Qt6
- [Qx](https://github.com/oblivioncth/Qx/)
- [libfp](https://github.com/oblivioncth/libfp/)
- [CLIFp](https://github.com/oblivioncth/CLIFp)
- [Neargye's Magic Enum](https://github.com/Neargye/magic_enum)
- [OBCMake](https://github.com/oblivioncth/OBCmake)

### Details
The source for this project is managed by a sensible CMake configuration that allows for straightforward compilation and consumption of its target(s), either as a sub-project or as an imported package. All required dependencies except for Qt6 are automatically acquired via CMake's FetchContent mechanism.

### Building
Ensure Qt6 is installed and locatable by CMake (or alternatively use the `qt-cmake` script that comes with Qt in-place of the`cmake` command).

Right now, a static build is required in order for CLIFp to work correctly.

Should work with MSVC, MINGW64, clang, and gcc.

```
# Acquire source
git clone https://github.com/oblivioncth/FIL

# Configure (ninja optional, but recommended)
cmake -S FIL -B build-FIL -G "Ninja Multi-config"

# Build
cmake --build build-FIL

# Install
cmake --install build-FIL

# Run
cd "build-FIL/out/install/bin"
fil
```
