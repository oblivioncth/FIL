#ifndef ATTRACTMODE_REGISTRATION_H
#define ATTRACTMODE_REGISTRATION_H

#include "launcher/abstract/lr-registration.h"

namespace Am {

class Install;
class PlatformInterface;
class PlatformInterfaceWriter;
class PlaylistInterface;
class PlaylistInterfaceWriter;
class RomEntry;

using LauncherId = Lr::Registrar<
    Install,
    PlatformInterface,
    void, // No reading to be done for this interface (tag lists are always overwritten)
    PlatformInterfaceWriter,
    PlaylistInterface,
    void, // No reading to be done for this interface (tag lists are always overwritten)
    PlaylistInterfaceWriter,
    RomEntry,
    void,
    void,
    void,
    u"AttractMode",
    u":/launcher/AttractMode/icon.png",
    u"" // TODO: Add url
>;

}
#endif // ATTRACTMODE_REGISTRATION_H
