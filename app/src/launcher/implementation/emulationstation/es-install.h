#ifndef EMULATIONSTATION_INSTALL_H
#define EMULATIONSTATION_INSTALL_H

// Project Includes
#include "launcher/abstract/lr-install.h"
#include "launcher/implementation/emulationstation/es-data.h"

namespace Es
{

class Install : public Lr::Install<LauncherId>
{
//-Class Variables--------------------------------------------------------------------------------------------------
private:
    // Paths
#ifdef _WIN32
    static inline const QString EXE_NAME = u"ES-DE.exe"_s;
#else
    static inline const QString EXE_NAME = u"es-de"_s;
#endif
    static inline const QString LOG_PATH = u"logs/es_log.txt"_s;

    // Support
    static inline const QList<Import::ImageMode> IMAGE_MODE_ORDER {
        // Import::ImageMode::Link,
        // Import::ImageMode::Copy,
        // Import::ImageMode::Reference
    };
    static inline const QRegularExpression LOG_VERSION_REGEX = QRegularExpression(uR"(.* Info:\s+ES-DE (?<ver>[0-9]\.[0-9]\.[0-9] ))"_s);

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QFileInfo mExeFile;
    QFileInfo mLogFile;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(const QString& installPath);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    QString versionFromExecutable() const;

public:
    // Info
    QList<Import::ImageMode> preferredImageModeOrder() const override;
    bool isRunning() const override;
    QString versionString() const override;
};

}

#endif // EMULATIONSTATION_INSTALL_H
