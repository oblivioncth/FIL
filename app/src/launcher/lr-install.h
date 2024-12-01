#ifndef LR_INSTALL_H
#define LR_INSTALL_H

// Qt Includes
#include <QDir>

// Project Includes
#include "launcher/lr-installfoundation.h"

//-Macros-------------------------------------------------------------------------------------------------------------------
#define REGISTER_LAUNCHER(lr_name, lr_install, lr_icon_path, lr_helpUrl) \
    class lr_install##Factory : public Lr::InstallFactory \
    { \
    public: \
        lr_install##Factory() \
        { \
            Install::Entry entry { \
                .factory = this, \
                .iconPath = lr_icon_path, \
                .helpUrl = lr_helpUrl \
            }; \
            Lr::Install::registerInstall(lr_name, entry); \
        } \
        virtual std::unique_ptr<Lr::Install> produce(const QString& installPath) const { return std::make_unique<lr_install>(installPath); } \
    }; \
    static lr_install##Factory _##install##Factory;

namespace Lr
{

class InstallFactory
{
//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual std::unique_ptr<Install> produce(const QString& installPath) const = 0;
};

class Install : public InstallFoundation
{
//-Structs------------------------------------------------------------------------------------------------------
public:
    struct Entry
    {
        const InstallFactory* factory;
        const QString* iconPath;
        const QUrl* helpUrl;
    };

//-Constructor---------------------------------------------------------------------------------------------------
public:
    Install(const QString& installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    // NOTE: Registry put behind function call to avoid SIOF since otherwise initialization of static registry before calls to registerLauncher would not be guaranteed
    static QMap<QString, Entry>& registry();
    static void registerInstall(const QString& name, const Entry& entry);
    [[nodiscard]] static std::unique_ptr<Install> acquireMatch(const QString& installPath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
protected:
    // Install management
    virtual void nullify() override;
    virtual Qx::Error populateExistingDocs() override = 0;

    // Doc Handling
    virtual std::shared_ptr<PlatformDoc::Reader> preparePlatformDocCheckout(std::unique_ptr<PlatformDoc>& platformDoc, const QString& translatedName) = 0;
    virtual std::shared_ptr<PlaylistDoc::Reader> preparePlaylistDocCheckout(std::unique_ptr<PlaylistDoc>& playlistDoc, const QString& translatedName) = 0;
    virtual std::shared_ptr<PlatformDoc::Writer> preparePlatformDocCommit(const std::unique_ptr<PlatformDoc>& document) = 0;
    virtual std::shared_ptr<PlaylistDoc::Writer> preparePlaylistDocCommit(const std::unique_ptr<PlaylistDoc>& document) = 0;

public:
    // Install management
    virtual void softReset() override;

    // Info
    virtual QString name() const = 0;
    virtual QList<Import::ImageMode> preferredImageModeOrder() const = 0;
    bool supportsImageMode(Import::ImageMode imageMode) const;
    virtual QString versionString() const;
    virtual bool isRunning() const = 0;

    // Import stage notifier hooks
    virtual Qx::Error preImport(const ImportDetails& details);
    virtual Qx::Error postImport();
    virtual Qx::Error prePlatformsImport();
    virtual Qx::Error postPlatformsImport();
    virtual Qx::Error preImageProcessing(QList<ImageMap>& workerTransfers, const ImageSources& bulkSources);
    virtual Qx::Error postImageProcessing();
    virtual Qx::Error prePlaylistsImport();
    virtual Qx::Error postPlaylistsImport();

    // Doc handling
    virtual QString translateDocName(const QString& originalName, DataDoc::Type type) const override;
    DocHandlingError checkoutPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, const QString& name);
    DocHandlingError checkoutPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, const QString& name);
    DocHandlingError commitPlatformDoc(std::unique_ptr<PlatformDoc> platformDoc);
    DocHandlingError commitPlaylistDoc(std::unique_ptr<PlaylistDoc> playlistDoc);

    // Image handling
    // NOTE: The image paths provided here can be null (i.e. images unavailable). Handle accordingly in derived.
    virtual void processDirectGameImages(const Game* game, const ImageSources& imageSources) = 0;

    // TODO: These might need to be changed to support launchers where the platform images are tied closely to the platform documents,
    // but currently none do this so this works.
    virtual QString platformCategoryIconPath() const; // Unsupported in default implementation, needs to return path with .png extension
    virtual std::optional<QDir> platformIconsDirectory() const; // Unsupported in default implementation
    virtual std::optional<QDir> playlistIconsDirectory() const; // Unsupported in default implementation
};

}
#endif // LR_INSTALL_H
