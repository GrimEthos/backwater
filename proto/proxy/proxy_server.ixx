module;

#include <string>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <system_error>

export module proxy;
import cpp.bit;


export namespace grim
{
    class ProxyServer
    {
    public:
        ProxyServer( std::string configText );

        void                                open( std::string config );
        void                                close( );

        void                                connected( std::string extAddr );
        void                                disconnected( std::string extAddr );

        void                                auth( 
                                                std::string extAddr, 
                                                uint64_t authToken,
                                                uint64_t * sessionId,
                                                std::error_code * err,
                                                std::string * reason );
        void                                reclaimAuth(
                                                std::string extAddr,
                                                uint64_t sessionId,
                                                std::error_code * err,
                                                std::string * reason );

        //! authServerNode creates a lookup for the sessionId of the
        //! identified service & nodeId
        void                                authServerNode(
                                                std::string extAddr,
                                                std::string svcName, int nodeId,
                                                std::error_code * err,
                                                std::string * reason );
        void                                lookupServerNode(
                                                std::string extAddr,
                                                std::string svcName, int nodeId,
                                                uint64_t * sessionId,
                                                std::error_code * err,
                                                std::string * reason );

    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SampleServer
    {
    public:
        SampleServer( std::string config );
    private:
        struct LookupReply
        {

        };
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

    class SampleClient
    {
    public:
        SampleClient( std::string config );
    private:
        struct                              Detail;
        std::unique_ptr<Detail>             detail = std::make_unique<Detail>( );
    };

}


namespace grim
{
    struct ProxyServer::Detail
    {
        int port;
    };

    ProxyServer::ProxyServer( std::string configText )
    {
        auto config = cpp::bit::Object::decode( configText );
    }



    struct SampleServer::Detail
    {
        int port;
    };

    SampleServer::SampleServer( std::string configText )
    {
        auto config = cpp::bit::Object::decode( configText );
    }



    struct SampleClient::Detail
    {
        int port;
    };

    SampleClient::SampleClient( std::string configText )
    {
        auto config = cpp::bit::Object::decode( configText );
    }
}