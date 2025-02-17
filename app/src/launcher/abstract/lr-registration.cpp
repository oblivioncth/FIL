// Unit Include
#include "lr-registration.h"

namespace Lr
{
//===============================================================================================================
// Registry
//===============================================================================================================

//-Class Functions--------------------------------------------------
//Protected:
Registry::Entry* Registry::registerInstall(Entry&& entry)
{
    Q_ASSERT(!smRegistry.contains(entry.name));
    return &smRegistry.insert(entry.name, std::move(entry)).value();
}

//Public:
std::unique_ptr<IInstall> Registry::acquireMatch(const QString& installPath)
{
    // Check all installs against path and return match if found
    for(const auto& entry : std::as_const(smRegistry))
    {
        std::unique_ptr<IInstall> possibleMatch = entry.make(installPath);

        if(possibleMatch->isValid())
            return possibleMatch;
    }

    // Return nullptr on failure to find match
    return nullptr;
}

QUrl Registry::helpUrl(QStringView name)
{
    auto rItr = smRegistry.constFind(name);
    return rItr != smRegistry.cend() ? rItr->helpUrl : QUrl();
}

QMapIterator<QStringView, Registry::Entry> Registry::entries() { return smRegistry; }

}
