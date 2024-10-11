// Unit Include
#include "am-install.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>

namespace Am
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

QString Install::versionFromExecutable() const
{
    QProcess attract;
    attract.setProgram(MAIN_EXE_PATH);
    attract.setArguments({"--version"});

    attract.start();
    if(!attract.waitForStarted(1000))
        return QString();

    if(!attract.waitForFinished(1000))
    {
        attract.kill(); // Force close
        attract.waitForFinished();

        return QString();
    }

    QString versionInfo = QString::fromLatin1(attract.readAllStandardOutput());
    QRegularExpressionMatch sv = Qx::RegularExpression::SEMANTIC_VERSION.match(versionInfo);
    return sv.hasMatch() ? sv.captured() : QString();
}

}
