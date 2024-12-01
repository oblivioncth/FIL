#ifndef IMPORT_SETTINGS_H
#define IMPORT_SETTINGS_H

// Qt Includes
#include <QString>
#include <QList>

// libfp Includes
#include <fp/fp-db.h>

namespace Import
{

// Enums
enum class Install{ Launcher, Flashpoint };
enum class UpdateMode {OnlyNew, NewAndExisting};
enum class ImageMode {Copy, Reference, Link};
enum class PlaylistGameMode {SelectedPlatform, ForceAll};

// Structs
struct Importee
{
    QString name;
    bool existing = false;
};

struct Selections
{
    QStringList platforms;
    QStringList playlists;
};

struct UpdateOptions
{
    UpdateMode importMode;
    bool removeObsolete;
};

struct OptionSet
{
    UpdateOptions updateOptions;
    ImageMode imageMode;
    bool downloadImages;
    PlaylistGameMode playlistMode;
    Fp::Db::InclusionOptions inclusionOptions;
};

}

#endif // IMPORT_SETTINGS_H
