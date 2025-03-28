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
#ifdef _WIN32
    static inline const QString EXE_NAME = NAME + u".exe"_s;
#else
    static inline const QString EXE_NAME = u"clifp"_s;
#endif
    static inline const QString PLAY_COMMAND = u"play"_s;
    static inline const QString RUN_COMMAND = u"run"_s;
    static inline const QString SHOW_COMMAND = u"show"_s;
    static inline const QString ID_ARG = uR"(--id="%1")"_s;
    static inline const QString APP_ARG = uR"(--app="%1")"_s;
    static inline const QString PARAM_ARG = uR"(--param="%1")"_s;
    static inline const QString MSG_ARG = uR"(--msg="%1")"_s;
    static inline const QString EXTRA_ARG = uR"(--extra="%1")"_s;
    static inline const QString FULLSCREEN_SWITCH = uR"(--fullscreen)"_s;

    static inline const QString ERR_FP_CANT_DEPLOY_CLIFP = u"Failed to deploy "_s + EXE_NAME + u" to the selected Flashpoint install.\n"_s
                                                           "\n"
                                                           "%1\n"
                                                           "\n"
                                                           "If you choose to ignore this you will have to place CLIFp in your Flashpoint install directory manually.";

// Class functions
public:
    static Qx::VersionNumber internalVersion();
    static Qx::VersionNumber installedVersion(const Fp::Install& fpInstall);
    static QString standardCLIFpPath(const Fp::Install& fpInstall);
    static bool hasCLIFp(const Fp::Install& fpInstall);
    static bool deployCLIFp(QString& errorMsg, const Fp::Install& fpInstall);

    static QString parametersFromStandard(QStringView originalAppPath, QStringView originalAppParams);
    static QString parametersFromStandard(const QUuid& titleId);
    static QString parametersFromStandard(const QString& titleId);
};

#endif // CLIFP_H
