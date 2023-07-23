#ifndef CLIFP_H
#define CLIFP_H

// Qt Includes
#include <QUuid>

// Qx Includes
#include <qx/core/qx-versionnumber.h>

// libfp Includes
#include <fp/fp-install.h>

class CLIFp
{
// Class members
public:
    static inline const QString NAME = u"CLIFp"_s;
    static inline const QString EXE_NAME = NAME + u".exe"_s;
    static inline const QString PLAY_COMMAND = u"play"_s;
    static inline const QString RUN_COMMAND = u"run"_s;
    static inline const QString SHOW_COMMAND = u"show"_s;
    static inline const QString ID_ARG = R"(--id="%1")";
    static inline const QString APP_ARG = R"(--app="%1")";
    static inline const QString PARAM_ARG = R"(--param="%1")";
    static inline const QString MSG_ARG = R"(--msg="%1")";
    static inline const QString EXTRA_ARG = R"(--extra="%1")";

    static inline const QString ERR_FP_CANT_DEPLOY_CLIFP = u"Failed to deploy "_s + EXE_NAME + u" to the selected Flashpoint install.\n"_s
                                                           "\n"
                                                           "%1\n"
                                                           "\n"
                                                           "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually.";

// Class functions
public:
    static QString standardCLIFpPath(const Fp::Install& fpInstall);
    static bool hasCLIFp(const Fp::Install& fpInstall);
    static Qx::VersionNumber currentCLIFpVersion(const Fp::Install& fpInstall);
    static bool deployCLIFp(QString& errorMsg, const Fp::Install& fpInstall, QString sourcePath);

    static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    static QString parametersFromStandard(QUuid titleId);
};

#endif // CLIFP_H
