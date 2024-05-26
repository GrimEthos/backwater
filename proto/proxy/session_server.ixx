module;

#include <string>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <system_error>

export module grim.proto.session_server;
import cpp.random;
import cpp.bit;
import cpp.asio.ip;
import cpp.asio.tcp;
import grim.proto.net;

export namespace grim
{
    const char * AuthServiceId = "grimethos.com";
    const char * ProxyServiceId = "proxy.grimethos.com";
    std::map<std::string, std::vector<std::string>> ServiceNetwork =
    {
        { ProxyServiceId, { } },
    };


    class SessionServer
    {
    public:
        enum class                          ResultCode { Ok, Arg, Access };

                                            SessionServer( );

        void                                open( cpp::AsyncContext & io, std::string config );
        void                                close( );


        class Data;
        class Client;

    private:
        void                                onConnect( std::error_code acceptError, const std::string & addr );
        void                                onReceive( const std::string & addr, std::string & recvBuffer );
        void                                onDisconnect( const std::string & addr, std::error_code reason );

    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SessionServer::Client
    {
    public:
                                            Client( );

        void                                open( cpp::AsyncContext & io, std::string config );
        void                                close( );

        void                                ready( int timeoutSeconds, net::ReadyFn );

        using                               onAuth = std::function<void(
                                                uint64_t sessionId,
                                                net::Result result )>;
        void                                auth( uint64_t authToken, std::string extAddr, onAuth );
        void                                reauth( uint64_t sessionId, std::string extAddr, onAuth );
        void                                authServerNode( uint64_t sessionId, std::string svcName, int nodeId, onAuth );
        
        using                               onLookupSession = std::function<void(
                                                uint64_t sessionId,
                                                net::Result result )>;
        void                                lookupSession( uint64_t sessionId, onLookupSession );

        using                               onLookupServerNode = std::function<void(
                                                uint64_t sessionId,
                                                net::Result result )>;
        void                                lookupServerNode( std::string svcName, int nodeId, onLookupServerNode );

    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SessionServer::Data
    {
    public:
        ResultCode                              connected( std::string clientAddr );
        ResultCode                              disconnected( std::string clientAddr );
        ResultCode                              authClient(
                                                std::string clientAddr,
                                                uint64_t authToken,
                                                int nodeId,
                                                uint64_t * sessionId );
        ResultCode                              reauthClient(
                                                std::string clientAddr,
                                                uint64_t sessionId );

        ResultCode                              auth(
                                                std::string clientAddr,
                                                uint64_t authToken,
                                                std::string extAddr,
                                                uint64_t * sessionId );
        ResultCode                              reauth(
                                                std::string clientAddr,
                                                uint64_t sessionId,
                                                std::string extAddr,
                                                std::string * oldExtAddr );
        ResultCode                              authServerNode(
                                                std::string clientAddr,
                                                uint64_t sessionId,
                                                std::string svcName, int nodeId );

        ResultCode                              openUdp(
                                                uint64_t sessionId,
                                                std::string intAddr,
                                                std::string udpAddr );
        ResultCode                              closeUdp( uint64_t sessionId );
        ResultCode                              lookupUdp(
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

        ResultCode                              verifyConnection(
                                                const std::string & clientAddr,
                                                uint64_t * sessionId );
        ResultCode                              verifyServer(
                                                uint64_t sessionId,
                                                std::string_view * service,
                                                int * nodeId );
        ResultCode                              verifySession(
                                                uint64_t sessionId,
                                                std::string_view * extAddr,
                                                std::string_view * email,
                                                uint64_t * userId,
                                                uint64_t * proxySessionId );
        ResultCode                              verifyServerNetwork(
                                                std::string serviceName,
                                                std::string addr );
        ResultCode                              grimauth(
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



namespace grim
{
    bool operator<( const SessionServer::Data::ServerNode & x, const SessionServer::Data::ServerNode & y ) {
        return std::tie( x.service, x.nodeId ) < std::tie( y.service, y.nodeId );
    }

    struct SessionServer::Client::Detail
    {
        std::unique_ptr<net::IClient>       client;
    };

    struct SessionServer::Detail
    {
        uint16_t                            port;
        Data                                data;
        cpp::TcpServer                      tcp;
    };

    SessionServer::SessionServer(  )
    {
    }

    void SessionServer::open( cpp::AsyncContext & io, std::string configText )
    {
        using namespace std::placeholders;

        auto config = cpp::bit::Object::decode( configText );
        detail->port = config.get( "port" ).asDecimal( );

        std::string keyPem = "";
        std::string certPem = "";
        std::string dhpPem = "";

        detail->tcp.open(
            io.get( ),
            detail->port,
            std::bind( &SessionServer::onConnect, this, _1, _2 ),
            std::bind( &SessionServer::onReceive, this, _1, _2 ),
            std::bind( &SessionServer::onDisconnect, this, _1, _2 ),
            "localhost", 
            cpp::TcpVersion::v6, 
            keyPem, 
            certPem, 
            dhpPem );
    }

    void SessionServer::close( )
    {
        detail->tcp.close( );
        detail->port = 0;
    }

    void SessionServer::onConnect( std::error_code acceptError, const std::string & addr )
    {

    }

    void SessionServer::onReceive( const std::string & addr, std::string & recvBuffer )
    {

    }

    void SessionServer::onDisconnect( const std::string & addr, std::error_code reason )
    {

    }

    SessionServer::Client::Client( )
    {

    }

    void SessionServer::Client::open( cpp::AsyncContext & io, std::string config )
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


    SessionServer::ResultCode SessionServer::Data::verifyConnection( const std::string & clientAddr, uint64_t * sessionId )
    {
        // verify clientAddr
        auto itr = clientSessions.find( clientAddr );
        if ( itr == clientSessions.end( ) )
            { return ResultCode::Arg; }
        *sessionId = itr->second;
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::verifyServer( uint64_t sessionId, std::string_view * service, int * nodeId )
    {
        auto itr = sessionServiceMap.find( sessionId );
        if ( itr == sessionServiceMap.end( ) )
            { return ResultCode::Arg; }
        auto & serverNode = itr->second;
        *service = serverNode.service;
        *nodeId = serverNode.nodeId;
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::verifyServerNetwork( std::string serviceName, std::string serverAddr )
    {
        auto addr = cpp::Inet4::toTcpEndpoint( serverAddr );
        auto serverNetwork = cpp::Inet4::toAddress( addr.address( ).to_string( ) );

        auto itr = ServiceNetwork.find( serviceName );
        if ( itr == ServiceNetwork.end() )
            { return ResultCode::Ok; }

        auto & validNetworks = itr->second;
        bool isSafeNetwork = validNetworks.empty( );
        for ( auto validNetwork : validNetworks )
        {
            auto network = cpp::Inet4::toAddress( validNetwork );
            if ( serverNetwork.is_subnet_of( network ) )
                { isSafeNetwork = true; break; }
        }
        if ( !isSafeNetwork )
            { return ResultCode::Access; }
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::grimauth(
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
            return ResultCode::Access;
        }
        return ResultCode::Ok;
    }

    uint64_t SessionServer::Data::makeSessionId( )
    {
        uint64_t sessionId = 0;
        while ( !sessionId || sessions.count( sessionId ) )
            { sessionId = rng.rand( ); }
        return sessionId;
    }

    SessionServer::ResultCode SessionServer::Data::verifySession(
        uint64_t sessionId,
        std::string_view * extAddr,
        std::string_view * email,
        uint64_t * userId,
        uint64_t * proxySessionId )
    {
        auto itr = sessions.find( sessionId );
        if ( itr == sessions.end( ) )
            { return ResultCode::Arg; }
        auto & sessionInfo = itr->second;
        *extAddr = sessionInfo.extAddr;
        *email = sessionInfo.email;
        *userId = sessionInfo.userId;
        *proxySessionId = sessionInfo.proxySessionId;
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::connected( std::string clientAddr )
    {
        auto itr = clientSessions.find( clientAddr );
        if ( itr != clientSessions.end( ) )
            { return ResultCode::Arg; }
        clientSessions[clientAddr] = 0;
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::disconnected( std::string clientAddr )
    {
        if ( !clientSessions.erase( clientAddr ) )
            { return ResultCode::Arg; }
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::authClient(
                                        std::string clientAddr,
                                        uint64_t authToken,
                                        int nodeId,
                                        uint64_t * sessionId )
    {
        uint64_t clientSessionId = 0;
        ResultCode result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != ResultCode::Ok )
            { return result; }

        result = verifyServerNetwork( ProxyServiceId, clientAddr );
        if ( result != ResultCode::Ok )
            { return result; }

        uint64_t userId;
        std::string email;
        result = grimauth( clientAddr, authToken, "grimethos.com", &userId, &email );
        if ( result != ResultCode::Ok )
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

        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::reauthClient(
                                            std::string clientAddr,
                                            uint64_t sessionId )
    {
        std::string oldAddr;
        ResultCode result = reauth( clientAddr, sessionId, clientAddr, &oldAddr );
        if ( result == ResultCode::Ok )
        { 
            if ( auto itr = clientSessions.find( oldAddr ); itr != clientSessions.end() )
                { clientSessions[oldAddr] = 0; }
            clientSessions[clientAddr] = sessionId; 
        }
        
        return result;
    }

    SessionServer::ResultCode SessionServer::Data::auth(
        std::string clientAddr,
        uint64_t authToken,
        std::string extAddr,
        uint64_t * sessionId )
    {
        uint64_t clientSessionId = 0;
        ResultCode result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != ResultCode::Ok )
            { return result; }

        std::string_view clientServiceName;
        int clientNodeId;
        result = verifyServer( clientSessionId, &clientServiceName, &clientNodeId );
        if ( result != ResultCode::Ok )
            { return result; }

        if ( clientServiceName != "proxy.grimethos.com" )
            { return ResultCode::Access; }

        uint64_t userId;
        std::string email;
        result = grimauth( extAddr, authToken, "grimethos.com", &userId, &email );
        if ( result != ResultCode::Ok )
            { return result; }

        uint64_t newSessionId = makeSessionId( );
        
        SessionInfo & sessionInfo = sessions[newSessionId];
        sessionInfo.extAddr = extAddr;
        sessionInfo.proxySessionId = clientSessionId;
        sessionInfo.email = email;
        sessionInfo.userId = userId;

        proxySessions[clientSessionId].insert( newSessionId );

        *sessionId = newSessionId;
        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::reauth(
        std::string clientAddr,
        uint64_t sessionId,
        std::string extAddr,
        std::string * oldExtAddr )
    {
        uint64_t clientSessionId = 0;
        ResultCode result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != ResultCode::Ok )
            { return result; }

        bool isProxyServer = clientAddr == extAddr;
        if ( isProxyServer )
            { clientSessionId = sessionId; }

        std::string_view clientServiceName;
        int clientNodeId;
        result = verifyServer( clientSessionId, &clientServiceName, &clientNodeId );
        if ( result != ResultCode::Ok )
            { return result; }

        std::string_view sessionExtAddr;
        std::string_view email;
        uint64_t userId = 0;
        uint64_t proxySessionId = 0;
        result = verifySession( sessionId, &sessionExtAddr, &email, &userId, &proxySessionId );
        if ( result != ResultCode::Ok )
            { return result; }

        auto oldAddr = cpp::Inet4::toTcpEndpoint( sessionExtAddr );
        auto newAddr = cpp::Inet4::toTcpEndpoint( extAddr );
        if ( oldAddr.address() != newAddr.address() )
            { return ResultCode::Access; }

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

        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::authServerNode(
        std::string clientAddr,
        uint64_t sessionId,
        std::string svcName, int nodeId )
    {
        uint64_t clientSessionId = 0;
        ResultCode result = verifyConnection( clientAddr, &clientSessionId );
        if ( result != ResultCode::Ok )
            { return result; }

        std::string_view clientServiceName;
        int clientNodeId;
        result = verifyServer( clientSessionId, &clientServiceName, &clientNodeId );
        if ( result != ResultCode::Ok )
            { return result; }

        std::string_view extAddr;
        std::string_view email;
        uint64_t userId = 0;
        uint64_t proxySessionId = 0;
        result = verifySession( sessionId, &extAddr, &email, &userId, &proxySessionId );
        if ( result != ResultCode::Ok )
            { return result; }

        if ( email != "monkeysmarts@gmail.com" )
            { return ResultCode::Access; }
        
        result = verifyServerNetwork( svcName, std::string{ extAddr } );
        if ( result != ResultCode::Ok )
            { return result; }

        ServerNode serverNode{ svcName, nodeId };
        serviceSessionMap[serverNode] = sessionId;
        sessionServiceMap[sessionId] = serverNode;

        return ResultCode::Ok;
    }

    SessionServer::ResultCode SessionServer::Data::openUdp(
        uint64_t sessionId,
        std::string intAddr,
        std::string udpAddr )
    {
        return ResultCode::Access;
    }

    SessionServer::ResultCode SessionServer::Data::closeUdp( uint64_t sessionId )
    {
        return ResultCode::Access;
    }

    SessionServer::ResultCode SessionServer::Data::lookupUdp(
        uint64_t sessionId,
        std::string * extAddr,
        std::string * intAddr,
        std::string * udpAddr,
        uint64_t * key )
    {
        return ResultCode::Access;
    }
}

namespace grim::test
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
        if (data.authClient( LOCAL_ADDR1, 1, 0, &proxySessionId[0] ) != SessionServer::ResultCode::Ok)
            { throw std::exception{ "data.authClient( LOCAL_ADDR1 )" }; }
        if ( data.authClient( LOCAL_ADDR2, 2, 1, &proxySessionId[1] ) != SessionServer::ResultCode::Ok )
            { throw std::exception{ "data.authClient( LOCAL_ADDR2 )" }; }

        data.connected( LOCAL_ADDR1_2 );
        if ( data.reauthClient( LOCAL_ADDR1_2, proxySessionId[0] ) != SessionServer::ResultCode::Ok )
            { throw std::exception{ "data.reauthClient( LOCAL_ADDR1_2 )" }; }

        uint64_t galaxySessionId;
        if ( data.auth( LOCAL_ADDR1_2, 1, REMOTE_ADDR1, &galaxySessionId ) != SessionServer::ResultCode::Ok )
            { throw std::exception{ "data.auth( LOCAL_ADDR1_2, 1, REMOTE_ADDR1 )" }; }
        if ( data.authServerNode( LOCAL_ADDR1_2, galaxySessionId, "galaxy.backwater.grimethos.com", 0 ) != SessionServer::ResultCode::Ok )
            { throw std::exception{ "data.authServerNode( LOCAL_ADDR1_2, galaxySessionId )" }; }
    }
}
