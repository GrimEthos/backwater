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
        Access = 0xfa,                      // no connection to proxy
        Route = 0xfb,                       // to session can't be reached
        Retry = 0xfc,                       // rate limiting
        More = 0xfd,                        // multi-part message, bind will invoke same callback multiple times
        Timeout = 0xfe,                     // timeout expired
        Unknown = 0xff
    };

    struct Message
    {
        uint64_t                            toSessionId;
        uint64_t                            fromSessionId;
        uint64_t                            len : 16; // data.length() / 8
        uint64_t                            bind : 16;
        uint64_t                            result : 8;
        uint64_t                            unused : 24;
    };

    using                                   BindFn = std::function<void( const Message & msg, StrArg data )>;
    using                                   ConnectingFn = std::function<void( StrArg addr )>;
    using                                   ConnectFn = std::function<void( StrArg addr, Result result, std::string reason )>;
    using                                   AuthingFn = std::function<void( StrArg email, StrArg access )>;
    using                                   AuthFn = std::function<void( StrArg email, uint64_t sessionId, Result result )>;
    using                                   ReadyFn = std::function<void( Result result )>;
    using                                   DisconnectFn = std::function<void( StrArg addr, Result result, std::string reason )>;


    //! Used as interface for service specific APIs.
    //! * implementation will:
    //!     * Control implicit auth state:
    //!         * auth request establishes sessionId once successful
    //!         * network disconnect will automatically attempt reconnect using existing sessionId
    //!     * Handles implicit peer notifications (add or remove sessionId endpoint)
    struct IClient
    {
        virtual void                        open(
                                                cpp::AsyncContext & io,
                                                StrArg addrs,
                                                StrArg email,
                                                StrArg access ) = 0;
        virtual void                        close( ) = 0;

        //! event handlers
        virtual void                        onConnecting( ConnectingFn ) = 0;
        virtual void                        onConnect( ConnectFn ) = 0;
        virtual void                        onAuthing( AuthingFn ) = 0;
        virtual void                        onAuth( AuthFn ) = 0;
        virtual void                        onReady( ReadyFn ) = 0;
        virtual void                        onDisconnect( DisconnectFn ) = 0;

        //! connect is a result handler for `open` which returns if/when
        //! a connection has been established
        virtual void                        connect(
                                                int timeoutSeconds,
                                                ConnectFn ) = 0;
        virtual bool                        connect(
                                                int timeoutSeconds,
                                                StrOut address,
                                                Result * result, StrOut reason ) = 0;
        //! auth is a result handler for `open` which returns if/when
        //! an authorization result is available
        virtual void                        auth(
                                                int timeoutSeconds,
                                                AuthFn ) = 0;
        virtual bool                        auth(
                                                int timeoutSeconds,
                                                StrOut email,
                                                uint64_t * sessionId,
                                                Result * result ) = 0;
        //! ready is a result handler for `open` which returns if/when
        //! the client is connected & auth'd and ready to send messages
        virtual void                        ready(
                                                int timeoutSeconds,
                                                ReadyFn ) = 0;
        virtual bool                        ready(
                                                int timeoutSeconds,
                                                Result * result ) = 0;

        virtual void                        send(
                                                uint64_t toSessionId,
                                                uint16_t type,
                                                cpp::Memory data,
                                                BindFn bindFunction ) = 0;
        virtual void                        send(
                                                uint64_t toSessionId,
                                                uint16_t bind,
                                                uint16_t type,
                                                uint16_t result,
                                                cpp::Memory data ) = 0;

        virtual void                        openUdp( uint16_t port ) = 0;
        virtual void                        closeUdp( ) = 0;
        virtual void                        sendUdp(
                                                uint64_t toSessionId,
                                                cpp::Memory data ) = 0;
    };

    struct IProxyApi
    {
        using                               HelloReply = std::function<void( Result result, StrArg email, uint64_t sessionId )>;
        virtual void                        hello( auth::AuthToken authToken, HelloReply reply ) = 0;
        
        using                               RelloReply = std::function<void( Result result )>;
        virtual void                        rello( uint64_t sessionId, RelloReply reply ) = 0;
        
        using                               AuthServerReply = std::function<void( Result result, StrArg email, uint64_t sessionId )>;
        virtual void                        authServer( StrArg svcName, int nodeId, AuthServerReply reply ) = 0;
        
        using                               FindServerReply = std::function<void( Result result, uint64_t sessionId )>;
        virtual void                        findServer( StrArg svcName, int nodeId, FindServerReply reply ) = 0;
    };

    struct IProxyServer
    {
        virtual void                        open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress,
                                                StrArg sessionAddress,
                                                StrArg email,
                                                uint8_t nodeId ) = 0;
        virtual void                        close( ) = 0;

        virtual void                        onConnect( ConnectFn ) = 0;
        virtual void                        onDisconnect( ConnectFn ) = 0;
    };

    struct ISessionServer
    {
        virtual void                        open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress4,
                                                StrArg listenAddress6,
                                                StrArg email ) = 0;
        virtual void                        close( ) = 0;

        //! event handlers
        virtual void                        onAuthing( AuthingFn ) = 0;
        virtual void                        onAuth( AuthFn ) = 0;
        virtual void                        onReady( ReadyFn ) = 0;

        //! auth is a result handler for `open` which returns if/when
        //! an authorization result is available
        virtual void                        auth(
                                                int timeoutSeconds,
                                                AuthFn ) = 0;
        virtual bool                        auth(
                                                int timeoutSeconds,
                                                StrOut email,
                                                uint64_t * sessionId,
                                                Result * result ) = 0;
        //! ready is a result handler for `open` which returns if/when
        //! the server is auth'd and listening for connections
        virtual void                        ready(
                                                int timeoutSeconds,
                                                ReadyFn ) = 0;
        virtual bool                        ready(
                                                int timeoutSeconds,
                                                Result * result ) = 0;

        virtual void                        onConnect( StrArg ip ) = 0;
        virtual void                        onDisconnect( StrArg ip ) = 0;

        virtual void                        onHello( StrArg ip, uint64_t authToken, int nodeId ) = 0;
        virtual void                        onRello( StrArg ip, uint64_t sessionId, int nodeId ) = 0;

        // proxy client requests
        virtual void                        onAuth( StrArg ip, StrArg extIp, uint64_t authToken ) = 0;
        virtual void                        onReauth( StrArg ip, StrArg extIp, uint64_t sessionId ) = 0;
        virtual void                        onAuthServer( StrArg ip, uint64_t sessionId, StrArg svcName, int nodeId ) = 0;

        // proxy requests
        virtual void                        onLookupSession( StrArg ip, uint64_t sessionId ) = 0;
        virtual void                        onLookupServer( StrArg ip, StrArg svcName, int nodeId ) = 0;

        struct ClientApi
        {
            using                           onHello = std::function<void(
                                                uint64_t sessionId,
                                                net::Result result )>;
            virtual void                    hello( 
                                                uint64_t authToken, 
                                                uint8_t nodeId ) = 0;

            using                           onLookupSession = std::function<void(
                                                uint64_t sessionId,
                                                net::Result result )>;
            virtual void                    lookupSession( 
                                                uint64_t sessionId, 
                                                onLookupSession ) = 0;

            using                           onLookupServerNode = std::function<void(
                                                uint64_t sessionId,
                                                net::Result result )>;
            virtual void                    lookupServerNode( 
                                                std::string svcName, 
                                                int nodeId, 
                                                onLookupServerNode ) = 0;
        };
    };
}

namespace grim::net
{
    static_assert( sizeof( Message ) == sizeof( uint64_t ) * 3 );
}
