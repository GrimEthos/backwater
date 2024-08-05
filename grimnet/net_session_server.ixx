module;

#include <cinttypes>
#include <set>
#include <map>
#include <functional>
#include <system_error>
#include <memory>

export module grim.net.session_server;

import cpp.random;
import cpp.log;
import cpp.asio.ip;
import cpp.asio.tcp;
import grim.arch.net;
import grim.auth;

export namespace grim::net
{
    class SessionServer 
        : public ISessionServer
    {
    public:
                                            SessionServer( );

        void                                open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress4,
                                                StrArg listenAddress6,
                                                StrArg email ) override;
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
        class                               Client;

    private:
        void                                notifyAuthing( );
        void                                notifyAuth( );
        void                                notifyReady( );
        // initialization
        void                                doAuthLogin( );
        void                                authLogin( grim::auth::Result result, grim::auth::AuthToken authToken );
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
        void                                onAuth( StrArg ip, StrArg extIp, uint64_t authToken ) override;
        void                                onReauth( StrArg ip, StrArg extIp, uint64_t sessionId ) override;
        void                                onAuthServer( StrArg ip, uint64_t sessionId, StrArg svcName, int nodeId ) override;
        void                                onLookupSession( StrArg ip, uint64_t sessionId ) override;
        void                                onLookupServer( StrArg ip, StrArg svcName, int nodeId ) override;

    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SessionServer::Client
        : public ISessionServer::ClientApi
    {
    public:
        Client( );

        void                                open( cpp::AsyncContext & io, std::string addr, std::string authToken );
        void                                close( );

        void                                ready( int timeoutSeconds, net::ReadyFn );

    private:
        void                                hello( uint64_t authToken, uint8_t nodeId ) override;
        void                                rello( uint64_t sessionId ) override;
        void                                auth( uint64_t authToken, std::string extAddr, onAuth );
        void                                reauth( uint64_t sessionId, std::string extAddr, onAuth );
        void                                authServerNode( uint64_t sessionId, std::string svcName, int nodeId, onAuth );
        void                                lookupSession( uint64_t sessionId, onLookupSession ) override;
        void                                lookupServerNode( std::string svcName, int nodeId, onLookupServerNode ) override;


    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SessionServer::Data
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
        cpp::Random                         rng;
        std::map<std::string, uint64_t>     clientSessions;
        std::map<uint64_t, SessionSet>      proxySessions;
        std::map<ServerNode, uint64_t>      serviceSessionMap;
        std::map<uint64_t, ServerNode>      sessionServiceMap;
        std::map<uint64_t, SessionInfo>     sessions;
        SessionUdpMap                       sessionUdp;
    };
    namespace test
    {
        void                                testSessionServerData( );
    };
}

namespace grim::net
{
    const char * AuthServiceId = "grimethos.com";
    const char * ProxyServiceId = "proxy.grimethos.com";
    std::map<std::string, std::vector<std::string>> ServiceNetwork =
    {
        { ProxyServiceId, { } },
    };

    bool operator<( const SessionServer::Data::ServerNode & x, const SessionServer::Data::ServerNode & y ) {
        return std::tie( x.service, x.nodeId ) < std::tie( y.service, y.nodeId );
    }

    struct SessionServer::Client::Detail
    {
        std::unique_ptr<net::IClient>       client;
    };

    struct SessionServer::Detail
    {
        std::string                         email;
        std::string                         bindAddress4;
        std::string                         bindAddress6;
        cpp::AsyncContext                   io;
        cpp::TcpServer                      tcp;
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

    SessionServer::SessionServer( ) :
        detail( std::make_unique<Detail>() )
    {
    }

    void SessionServer::open(
            cpp::AsyncContext & io,
            StrArg listenAddress4,
            StrArg listenAddress6,
            StrArg email )
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

    void SessionServer::close( )
    {
        detail->tcp.close( );
    }

    void SessionServer::onAuthing( AuthingFn fn )
    {
        detail->onAuthingHandler = fn;
    }

    void SessionServer::onAuth( AuthFn fn )
    {
        detail->onAuthHandler = fn;
    }

    void SessionServer::onReady( ReadyFn fn )
    {
        detail->onReadyHandler = fn;
    }

    void SessionServer::auth( int timeoutSeconds, AuthFn fn )
    {
        // if isReady, post result immediately
        if ( detail->isAuthed )
            { detail->io.post( [this, fn]( ) { fn( detail->email, 0, Result::Ok ); } ); return; }

        // start a timer that will return timeout if it elapses before the readyHandler is called
        detail->authHandler = fn;
        detail->handlerTimer = detail->io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this]( )
            { detail->authHandler( detail->email, 0, Result::Timeout ); } );
    }

    bool SessionServer::auth(
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

    void SessionServer::ready( int timeoutSeconds, ReadyFn fn )
    {
        // if isReady, post result immediately
        if ( detail->isReady )
            { detail->io.post( [fn]( ) { fn( Result::Ok ); } ); return; }

        // start a timer that will return timeout if it elapses before the readyHandler is called
        detail->readyHandler = fn;
        detail->handlerTimer = detail->io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this]( ) 
            { detail->readyHandler( Result::Timeout ); } );
    }

    bool SessionServer::ready( int timeoutSeconds, Result * result )
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

    void SessionServer::onConnect( StrArg ip )
    {

    }

    void SessionServer::onDisconnect( StrArg ip )
    {

    }

    void SessionServer::onHello( StrArg ip, uint64_t authToken, int nodeId )
    {

    }
    void SessionServer::onRello( StrArg ip, uint64_t sessionId, int nodeId )
    {

    }

    void SessionServer::onAuth( StrArg ip, StrArg extIp, uint64_t authToken )
    {

    }
    void SessionServer::onReauth( StrArg ip, StrArg extIp, uint64_t sessionId )
    {

    }
    void SessionServer::onAuthServer( StrArg ip, uint64_t sessionId, StrArg svcName, int nodeId )
    {

    }

    void SessionServer::onLookupSession( StrArg ip, uint64_t sessionId )
    {

    }
    void SessionServer::onLookupServer( StrArg ip, StrArg svcName, int nodeId )
    {

    }

    void SessionServer::notifyAuthing( )
    {
        if ( detail->onAuthingHandler ) 
            { detail->onAuthingHandler( detail->email, "" ); }
    }

    void SessionServer::notifyAuth( )
    {
        detail->isAuthed = true;
        detail->handlerTimer.cancel( );
        if ( detail->onAuthHandler )
            { detail->onAuthHandler( detail->email, 0, Result::Ok ); }
        if ( detail->authHandler )
            { detail->authHandler( detail->email, 0, Result::Ok ); }
    }

    void SessionServer::notifyReady( )
    {
        detail->isReady = true;
        detail->handlerTimer.cancel( );
        if ( detail->onReadyHandler )
            { detail->onReadyHandler( Result::Ok ); }
        if ( detail->readyHandler )
            { detail->readyHandler( Result::Ok ); }
    }

    void SessionServer::doAuthLogin( )
    {
        using namespace std::placeholders;
        int options = grim::auth::LoginOption::Default;
        detail->grimauth.login(
            grim::auth::UserEmail{ detail->email },
            grim::auth::ServiceId{ "backwater.grimethos.com" },
            options,
            5,
            std::bind(&SessionServer::authLogin, this, _1, _2));
        notifyAuthing( );
    }

    void SessionServer::authLogin( grim::auth::Result result, grim::auth::AuthToken authToken )
    {
        using grim::auth::Result;

        switch ( result )
        {
        case Result::Ok:
            detail->authToken = authToken;
            doAuthReady( );
            break;
        case Result::Pending:
            // this shouldn't happen
            break;
        case Result::Denied:
            cpp::Log::notice( "Authentication denied by {}", detail->email );
            break;
        case Result::Timeout:
            // todo: is this correct?
            doAuthLogin( );
            break;
        case Result::Retry:
            detail->io.waitFor( cpp::Duration::ofSeconds( 60 ), [this]( ) { doAuthLogin( ); } );
            break;
        }
    }

    void SessionServer::doAuthReady( )
    {
        using namespace std::placeholders;

        notifyAuth( );

        int options = grim::auth::LoginOption::Default;
        detail->grimauth.authInit(
            grim::auth::AuthToken{ detail->authToken },
            std::bind( &SessionServer::authReady, this, _1 ) );
    }

    void SessionServer::authReady( grim::auth::Result result )
    {
        if ( result != grim::auth::Result::Ok )
        {
            cpp::Log::error( "authReady() : result={}", std::to_underlying(result) );
            detail->io.waitFor( cpp::Duration::ofSeconds( 60 ), [this]( ) { doAuthLogin( ); } );
            return;
        }
        doListen( );
    }

    void SessionServer::doListen( )
    {
        using namespace std::placeholders;
        cpp::TcpServer::TlsInfo tlsInfo;
        detail->tcp.open(
            detail->io,
            detail->bindAddress4,
            detail->bindAddress6,
            std::bind( &SessionServer::connect, this, _1, _2 ),
            std::bind( &SessionServer::receive, this, _1, _2 ),
            std::bind( &SessionServer::disconnect, this, _1, _2 ),
            nullptr );
        notifyReady( );
    }

    void SessionServer::connect( std::error_code acceptError, const std::string & addr )
    {
        if ( acceptError )
        {
            cpp::Log::error( "connect() : addr='{}' msg='{}'", addr, acceptError.message( ) );
            return;
        }
        cpp::Log::info( "connect() : addr='{}'", addr );
        detail->data.connected( addr );
    }

    void SessionServer::receive( const std::string & addr, std::string & recvBuffer )
    {
    }

    void SessionServer::disconnect( const std::string & addr, std::error_code reason )
    {
        cpp::Log::info( "disconnect() : addr='{}' msg='{}'", addr, reason.message( ) );

    }

    SessionServer::Client::Client( )
    {

    }

    void SessionServer::Client::open( cpp::AsyncContext & io, std::string addr, std::string authToken )
    {

    }

    void SessionServer::Client::close( )
    {

    }

    void SessionServer::Client::ready( int timeoutSeconds, net::ReadyFn )
    {

    }

    void SessionServer::Client::auth( uint64_t authToken, std::string extAddr, onAuth )
    {

    }

    void SessionServer::Client::reauth( uint64_t sessionId, std::string extAddr, onAuth )
    {

    }

    void SessionServer::Client::authServerNode( uint64_t sessionId, std::string svcName, int nodeId, onAuth )
    {

    }

    void SessionServer::Client::lookupSession( uint64_t sessionId, onLookupSession )
    {

    }

    void SessionServer::Client::lookupServerNode( std::string svcName, int nodeId, onLookupServerNode )
    {

    }


    Result SessionServer::Data::verifyConnection( const std::string & clientAddr, uint64_t * sessionId )
    {
        // verify clientAddr
        auto itr = clientSessions.find( clientAddr );
        if ( itr == clientSessions.end( ) )
            { return Result::Arg; }
        *sessionId = itr->second;
        return Result::Ok;
    }

    Result SessionServer::Data::verifyServer( uint64_t sessionId, std::string_view * service, int * nodeId )
    {
        auto itr = sessionServiceMap.find( sessionId );
        if ( itr == sessionServiceMap.end( ) )
            { return Result::Arg; }
        auto & serverNode = itr->second;
        *service = serverNode.service;
        *nodeId = serverNode.nodeId;
        return Result::Ok;
    }

    Result SessionServer::Data::verifyServerNetwork( std::string serviceName, std::string serverAddr )
    {
        auto addr = cpp::Inet4::toTcpEndpoint( serverAddr );
        auto serverNetwork = cpp::Inet4::toAddress( addr.address( ).to_string( ) );

        auto itr = ServiceNetwork.find( serviceName );
        if ( itr == ServiceNetwork.end( ) )
            { return Result::Ok; }

        auto & validNetworks = itr->second;
        bool isSafeNetwork = validNetworks.empty( );
        for ( auto validNetwork : validNetworks )
        {
            auto network = cpp::Inet4::toAddress( validNetwork );
            if ( serverNetwork.is_subnet_of( network ) )
            { isSafeNetwork = true; break; }
        }
        if ( !isSafeNetwork )
            { return Result::Access; }
        return Result::Ok;
    }

    Result SessionServer::Data::grimauth(
        const std::string & extAddr,
        uint64_t authToken,
        const std::string & svc,
        uint64_t * userId,
        std::string * email )
    {
        if ( authToken == 1 )
        {
            *userId = 1;
            *email = "monkeysmarts@gmail.com";
        }
        else if ( authToken == 2 )
        {
            *userId = 2;
            *email = "thomas.farthing@grimethos.com";
        }
        else
        {
            return Result::Access;
        }
        return Result::Ok;
    }

    uint64_t SessionServer::Data::makeSessionId( )
    {
        uint64_t sessionId = 0;
        while ( !sessionId || sessions.count( sessionId ) )
        { sessionId = rng.rand( ); }
        return sessionId;
    }

    Result SessionServer::Data::verifySession(
        uint64_t sessionId,
        std::string_view * extAddr,
        std::string_view * email,
        uint64_t * userId,
        uint64_t * proxySessionId )
    {
        auto itr = sessions.find( sessionId );
        if ( itr == sessions.end( ) )
        { return Result::Arg; }
        auto & sessionInfo = itr->second;
        *extAddr = sessionInfo.extAddr;
        *email = sessionInfo.email;
        *userId = sessionInfo.userId;
        *proxySessionId = sessionInfo.proxySessionId;
        return Result::Ok;
    }

    Result SessionServer::Data::connected( std::string clientAddr )
    {
        auto itr = clientSessions.find( clientAddr );
        if ( itr != clientSessions.end( ) )
        { return Result::Arg; }
        clientSessions[clientAddr] = 0;
        return Result::Ok;
    }

    Result SessionServer::Data::disconnected( std::string clientAddr )
    {
        if ( !clientSessions.erase( clientAddr ) )
            { return Result::Arg; }
        return Result::Ok;
    }

    Result SessionServer::Data::hello(
        std::string clientAddr,
        uint64_t authToken,
        int nodeId,
        uint64_t * sessionId )
    {
        uint64_t clientSessionId = 0;
        Result result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != Result::Ok )
            { return result; }

        result = verifyServerNetwork( ProxyServiceId, clientAddr );
        if ( result != Result::Ok )
            { return result; }

        uint64_t userId;
        std::string email;
        result = grimauth( clientAddr, authToken, "grimethos.com", &userId, &email );
        if ( result != Result::Ok )
            { return result; }

        uint64_t newSessionId = makeSessionId( );

        clientSessions[clientAddr] = newSessionId;

        SessionInfo & sessionInfo = sessions[newSessionId];
        sessionInfo.extAddr = clientAddr;
        sessionInfo.proxySessionId = clientSessionId;
        sessionInfo.email = email;
        sessionInfo.userId = userId;

        ServerNode serverNode{ ProxyServiceId, nodeId };
        serviceSessionMap[serverNode] = newSessionId;
        sessionServiceMap[newSessionId] = serverNode;

        *sessionId = newSessionId;

        return Result::Ok;
    }

    Result SessionServer::Data::rello(
        std::string clientAddr,
        uint64_t sessionId )
    {
        std::string oldAddr;
        Result result = reauth( clientAddr, sessionId, clientAddr, &oldAddr );
        if ( result == Result::Ok )
        {
            if ( auto itr = clientSessions.find( oldAddr ); itr != clientSessions.end( ) )
                { clientSessions[oldAddr] = 0; }
            clientSessions[clientAddr] = sessionId;
        }

        return result;
    }

    Result SessionServer::Data::auth(
        std::string clientAddr,
        uint64_t authToken,
        std::string extAddr,
        uint64_t * sessionId )
    {
        uint64_t clientSessionId = 0;
        Result result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != Result::Ok )
            { return result; }

        std::string_view clientServiceName;
        int clientNodeId;
        result = verifyServer( clientSessionId, &clientServiceName, &clientNodeId );
        if ( result != Result::Ok )
            { return result; }

        if ( clientServiceName != "proxy.grimethos.com" )
            { return Result::Access; }

        uint64_t userId;
        std::string email;
        result = grimauth( extAddr, authToken, "grimethos.com", &userId, &email );
        if ( result != Result::Ok )
            { return result; }

        uint64_t newSessionId = makeSessionId( );

        SessionInfo & sessionInfo = sessions[newSessionId];
        sessionInfo.extAddr = extAddr;
        sessionInfo.proxySessionId = clientSessionId;
        sessionInfo.email = email;
        sessionInfo.userId = userId;

        proxySessions[clientSessionId].insert( newSessionId );

        *sessionId = newSessionId;
        return Result::Ok;
    }

    Result SessionServer::Data::reauth(
        std::string clientAddr,
        uint64_t sessionId,
        std::string extAddr,
        std::string * oldExtAddr )
    {
        uint64_t clientSessionId = 0;
        Result result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != Result::Ok )
            { return result; }

        bool isProxyServer = clientAddr == extAddr;
        if ( isProxyServer )
            { clientSessionId = sessionId; }

        std::string_view clientServiceName;
        int clientNodeId;
        result = verifyServer( clientSessionId, &clientServiceName, &clientNodeId );
        if ( result != Result::Ok )
            { return result; }

        std::string_view sessionExtAddr;
        std::string_view email;
        uint64_t userId = 0;
        uint64_t proxySessionId = 0;
        result = verifySession( sessionId, &sessionExtAddr, &email, &userId, &proxySessionId );
        if ( result != Result::Ok )
            { return result; }

        auto oldAddr = cpp::Inet4::toTcpEndpoint( sessionExtAddr );
        auto newAddr = cpp::Inet4::toTcpEndpoint( extAddr );
        if ( oldAddr.address( ) != newAddr.address( ) )
            { return Result::Access; }

        SessionInfo & sessionInfo = sessions[sessionId];
        *oldExtAddr = sessionInfo.extAddr;
        uint64_t oldProxySessionId = sessionInfo.proxySessionId;
        sessionInfo.extAddr = extAddr;

        // if this is a client reauth (proxy server)
        if ( isProxyServer )
        {
            proxySessions[oldProxySessionId].erase( sessionId );
            proxySessions[clientSessionId].insert( sessionId );
        }
        else
        {
            sessionInfo.proxySessionId = clientSessionId;
        }

        return Result::Ok;
    }

    Result SessionServer::Data::authServerNode(
        std::string clientAddr,
        uint64_t sessionId,
        std::string svcName, int nodeId )
    {
        uint64_t clientSessionId = 0;
        Result result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != Result::Ok )
        { return result; }

        std::string_view clientServiceName;
        int clientNodeId;
        result = verifyServer( clientSessionId, &clientServiceName, &clientNodeId );
        if ( result != Result::Ok )
        { return result; }

        std::string_view extAddr;
        std::string_view email;
        uint64_t userId = 0;
        uint64_t proxySessionId = 0;
        result = verifySession( sessionId, &extAddr, &email, &userId, &proxySessionId );
        if ( result != Result::Ok )
        { return result; }

        if ( email != "monkeysmarts@gmail.com" )
        { return Result::Access; }

        result = verifyServerNetwork( svcName, std::string{ extAddr } );
        if ( result != Result::Ok )
        { return result; }

        ServerNode serverNode{ svcName, nodeId };
        serviceSessionMap[serverNode] = sessionId;
        sessionServiceMap[sessionId] = serverNode;

        return Result::Ok;
    }

    Result SessionServer::Data::openUdp(
        uint64_t sessionId,
        std::string intAddr,
        std::string udpAddr )
    {
        return Result::Access;
    }

    Result SessionServer::Data::closeUdp( uint64_t sessionId )
    {
        return Result::Access;
    }

    Result SessionServer::Data::lookupUdp(
        uint64_t sessionId,
        std::string * extAddr,
        std::string * intAddr,
        std::string * udpAddr,
        uint64_t * key )
    {
        return Result::Access;
    }


    namespace test
    {
        void testSessionServerData( )
        {
            const char * LOCAL_ADDR1 = "10.10.10.1:1";
            const char * LOCAL_ADDR1_2 = "10.10.10.1:3";
            const char * LOCAL_ADDR2 = "10.10.10.2:2";
            const char * REMOTE_ADDR1 = "10.10.10.100:1";
            const char * REMOTE_ADDR2 = "10.10.10.101:2";

            SessionServer::Data data;
            data.connected( LOCAL_ADDR1 );
            data.connected( LOCAL_ADDR2 );

            uint64_t proxySessionId[2];
            if ( data.hello( LOCAL_ADDR1, 1, 0, &proxySessionId[0] ) != Result::Ok )
                { throw std::exception{ "data.authClient( LOCAL_ADDR1 )" }; }
            if ( data.hello( LOCAL_ADDR2, 2, 1, &proxySessionId[1] ) != Result::Ok )
                { throw std::exception{ "data.authClient( LOCAL_ADDR2 )" }; }

            data.connected( LOCAL_ADDR1_2 );
            if ( data.rello( LOCAL_ADDR1_2, proxySessionId[0] ) != Result::Ok )
                { throw std::exception{ "data.reauthClient( LOCAL_ADDR1_2 )" }; }

            uint64_t galaxySessionId;
            if ( data.auth( LOCAL_ADDR1_2, 1, REMOTE_ADDR1, &galaxySessionId ) != Result::Ok )
                { throw std::exception{ "data.auth( LOCAL_ADDR1_2, 1, REMOTE_ADDR1 )" }; }
            if ( data.authServerNode( LOCAL_ADDR1_2, galaxySessionId, "galaxy.backwater.grimethos.com", 0 ) != Result::Ok )
                { throw std::exception{ "data.authServerNode( LOCAL_ADDR1_2, galaxySessionId )" }; }
        }
    }
}
