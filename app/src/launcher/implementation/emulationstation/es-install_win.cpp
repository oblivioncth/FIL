// Unit Include
#include "es-install.h"

// Qx Includes
#include <qx/windows/qx-filedetails.h>

namespace Es
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

QString Install::versionFromExecutable() const
{
    Qx::FileDetails exeDetails = Qx::FileDetails::readFileDetails(mExeFile.fileName());
    if(!exeDetails.isNull())
    {
        Qx::VersionNumber ver = exeDetails.productVersion();
        if(!ver.isNull())
            return ver.first(3).toString();
    }

    return QString();
}

}
