// Unit Include
#include "es-install.h"

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/core/qx-versionnumber.h>

namespace Es
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

QString Install::versionFromExecutable() const
{
    // Try system search based execution if exe wasn't found
    QString ePath = mExeFile.exists() ? mExeFile.fileName() : EXE_NAME;
    Qx::ExecuteResult res = Qx::execute(ePath, {u"--version"_s}, 1000);
    if(res.exitCode == 0)
    {
        static const QRegularExpression re(uR"(ES-DE (?<ver>[0-9]\.[0-9].[0-9]) )"_s);
        QRegularExpressionMatch sv = re.matchView(res.output);
        if(sv.hasMatch())
        {
            Qx::VersionNumber ver = Qx::VersionNumber::fromString(sv.captured(u"ver"_s));
            if(!ver.isNull())
                return ver.toString();
        }
    }

    return QString();
}

}
