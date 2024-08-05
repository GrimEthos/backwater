module;

#include <cinttypes>
#include <functional>
#include <map>
#include <set>
#include <system_error>

export module grim.net.proxy_server;

import cpp.asio.ip;
import cpp.asio.tcp;
import cpp.log;
import grim.arch.net;
import grim.auth;
import grim.net.session_server;


export namespace grim::net
{
    class ProxyServer
        : public IProxyServer
    {
    public:
                                            ProxyServer( );

        void                                open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress4,
                                                StrArg listenAddress6,
                                                StrArg sessionAddress,
                                                StrArg email,
                                                uint8_t nodeId ) override;
        void                                close( ) override;

        void                                onAuthing( AuthingFn ) override;
        void                                onAuth( AuthFn ) override;
        void                                onReady( ReadyFn ) override;

        void                                auth(
                                                int timeoutSeconds,
                                                AuthFn ) override;
        bool                                auth(
                                                int timeoutSeconds,
                                                StrOut email,
                                                uint64_t * sessionId,
                                                Result * result ) override;

        void                                ready(
                                                int timeoutSeconds,
                                                ReadyFn ) override;
        bool                                ready(
                                                int timeoutSeconds,
                                                Result * result ) override;

        class                               Data;

    private:
        void                                notifyAuthing( );
        void                                notifyAuth( );
        void                                notifyReady( );
        // initialization
        void                                doAuthLogin( );
        void                                authLogin( grim::auth::Result result, grim::auth::AuthToken authToken );
        void                                doSessionHello( );
        void                                sessionHello( grim::auth::Result result, grim::auth::AuthToken authToken );
        void                                doAuthReady( );
        void                                authReady( grim::auth::Result result );
        void                                doListen( );
        // tcp handlers
        void                                connect( std::error_code acceptError, const std::string & addr );
        void                                receive( const std::string & addr, std::string & recvBuffer );
        void                                disconnect( const std::string & addr, std::error_code reason );
        // request handlers
        void                                onConnect( StrArg ip ) override;
        void                                onDisconnect( StrArg ip ) override;
        void                                onHello( StrArg ip, uint64_t authToken, int nodeId ) override;
        void                                onRello( StrArg ip, uint64_t sessionId, int nodeId ) override;
        void                                onAuth( StrArg ip, StrArg extIp, uint64_t authToken );
        void                                onReauth( StrArg ip, StrArg extIp, uint64_t sessionId );
        void                                onAuthServer( StrArg ip, uint64_t sessionId, StrArg svcName, int nodeId );
        void                                onLookupSession( StrArg ip, uint64_t sessionId );
        void                                onLookupServer( StrArg ip, StrArg svcName, int nodeId );

    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };


    class ProxyServer::Data
    {
    public:
        Result                              connected( std::string clientAddr );
        Result                              disconnected( std::string clientAddr );
        Result                              hello(
                                                std::string clientAddr,
                                                uint64_t authToken,
                                                int nodeId,
                                                uint64_t * sessionId );
        Result                              rello(
                                                std::string clientAddr,
                                                uint64_t sessionId );

        Result                              auth(
                                                std::string clientAddr,
                                                uint64_t authToken,
                                                std::string extAddr,
                                                uint64_t * sessionId );
        Result                              reauth(
                                                std::string clientAddr,
                                                uint64_t sessionId,
                                                std::string extAddr,
                                                std::string * oldExtAddr );
        Result                              authServerNode(
                                                std::string clientAddr,
                                                uint64_t sessionId,
                                                std::string svcName, int nodeId );

        Result                              openUdp(
                                                uint64_t sessionId,
                                                std::string intAddr,
                                                std::string udpAddr );
        Result                              closeUdp( uint64_t sessionId );
        Result                              lookupUdp(
                                                uint64_t sessionId,
                                                std::string * extAddr,
                                                std::string * intAddr,
                                                std::string * udpAddr,
                                                uint64_t * key );
    public:
        struct ServerNode
        {
            std::string                     service;
            int                             nodeId;
        };
        struct SessionInfo
        {
            std::string                     extAddr;
            std::string                     email;
            uint64_t                        userId;
            uint64_t                        proxySessionId;
        };
        struct SessionUdpInfo
        {
            std::string                     intAddr;
            std::string                     udpAddr;
            uint64_t                        key;
        };
        using                               SessionSet = std::set<uint64_t>;
        using                               SessionUdpMap = std::map<uint64_t, SessionUdpInfo>;
    private:
        Result                              verifyConnection(
                                                const std::string & clientAddr,
                                                uint64_t * sessionId );
        Result                              verifyServer(
                                                uint64_t sessionId,
                                                std::string_view * service,
                                                int * nodeId );
        Result                              verifySession(
                                                uint64_t sessionId,
                                                std::string_view * extAddr,
                                                std::string_view * email,
                                                uint64_t * userId,
                                                uint64_t * proxySessionId );
        Result                              verifyServerNetwork(
                                                std::string serviceName,
                                                std::string addr );
        Result                              grimauth(
                                                const std::string & extAddr,
                                                uint64_t authToken,
                                                const std::string & svc,
                                                uint64_t * userId,
                                                std::string * email );
        uint64_t                            makeSessionId( );
    private:
        std::map<std::string, uint64_t>     clientSessions;
        std::map<uint64_t, SessionSet>      proxySessions;
        std::map<ServerNode, uint64_t>      serviceSessionMap;
        std::map<uint64_t, ServerNode>      sessionServiceMap;
        std::map<uint64_t, SessionInfo>     sessions;
    };
    namespace test
    {
        void                                testProxyServerData( );
    };
}

namespace grim::net
{
    struct ProxyServer::Detail
    {
        std::string                         email;
        std::string                         bindAddress4;
        std::string                         bindAddress6;
        cpp::AsyncContext                   io;
        cpp::TcpServer                      tcp;
        grim::net::SessionServer::Client    sessionClient;
        grim::auth::Client                  grimauth;
        grim::auth::AuthToken               authToken;

        AuthingFn                           onAuthingHandler;
        AuthFn                              onAuthHandler;
        ReadyFn                             onReadyHandler;
        ConnectFn                           onConnectHandler;
        DisconnectFn                        onDisconnectHandler;

        AuthFn                              authHandler;
        ReadyFn                             readyHandler;
        cpp::AsyncTimer                     handlerTimer;

        uint64_t                            isAuthed : 1;
        uint64_t                            isReady : 1;

        Data                                data;
    };

    ProxyServer::ProxyServer( ) :
        detail( std::make_unique<Detail>( ) )
    {
    }

    void ProxyServer::open(
            cpp::AsyncContext & io,
            StrArg listenAddress4,
            StrArg listenAddress6,
            StrArg sessionAddress,
            StrArg email,
            uint8_t nodeId )
    {
        detail->isAuthed = false;
        detail->isReady = false;

        detail->io = io;
        detail->bindAddress4 = listenAddress4;
        detail->bindAddress6 = listenAddress6;
        detail->email = email;
        detail->grimauth.setAsyncContext( io );
        doAuthLogin( );
    }

    void ProxyServer::close( )
    {
        detail->tcp.close( );
    }

    void ProxyServer::onAuthing( AuthingFn fn )
    {
        detail->onAuthingHandler = fn;
    }

    void ProxyServer::onAuth( AuthFn fn )
    {
        detail->onAuthHandler = fn;
    }

    void ProxyServer::onReady( ReadyFn fn )
    {
        detail->onReadyHandler = fn;
    }

    void ProxyServer::auth( int timeoutSeconds, AuthFn fn )
    {
        // if isReady, post result immediately
        if ( detail->isAuthed )
        { detail->io.post( [this, fn]( ) { fn( detail->email, 0, Result::Ok ); } ); return; }

        // start a timer that will return timeout if it elapses before the readyHandler is called
        detail->authHandler = fn;
        detail->handlerTimer = detail->io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this]( )
            { detail->authHandler( detail->email, 0, Result::Timeout ); } );
    }

    bool ProxyServer::auth(
        int timeoutSeconds,
        StrOut email,
        uint64_t * sessionId,
        Result * result )
    {
        *result = Result::Ok;
        if ( !detail->isAuthed )
        {
            auto isAuthed = detail->io.createCondition<Result>( );
            ready( timeoutSeconds, [&]( Result result )
                { isAuthed.notify( result ); } );
            *result = isAuthed.wait( );
        }
        *email = detail->email;
        *sessionId = 0;

        return *result == Result::Ok;
    }

    void ProxyServer::ready( int timeoutSeconds, ReadyFn fn )
    {
        // if isReady, post result immediately
        if ( detail->isReady )
        { detail->io.post( [fn]( ) { fn( Result::Ok ); } ); return; }

        // start a timer that will return timeout if it elapses before the readyHandler is called
        detail->readyHandler = fn;
        detail->handlerTimer = detail->io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this]( )
            { detail->readyHandler( Result::Timeout ); } );
    }

    bool ProxyServer::ready( int timeoutSeconds, Result * result )
    {
        *result = Result::Ok;
        if ( !detail->isReady )
        {
            auto isReady = detail->io.createCondition<Result>( );
            ready( timeoutSeconds, [&]( Result result )
                { isReady.notify( result ); } );
            *result = isReady.wait( );
        }
        return *result == Result::Ok;
    }

}