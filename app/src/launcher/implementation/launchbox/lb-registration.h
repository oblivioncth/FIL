#ifndef LAUNCHBOX_REGISTRATION_H
#define LAUNCHBOX_REGISTRATION_H

#include "launcher/abstract/lr-registration.h"

namespace Lb {

class Install;
class PlatformDoc;
class PlatformDocReader;
class PlatformDocWriter;
class PlaylistDoc;
class PlaylistDocReader;
class PlaylistDocWriter;
class Game;
class AddApp;
class PlaylistHeader;
class PlaylistGame;

using LauncherId = Lr::Registrar<
    Install,
    PlatformDoc,
    PlatformDocReader,
    PlatformDocWriter,
    PlaylistDoc,
    PlaylistDocReader,
    PlaylistDocWriter,
    Game,
    AddApp,
    PlaylistHeader,
    PlaylistGame,
    u"LaunchBox",
    u":/launcher/LaunchBox/icon.svg",
    u"https://forums.launchbox-app.com/files/file/2652-obbys-flashpoint-importer-for-launchbox"
>;

}
#endif // LAUNCHBOX_REGISTRATION_H
