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
    // Support
    static inline const QList<Import::ImageMode> IMAGE_MODE_ORDER {
        // Import::ImageMode::Link,
        // Import::ImageMode::Copy,
        // Import::ImageMode::Reference
    };

//-Instance Functions------------------------------------------------------------------------------------------------------
private:

public:
    // Info
    QList<Import::ImageMode> preferredImageModeOrder() const override;
};

}

#endif // EMULATIONSTATION_INSTALL_H
