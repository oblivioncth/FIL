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

//-Constructor------------------------------------------------------------------------------------------------
//Public:
//TODO: Add handling of if root is used where we actually want sub directory ES-DE, make arg non const and morph it
Install::Install(const QString& installPath) :
    Lr::Install<LauncherId>(installPath),
    mExeFile(installPath + '/' + EXE_NAME),
    mLogFile(installPath + '/' + LOG_PATH)
{}

//-Instance Functions----------------------------------------------------------------------------------------------
//Private:

//Public:
//TODO: Remember to override softReset() if needed
QList<Import::ImageMode> Install::preferredImageModeOrder() const { return IMAGE_MODE_ORDER; }
bool Install::isRunning() const { return Qx::processIsRunning(mExeFile.fileName()); } // TODO: Use hardcoded name if we allow mExeFile to be set to the app image, which wont have the same name

QString Install::versionString() const
{
    // Limits to first 3 segments for consistency since that's what AttractMode seems to use

    // Try executable first if it's known
    if(QString exeVer = versionFromExecutable(); !exeVer.isEmpty())
        return exeVer;

    // Try log
    if(mLogFile.exists())
    {
        QFile log(mLogFile.absoluteFilePath());
        if(log.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString verLine = QString::fromLatin1(log.readLine());
            if(!verLine.isEmpty())
            {
                QRegularExpressionMatch sv = LOG_VERSION_REGEX.matchView(verLine);
                if(sv.hasMatch())
                {
                    Qx::VersionNumber ver = Qx::VersionNumber::fromString(sv.captured(u"ver"_s));
                    if(!ver.isNull())
                        return ver.toString();
                }
            }
        }
    }

    // Can't determine version
    return Lr::IInstall::versionString();
}
}
