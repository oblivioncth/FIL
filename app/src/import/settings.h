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

class ImagePaths
{
//-Instance Members--------------------------------------------------------------------------------------------------
private:
    QString mLogoPath;
    QString mScreenshotPath;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    ImagePaths();
    ImagePaths(const QString& logoPath, const QString& screenshotPath);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isNull() const;
    QString logoPath() const;
    QString screenshotPath() const;
    void setLogoPath(const QString& path);
    void setScreenshotPath(const QString& path);
};

}

#endif // IMPORT_SETTINGS_H
