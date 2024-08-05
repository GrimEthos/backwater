module;

#include <cinttypes>
#include <functional>
#include <system_error>

export module grim.arch.net;

import cpp.asio.tcp;
import cpp.memory;
import cpp.chrono;
import grim.arch.auth;

export namespace grim::net
{
    using                                   StrArg = const cpp::Memory &;
    using                                   StrOut = std::string *;
    using                                   ResultValue = uint8_t;
    constexpr std::endian                   ByteOrder = std::endian::big;

    enum class Result : ResultValue {
        Ok = 0,                             // 

        Arg = 0xf0,                         // an argument was invalid
        Access = 0xfa,                      // privs
        Route = 0xfb,                       // to session can't be reached
        Retry = 0xfc,                       // rate limiting
        More = 0xfd,                        // multi-part message, bind will invoke same callback multiple times
        Timeout = 0xfe,                     // timeout expired
        Unknown = 0xff
    };

    struct Message
    {
        uint64_t                            len     : 16; // data.length() / 8
        uint64_t                            moniker : 16;
        uint64_t                            bind    : 16;
        uint64_t                            type    : 8;
        uint64_t                            result  : 8;
        uint64_t                            toSessionId;
        uint64_t                            fromSessionId;
    };

    using                                   IdentifyingFn = std::function<void( StrArg email )>;
    using                                   IdentifyFn = std::function<void( Result result, StrArg email, StrArg pendingUrl )>;
    using                                   ConnectingFn = std::function<void( StrArg addr )>;
    using                                   ConnectFn = std::function<void( Result result, StrArg addr, std::string reason )>;
    using                                   AuthingFn = std::function<void( StrArg email )>;
    using                                   AuthFn = std::function<void( Result result, StrArg email, uint64_t sessionId )>;
    using                                   DisconnectFn = std::function<void( Result result, StrArg addr, std::string reason )>;
    using                                   ReadyFn = std::function<void( Result result )>;
    using                                   BindFn = std::function<void( const Message & msg, StrArg data )>;

    //! Used as interface for service specific APIs.
    //! * implementation will:
    //!     * Control implicit auth state:
    //!         * auth request establishes sessionId once successful
    //!         * network disconnect will automatically attempt reconnect using existing sessionId
    //!     * Handles implicit peer notifications (add or remove sessionId endpoint)
    struct IClient
    {
        //! event handlers
        virtual void                        onIdentifying( IdentifyingFn ) = 0;
        virtual void                        onIdentify( IdentifyFn ) = 0;
        virtual void                        onConnecting( ConnectingFn ) = 0;
        virtual void                        onConnect( ConnectFn ) = 0;
        virtual void                        onAuthing( AuthingFn ) = 0;
        virtual void                        onAuth( AuthFn ) = 0;
        virtual void                        onReady( ReadyFn ) = 0;
        virtual void                        onDisconnect( DisconnectFn ) = 0;

        //! async result handler which returns if/when grimauth id is ready
        virtual void                        identify( int timeoutSeconds, IdentifyFn ) = 0;
        virtual bool                        identify(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut email,
                                                StrOut pendingUrl ) = 0;
        //! async result handler which returns if/when connected
        virtual void                        connect( int timeoutSeconds, ConnectFn ) = 0;
        virtual bool                        connect(
                                                int timeoutSeconds,
                                                Result * result, 
                                                StrOut address,
                                                StrOut reason ) = 0;
        //! async result handler which returns if/when session auth is ready
        virtual void                        auth( int timeoutSeconds, AuthFn ) = 0;
        virtual bool                        auth(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut email,
                                                uint64_t * sessionId ) = 0;
        //! async result handler which returns if/when ready to send messages
        virtual void                        ready( int timeoutSeconds, ReadyFn ) = 0;
        virtual bool                        ready(
                                                int timeoutSeconds,
                                                Result * result ) = 0;

        virtual void                        send(
                                                uint64_t toSessionId,
                                                uint8_t type,
                                                cpp::Memory data,
                                                BindFn bindFunction ) = 0;
        virtual void                        send(
                                                uint64_t toSessionId,
                                                uint16_t moniker,
                                                uint16_t bind,
                                                uint8_t type,
                                                uint8_t result,
                                                cpp::Memory data ) = 0;
    };


    using                                   OnHello = std::function<void( Result result, StrArg email, uint64_t sessionId )>;
    using                                   OnRello = std::function<void( Result result )>;
    struct INetServerApi
    {
        virtual void                        hello( auth::AuthToken authToken, OnHello ) = 0;
        virtual void                        rello( uint64_t sessionId, OnRello ) = 0;
    };


    //! Interface used by Proxy & Session servers
    struct INetServer
    {
        //! event handlers
        virtual void                        onIdentifying( IdentifyingFn ) = 0;
        virtual void                        onIdentify( IdentifyFn ) = 0;
        virtual void                        onAuthing( AuthingFn ) = 0;
        virtual void                        onAuth( AuthFn ) = 0;
        virtual void                        onReady( ReadyFn ) = 0;

        //! async result handler which returns if/when grimauth id is ready
        virtual void                        identify( int timeoutSeconds, IdentifyFn ) = 0;
        virtual bool                        identify(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut email,
                                                StrOut pendingUrl ) = 0;
        //! async result handler which returns if/when session auth is ready
        virtual void                        auth( int timeoutSeconds, AuthFn ) = 0;
        virtual bool                        auth(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut email,
                                                uint64_t * sessionId ) = 0;
        //! async result handler which returns if/when ready to send messages
        virtual void                        ready( int timeoutSeconds, ReadyFn ) = 0;
        virtual bool                        ready(
                                                int timeoutSeconds,
                                                Result * result ) = 0;

        virtual void                        onConnect( StrArg ip ) = 0;
        virtual void                        onDisconnect( StrArg ip ) = 0;
        virtual void                        onRecv( StrArg ip, const Message & message, const cpp::Memory & data ) = 0;
        virtual void                        onHello( StrArg ip, const Message & message, uint64_t authToken ) = 0;
        virtual void                        onRello( StrArg ip, const Message & message, uint64_t sessionId ) = 0;
    };


    struct IProxyApi
    {
        using                               AuthServerReply = std::function<void( Result result, StrArg email, uint64_t sessionId )>;
        virtual void                        authServer( StrArg svcName, int nodeId, AuthServerReply reply ) = 0;

        using                               FindServerReply = std::function<void( Result result, uint64_t sessionId )>;
        virtual void                        findServer( StrArg svcName, int nodeId, FindServerReply reply ) = 0;

        virtual void                        openUdp( uint16_t port ) = 0;
        virtual void                        closeUdp( ) = 0;
        virtual void                        sendUdp(
                                                uint64_t toSessionId,
                                                cpp::Memory data ) = 0;
    };


    struct IProxyServer : public INetServer
    {
        //! (1) wait for login, (2) connect to session, (3) session hello, (4) listen, (5) ready
        virtual void                        open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress4,
                                                StrArg listenAddress6,
                                                StrArg sessionAddress,
                                                StrArg email,
                                                uint8_t nodeId ) = 0;
        virtual void                        close( ) = 0;

        virtual void                        onAuthServer( StrArg ip, const Message & message, uint64_t sessionId, StrArg svcName, int nodeId ) = 0;
        virtual void                        onLookupServer( StrArg ip, const Message & message, StrArg svcName, int nodeId ) = 0;
        virtual void                        onLookupSession( StrArg ip, const Message & message, uint64_t sessionId ) = 0;
    };


    struct ISessionApi
    {
        using                           OnSessionResult = std::function<void( uint64_t sessionId, net::Result result )>;
        virtual void                    auth( std::string extAddr, uint64_t authToken, OnSessionResult ) = 0;
        virtual void                    reauth( std::string extAddr, uint64_t sessionId, OnSessionResult ) = 0;
        virtual void                    authServerNode( uint64_t sessionId, std::string svcName, int nodeId, OnSessionResult ) = 0;
        virtual void                    lookupServerNode( std::string svcName, int nodeId, OnSessionResult ) = 0;

        using                           OnLookupSession = std::function<void( uint64_t userId, std::string email, net::Result result )>;
        virtual void                    lookupSession( uint64_t sessionId, OnLookupSession ) = 0;
    };


    struct ISessionServer : public INetServer
    {
        //! (1) wait for login, (2) init auth, (3) listen, (4) ready
        virtual void                        open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress4,
                                                StrArg listenAddress6,
                                                StrArg email ) = 0;
        virtual void                        close( ) = 0;

        virtual void                        onAuth( StrArg ip, const Message & message, StrArg extIp, uint64_t authToken ) = 0;
        virtual void                        onReauth( StrArg ip, const Message & message, StrArg extIp, uint64_t sessionId ) = 0;
        virtual void                        onAuthServer( StrArg ip, const Message & message, uint64_t sessionId, StrArg svcName, int nodeId ) = 0;
        virtual void                        onLookupServer( StrArg ip, const Message & message, StrArg svcName, int nodeId ) = 0;
        virtual void                        onLookupSession( StrArg ip, const Message & message, uint64_t sessionId ) = 0;
    };
}

namespace grim::net
{
    static_assert( sizeof( Message ) == sizeof( uint64_t ) * 3 );
}
