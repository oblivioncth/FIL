#ifndef CLIFP_H
#define CLIFP_H

#include <QUuid>
#include "flashpoint/fp-install.h"
#include "qx.h"

class CLIFp
{
// Class members
public:
    static inline const QString EXE_NAME = "CLIFp.exe";
    static inline const QString PLAY_COMMAND = "play";
    static inline const QString RUN_COMMAND = "run";
    static inline const QString SHOW_COMMAND = "show";
    static inline const QString ID_ARG = R"(--id="%1")";
    static inline const QString APP_ARG = R"(--app="%1")";
    static inline const QString PARAM_ARG = R"(--param="%1")";
    static inline const QString MSG_ARG = R"(--msg="%1")";
    static inline const QString EXTRA_ARG = R"(--extra="%1")";

    static inline const QString ERR_FP_CANT_DEPLOY_CLIFP = "Failed to deploy " + EXE_NAME + " to the selected Flashpoint install.\n"
                                                           "\n"
                                                           "%1\n"
                                                           "\n"
                                                           "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually.";

// Class functions
public:
    static QString standardCLIFpPath(const FP::Install& fpInstall);
    static bool hasCLIFp(const FP::Install& fpInstall);
    static Qx::MMRB currentCLIFpVersion(const FP::Install& fpInstall);
    static bool deployCLIFp(QString& errorMsg, const FP::Install& fpInstall, QString sourcePath);

    static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    static QString parametersFromStandard(QUuid titleID);
};

#endif // CLIFP_H
