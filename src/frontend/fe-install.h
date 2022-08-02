#ifndef FE_INSTALL_H
#define FE_INSTALL_H

// Qt Includes
#include <QDir>

// Project Includes
#include "fe-installfoundation.h"

//-Macros-------------------------------------------------------------------------------------------------------------------
#define REGISTER_FRONTEND(fe_name, fe_install, fe_icon_path, fe_helpUrl) \
    class fe_install##Factory : public Fe::InstallFactory \
    { \
    public: \
        fe_install##Factory() \
        { \
            Install::Entry entry { \
                .factory = this, \
                .iconPath = fe_icon_path, \
                .helpUrl = fe_helpUrl \
            }; \
            Fe::Install::registerInstall(fe_name, entry); \
        } \
        virtual std::shared_ptr<Fe::Install> produce(QString installPath) const { return std::make_shared<fe_install>(installPath); } \
    }; \
    static fe_install##Factory _##install##Factory;

namespace Fe
{

class InstallFactory
{
//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual std::shared_ptr<Install> produce(QString installPath) const = 0;
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
    Install(QString installPath);

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    // NOTE: Registry put behind function call to avoid SIOF since otherwise initialization of static registry before calls to registerFrontend would not be guaranteed
    static QMap<QString, Entry>& registry();
    static void registerInstall(QString name, Entry entry);
    static std::shared_ptr<Install> acquireMatch(const QString& installPath);

//-Instance Functions---------------------------------------------------------------------------------------------------------
protected:
    // Install management
    virtual void nullify() override;
    virtual Qx::GenericError populateExistingDocs() override = 0;
    virtual QString translateDocName(const QString& originalName, DataDoc::Type type) const override;

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
    virtual QString executablePath() const = 0;
    virtual QList<ImageMode> preferredImageModeOrder() const = 0;
    bool supportsImageMode(ImageMode imageMode) const;
    virtual QString versionString() const;

    // Import stage notifier hooks
    virtual Qx::GenericError preImport(const ImportDetails& details);
    virtual Qx::GenericError postImport();
    virtual Qx::GenericError prePlatformsImport();
    virtual Qx::GenericError postPlatformsImport();
    virtual Qx::GenericError preImageProcessing(QList<ImageMap>& workerTransfers, ImageSources bulkSources);
    virtual Qx::GenericError postImageProcessing();
    virtual Qx::GenericError prePlaylistsImport();
    virtual Qx::GenericError postPlaylistsImport();

    // Doc handling
    Qx::GenericError checkoutPlatformDoc(std::unique_ptr<PlatformDoc>& returnBuffer, QString name);
    Qx::GenericError checkoutPlaylistDoc(std::unique_ptr<PlaylistDoc>& returnBuffer, QString name);
    Qx::GenericError commitPlatformDoc(std::unique_ptr<PlatformDoc> platformDoc);
    Qx::GenericError commitPlaylistDoc(std::unique_ptr<PlaylistDoc> playlistDoc);

    // Image handling
    // NOTE: The image paths provided here can be null (i.e. images unavailable). Handle accordingly in derived.
    virtual void processDirectGameImages(const Game* game, const Fe::ImageSources& imageSources) = 0;
};

}
#endif // FE_INSTALL_H
