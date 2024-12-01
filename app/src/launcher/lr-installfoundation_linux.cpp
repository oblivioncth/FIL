// Unit Include
#include "lr-installfoundation.h"

// Qt Includes
#include <QFile>

namespace Lr
{
//===============================================================================================================
// InstallFoundation
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Private:
void InstallFoundation::ensureModifiable(const QString& filePath)
{
    QFile f(filePath);;
    f.setPermissions(f.permissions() | QFile::WriteOwner | QFile::WriteGroup);
}

}
