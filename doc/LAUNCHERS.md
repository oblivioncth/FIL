# Launcher Specific Details

## LaunchBox

The import strategy for LaunchBox results in a setup that is straightforward and very similar to when Flashpoint Archive used LaunchBox as its frontend. Platforms to platforms, playlists to playlists, games to games, additional apps to additional apps, and so forth.

Each platform is grouped within the platform category "Flashpoint".

All entry metadata is converted to its nearest LaunchBox equivalent, with nearly all fields being covered. One minor exception is the Flashpoint "Language" field, as it is added as a LaunchBox Custom Field, which requires a premium license to see.

Everything should work out-of-the-box after an import.

## AttractMode

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