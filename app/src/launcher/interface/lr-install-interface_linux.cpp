// Unit Include
#include "lr-install-interface.h"

// Qt Includes
#include <QFile>

namespace Lr
{
//===============================================================================================================
// IInstall
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Private:
void IInstall::ensureModifiable(const QString& filePath)
{
    QFile f(filePath);;
    f.setPermissions(f.permissions() | QFile::WriteOwner | QFile::WriteGroup);
}

}
