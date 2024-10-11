// Unit Include
#include "am-install.h"

// Qx Includes
#include <qx/windows/qx-filedetails.h>

namespace Am
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

QString Install::versionFromExecutable() const
{
    QString exePath = mMainExe.exists() ? mMainExe.fileName() :
                      mConsoleExe.exists() ? mConsoleExe.fileName() :
                      QString();

    if(!exePath.isEmpty())
    {
        Qx::FileDetails exeDetails = Qx::FileDetails::readFileDetails(exePath);
        if(!exeDetails.isNull())
        {
            Qx::VersionNumber ver = exeDetails.productVersion();
            if(!ver.isNull())
                return ver.first(3).toString();
        }
    }

    return QString();
}

}
