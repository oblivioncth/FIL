#ifndef EMULATIONSTATION_REGISTRATION_H
#define EMULATIONSTATION_REGISTRATION_H

#include "launcher/abstract/lr-registration.h"

namespace Es
{

class Install;
class Gamelist;
class GamelistReader;
class GamelistWriter;
class Collection;
class CollectionReader;
class CollectionWriter;
class Game;
class PlaylistHeader;
class PlaylistGame;

using LauncherId = Lr::Registrar<
    Install,
    Gamelist,
    GamelistReader,
    GamelistWriter,
    Collection,
    CollectionReader,
    CollectionWriter,
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
