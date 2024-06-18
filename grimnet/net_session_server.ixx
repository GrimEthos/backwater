module;

#include <cinttypes>
#include <set>
#include <map>
#include <functional>
#include <system_error>
#include <memory>

export module grim.net.session_server;

import cpp.random;
import grim.arch.net;

export namespace grim::net
{
    class SessionServer 
        : public ISessionServer
    {
    public:
                                            SessionServer( );

        void                                open(
                                                cpp::AsyncContext & io,
                                                StrArg listenAddress,
                                                StrArg email,
                                                uint8_t nodeId ) override;
        void                                close( ) override;

        void                                onAuthing( AuthingFn ) override;
        void                                onAuth( AuthFn ) override;
        void                                onReady( ReadyFn ) override;

        void                                onConnect( ConnectFn ) override;
        void                                onDisconnect( ConnectFn ) override;

        void                                onHello( StrArg ip, uint64_t authToken, int nodeId ) override;
        void                                onRello( StrArg ip, uint64_t sessionId, int nodeId ) override;

        void                                onAuth( StrArg ip, StrArg extIp, uint64_t authToken ) override;
        void                                onReauth( StrArg ip, StrArg extIp, uint64_t sessionId ) override;
        void                                onAuthServer( StrArg ip, uint64_t sessionId, StrArg svcName, int nodeId ) override;

        void                                onLookupSession( StrArg ip, uint64_t sessionId ) override;
        void                                onLookupServer( StrArg ip, StrArg svcName, int nodeId ) override;

        class                               Data;
        class                               Client;

    private:
        void                                onConnect( std::error_code acceptError, const std::string & addr );
        void                                onReceive( const std::string & addr, std::string & recvBuffer );
        void                                onDisconnect( const std::string & addr, std::error_code reason );

    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SessionServer::Client
        : ISessionServer::ClientApi
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
        ResultCode                          connected( std::string clientAddr );
        ResultCode                          disconnected( std::string clientAddr );
        ResultCode                          hello(
                                                std::string clientAddr,
                                                uint64_t authToken,
                                                int nodeId,
                                                uint64_t * sessionId );
        ResultCode                          rello(
                                                std::string clientAddr,
                                                uint64_t sessionId );

        ResultCode                          auth(
                                                std::string clientAddr,
                                                uint64_t authToken,
                                                std::string extAddr,
                                                uint64_t * sessionId );
        ResultCode                          reauth(
                                                std::string clientAddr,
                                                uint64_t sessionId,
                                                std::string extAddr,
                                                std::string * oldExtAddr );
        ResultCode                          authServerNode(
                                                std::string clientAddr,
                                                uint64_t sessionId,
                                                std::string svcName, int nodeId );

        ResultCode                          openUdp(
                                                uint64_t sessionId,
                                                std::string intAddr,
                                                std::string udpAddr );
        ResultCode                          closeUdp( uint64_t sessionId );
        ResultCode                          lookupUdp(
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

        ResultCode                          verifyConnection(
                                                const std::string & clientAddr,
                                                uint64_t * sessionId );
        ResultCode                          verifyServer(
                                                uint64_t sessionId,
                                                std::string_view * service,
                                                int * nodeId );
        ResultCode                          verifySession(
                                                uint64_t sessionId,
                                                std::string_view * extAddr,
                                                std::string_view * email,
                                                uint64_t * userId,
                                                uint64_t * proxySessionId );
        ResultCode                          verifyServerNetwork(
                                                std::string serviceName,
                                                std::string addr );
        ResultCode                          grimauth(
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
