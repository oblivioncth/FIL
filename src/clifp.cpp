#include "clifp.h"
#include "flashpoint-install.h"

//===============================================================================================================
// CLIFp
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QString CLIFp::parametersFromStandard(QString originalAppPath, QString originalAppParams)
{
    if(originalAppPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE)
        return MSG_ARG.arg(originalAppParams) + " -q";
    else if(originalAppPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
        return EXTRA_ARG.arg(originalAppParams) + " -q";
    else
        return APP_ARG.arg(originalAppPath) + " " + PARAM_ARG.arg(originalAppParams) + " -q";
}

QString CLIFp::parametersFromStandard(QUuid titleID) { return AUTO_ARG.arg(titleID.toString(QUuid::WithoutBraces)) + " -q"; }
