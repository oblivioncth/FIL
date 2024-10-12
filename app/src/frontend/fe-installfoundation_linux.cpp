// Unit Include
#include "fe-installfoundation.h"

// Qt Includes
#include <QFile>

namespace Fe
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
