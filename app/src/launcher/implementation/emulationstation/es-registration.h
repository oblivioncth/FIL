#ifndef EMULATIONSTATION_REGISTRATION_H
#define EMULATIONSTATION_REGISTRATION_H

#include "launcher/abstract/lr-registration.h"

namespace Es
{

class Install;
class Platform;
class PlatformReader;
class PlatformWriter;
class Playlist;
class PlaylistReader;
class PlaylistWriter;
class Game;

using LauncherId = Lr::Registrar<
    Install,
    Platform,
    PlatformReader,
    PlatformWriter,
    Playlist,
    PlaylistReader,
    PlaylistWriter,
    Game,
    void,
    void,
    void,
    u"ES-DE",
    u":/launcher/EmulationStation/icon.png",
    u"" // TODO: Add url
    >;

}

#endif // EMULATIONSTATION_REGISTRATION_H
