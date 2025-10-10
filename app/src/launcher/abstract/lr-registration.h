#ifndef LR_REGISTRATION_H
#define LR_REGISTRATION_H

// Qt Includes
#include <QUrl>

// Qx Includes
#include <qx/utility/qx-concepts.h>
#include <qx/utility/qx-stringliteral.h>

// Project Includes
#include "launcher/interface/lr-data-interface.h"
#include "launcher/interface/lr-install-interface.h"

#define REGISTER_LAUNCHER(LauncherId) static Lr::Register<LauncherId> reg;

namespace Lr
{
template<
    class Install,
    class Platform,
    class PlatformReader, // Voidable, if platforms are always overwritten in their entirety
    class PlatformWriter,
    class Playlist,
    class PlaylistReader, // Voidable, if playlists are always overwritten in their entirety
    class PlaylistWriter,
    class Game,
    class AddApp, // Voidable, if not using the BasicXXXDoc types
    class PlaylistHeader, // Voidable, if not using the BasicXXXDoc types
    class PlaylistGame, // Voidable, if not using the BasicXXXDoc types
    Qx::U16StringLiteral NameS,
    Qx::U16StringLiteral IconPathS,
    Qx::U16StringLiteral HelpUrlS
>
struct Registrar
{
    using InstallT = Install;
    using PlatformT = Platform;
    using PlatformReaderT = PlatformReader;
    using PlatformWriterT = PlatformWriter;
    using PlaylistT = Playlist;
    using PlaylistReaderT = PlaylistReader;
    using PlaylistWriterT = PlaylistWriter;
    using GameT = Game;
    using AddAppT = AddApp;
    using PlaylistHeaderT = PlaylistHeader;
    using PlaylistGameT = PlaylistGame;
    static constexpr QStringView Name = NameS;
    static constexpr QStringView IconPath = IconPathS;
    static constexpr QStringView HelpUrl = HelpUrlS;
};

/* This is shitty, but there is no good way to check if a type is a specialization of a template when
 * that template has non-type parameters, so here we just see if its equivalent based on its aliases
 */
template<typename T>
concept LauncherId = requires{
    typename T::InstallT;
    typename T::PlatformT;
    typename T::PlatformReaderT;
    typename T::PlatformWriterT;
    typename T::PlaylistT;
    typename T::PlaylistReaderT;
    typename T::PlaylistWriterT;
    typename T::GameT;
    typename T::AddAppT;
    typename T::PlaylistHeaderT;
    typename T::PlaylistGameT;
    T::Name;
    T::IconPath;
    T::HelpUrl;
};

namespace _detail
{

template<class I>
concept _valid_install = std::derived_from<I, IInstall>;

template<class P>
concept _valid_platform = std::derived_from<P, IPlatformDoc>;

template<class P>
concept _valid_playlist = std::derived_from<P, IPlaylistDoc>;

template<class R, class D>
concept _valid_reader = (std::derived_from<R, IDataDoc::Reader> && std::constructible_from<R, D*>) || std::same_as<R, void>;

template<class W, class D>
concept _valid_writer = std::derived_from<W, IDataDoc::Writer> && std::constructible_from<W, D*>;

template<class G>
concept _valid_game = std::derived_from<G, Game>;

template<class A>
concept _valid_addapp = std::derived_from<A, AddApp> || std::same_as<A, void>;

template<class P>
concept _valid_playlistheader = std::derived_from<P, PlaylistHeader> || std::same_as<P, void>;

template<class P>
concept _valid_playlistgame = std::derived_from<P, PlaylistGame> || std::same_as<P, void>;

}

// This needs a full examination anyway
template<typename T>
concept CompleteLauncherId =
    _detail::_valid_install<typename T::InstallT> &&
    _detail::_valid_platform<typename T::PlatformT> &&
    _detail::_valid_reader<typename T::PlatformReaderT, typename T::PlatformT> &&
    _detail::_valid_writer<typename T::PlatformWriterT, typename T::PlatformT> &&
    _detail::_valid_playlist<typename T::PlaylistT> &&
    _detail::_valid_reader<typename T::PlaylistReaderT, typename T::PlaylistT> &&
    _detail::_valid_writer<typename T::PlaylistWriterT, typename T::PlaylistT> &&
    _detail::_valid_game<typename T::GameT> &&
    _detail::_valid_addapp<typename T::AddAppT> &&
    _detail::_valid_playlistheader<typename T::PlaylistHeaderT> &&
    _detail::_valid_playlistgame<typename T::PlaylistGameT>;

template<typename Id>
concept HasPlatformReader = !std::same_as<typename Id::PlatformReaderT, void>;

template<typename Id>
concept HasPlaylistReader = !std::same_as<typename Id::PlaylistReaderT, void>;

class Registry
{
    template<CompleteLauncherId Id>
    friend class Register;
//-Structs---------------------------------------------------
public:
    struct Entry
    {
        QStringView name;
        std::unique_ptr<IInstall> (*make)(const QString&);
        QStringView iconPath;
        QUrl helpUrl;
    };

//-Class Variables-------------------------------------------------
private:
    static inline constinit QMap<QStringView, Entry> smRegistry;

//-Class Functions--------------------------------------------------
protected:
    static Entry* registerInstall(Entry&& entry);

public:
    [[nodiscard]] static std::unique_ptr<IInstall> acquireMatch(const QString& installPath);
    static QUrl helpUrl(QStringView name);
    static QMapIterator<QStringView, Entry> entries();
};

template<CompleteLauncherId Id>
class Register;

template<LauncherId Id>
class StaticRegistry
{
    template<CompleteLauncherId>
    friend class Register;
    static inline Registry::Entry* smEntry;

public:
    static inline QStringView name() { Q_ASSERT(smEntry); return smEntry->name; }
    static inline QStringView iconPath() { Q_ASSERT(smEntry); return smEntry->iconPath; }
    static inline QUrl helpUrl() { Q_ASSERT(smEntry); return smEntry->helpUrl; }
};

template<CompleteLauncherId Id>
class Register
{
    /* NOTE: This is used by the REGISTER_LAUNCHER() macro to cause launcher registration
     * at runtime, and to setup the StaticRegistry for the launcher of 'Id'. This has to
     * be done separately from Registrar because we need the definition of the launcher's
     * Install type to be available.
     */

private:
    static std::unique_ptr<IInstall> createInstall(const QString& path) {
        return std::make_unique<typename Id::InstallT>(path);
    }

public:
    Register()
    {
        StaticRegistry<Id>::smEntry = Registry::registerInstall(Registry::Entry{
            .name = Id::Name,
            .make = createInstall,
            .iconPath = Id::IconPath,
            .helpUrl = QUrl(Id::HelpUrl.toString())
        });
    }
};

}

#endif // LR_REGISTRATION_H
