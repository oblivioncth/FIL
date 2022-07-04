// Unit Include
#include "clifp.h"

// Qx Includes
#include <qx/windows/qx-filedetails.h>

// libfp Includes
#include <fp/flashpoint/fp-install.h>

//===============================================================================================================
// CLIFp
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QString CLIFp::standardCLIFpPath(const Fp::Install& fpInstall) { return fpInstall.fullPath() + "/" + EXE_NAME; }

bool CLIFp::hasCLIFp(const Fp::Install& fpInstall)
{
    QFileInfo presentInfo(standardCLIFpPath(fpInstall));
    return presentInfo.exists() && presentInfo.isFile();
}

Qx::VersionNumber CLIFp::currentCLIFpVersion(const Fp::Install& fpInstall)
{
    if(!hasCLIFp(fpInstall))
        return Qx::VersionNumber();
    else
        return Qx::FileDetails::readFileDetails(standardCLIFpPath(fpInstall)).fileVersion();
}

bool CLIFp::deployCLIFp(QString& errorMsg, const Fp::Install& fpInstall, QString sourcePath)
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
    QFile internalCLIFp(sourcePath);
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

QString CLIFp::parametersFromStandard(QString originalAppPath, QString originalAppParams)
{
    QString clifpParam = "-q "; // Start with global quiet switch

    if(originalAppPath == Fp::Db::Table_Add_App::ENTRY_MESSAGE)
        clifpParam += SHOW_COMMAND + " " + MSG_ARG.arg(originalAppParams);
    else if(originalAppPath == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
         clifpParam += SHOW_COMMAND + " " + EXTRA_ARG.arg(originalAppParams);
    else
         clifpParam += RUN_COMMAND + " " +  APP_ARG.arg(originalAppPath) + " " + PARAM_ARG.arg(originalAppParams);

    return clifpParam;
}

QString CLIFp::parametersFromStandard(QUuid titleId) { return "-q " + PLAY_COMMAND + " " + ID_ARG.arg(titleId.toString(QUuid::WithoutBraces)); }
