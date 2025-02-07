#ifndef IMPORT_DETAILS_H
#define IMPORT_DETAILS_H

/* Although somewhat redundant with settings.h, this is a collection of
 * Import related info that is specifically collected to be shared with
 * Installs
 */

// Project Includes
#include "import/settings.h"

namespace Import
{

struct Details
{
    friend class Worker;
    Import::UpdateOptions updateOptions;
    Import::ImageMode imageMode;
    QString clifpPath;
    QList<QString> involvedPlatforms;
    QList<QString> involvedPlaylists;

    static Details current();

private:
    static constinit std::optional<Details> mCurrent;
    static void setCurrent(const Details& details);
    static void clearCurrent();
};

}

#endif // IMPORT_DETAILS_H
