// Unit Include
#include "clifp.h"

// Qx Includes
#ifdef _WIN32
#include <qx/windows/qx-filedetails.h>
#endif

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "project_vars.h"

//===============================================================================================================
// CLIFp
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
Qx::VersionNumber CLIFp::internalVersion()
{
    static Qx::VersionNumber v = Qx::VersionNumber::fromString(PROJECT_BUNDLED_CLIFP_VERSION).normalized();
    return v;
}

Qx::VersionNumber CLIFp::installedVersion(const Fp::Install& fpInstall)
{
    if(!hasCLIFp(fpInstall))
        return Qx::VersionNumber();
    else
    {
#ifdef _WIN32
        return Qx::FileDetails::readFileDetails(standardCLIFpPath(fpInstall)).fileVersion().normalized();
#else
        /* TODO: For now on Linux we just return a null version so that deployment always
         * occurs. Eventually, find a good way to grab version info from the installed ELF.
         *
         * Currently, we can't run it since it doesn't output to console, and there is no
         * standardized way to embed the info as part of the ELF structure.
         */
        return Qx::VersionNumber();
#endif
    }
}

QString CLIFp::standardCLIFpPath(const Fp::Install& fpInstall) { return fpInstall.dir().absoluteFilePath(EXE_NAME); }

bool CLIFp::hasCLIFp(const Fp::Install& fpInstall)
{
    QFileInfo presentInfo(standardCLIFpPath(fpInstall));
    return presentInfo.exists() && presentInfo.isFile();
}

bool CLIFp::deployCLIFp(QString& errorMsg, const Fp::Install& fpInstall)
{
    // Delete existing if present
    QFile clifp(standardCLIFpPath(fpInstall));
    QFileInfo fileInfo(clifp);
    if(fileInfo.exists() && fileInfo.isFile())
    {
        if(!clifp.remove())
        {
            errorMsg = clifp.errorString();
            return false;
        }
    }

    // Deploy new
    QFile internalCLIFp(u":/file/clifp"_s);
    if(!internalCLIFp.copy(clifp.fileName()))
    {
        errorMsg = internalCLIFp.errorString();
        return false;
    }

    // Remove default read-only state
    clifp.setPermissions(QFile::ReadOther | QFile::WriteOther);

    // Return true on
    return true;
}

QString CLIFp::parametersFromStandard(QStringView originalAppPath, QStringView originalAppParams)
{
    QString clifpParam = u"-q "_s; // Start with global quiet switch

    if(originalAppPath == Fp::Db::Table_Add_App::ENTRY_MESSAGE)
        clifpParam += SHOW_COMMAND + ' ' + MSG_ARG.arg(originalAppParams);
    else if(originalAppPath == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
        clifpParam += SHOW_COMMAND + ' ' + EXTRA_ARG.arg(originalAppParams);
    else
        clifpParam += RUN_COMMAND + ' ' +  APP_ARG.arg(originalAppPath) + ' ' + PARAM_ARG.arg(originalAppParams);

    return clifpParam;
}

QString CLIFp::parametersFromStandard(const QUuid& titleId) { return u"-q "_s + PLAY_COMMAND + ' ' + ID_ARG.arg(titleId.toString(QUuid::WithoutBraces)); }
QString CLIFp::parametersFromStandard(const QString& titleId) { return u"-q "_s + PLAY_COMMAND + ' ' + ID_ARG.arg(titleId); }
