#ifndef CLIFP_H
#define CLIFP_H

#include <QUuid>

class CLIFp
{
// Class members
public:
    static inline const QString EXE_NAME = "CLIFp.exe";
    static inline const QString APP_ARG = R"(--exe="%1")";
    static inline const QString PARAM_ARG = R"(--param="%1")";
    static inline const QString EXTRA_ARG = R"(--extra-"%1")";
    static inline const QString MSG_ARG = R"(--msg="%1")";
    static inline const QString AUTO_ARG = R"(--auto="%1")";

// Class functions
public:
    static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    static QString parametersFromStandard(QUuid titleID);
};

#endif // CLIFP_H
