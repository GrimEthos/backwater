module;

#include <cinttypes>
#include <functional>
#include <system_error>

export module grim.net;
import cpp.asio.tcp;
import cpp.memory;
import cpp.chrono;
import grim.auth;

//! Types of network entities:
//! (1) session server
//! (2) proxy server
//! (3) server
//! (4) client
//! 
//! Types of connections:
//! (1) proxy server to session server
//! (2) server/client to proxy
//! (3) direct udp
//! 
//! session server clients
//! * authenticate
export namespace grim::net
{
    using                                   StrArg = const cpp::Memory &;
    using                                   StrOut = std::string *;
    using                                   Result = int;

    enum ResultCode { 
        Ok = 0,                             // 
        NoConnection = 0xffff,              // no connection to proxy
        NoRoute = 0xfffe,                   // to session can't be reached
        NoAuth = 0xfffd,                    // session can't be reached
        Retry = 0xfffc,                     // rate limiting
        More = 0xfffb,                      // multi-part message, bind will invoke same callback multiple times
        Timeout = 0xfffa,                   // timeout expired
    };

    struct Message
    {
        uint64_t                            toSessionId;
        uint64_t                            fromSessionId;
        uint64_t                            len : 16; // data.length() / 8
        uint64_t                            bind : 16;
        uint64_t                            type : 16; // optional hint about data contents
        uint64_t                            result : 16;
    };

    using                                   BindFn = std::function<void( const Message & msg, StrArg data )>;
    using                                   ConnectingFn = std::function<void( StrArg addr )>;
    using                                   ConnectFn = std::function<void(
                                                StrArg addr,
                                                Result result, std::string reason )>;
    using                                   AuthingFn = std::function<void(
                                                StrArg addr,
                                                StrArg access )>;
    using                                   AuthFn = std::function<void(
                                                StrArg email,
                                                uint64_t sessionId,
                                                Result result, std::string reason )>;
    using                                   ReadyFn = std::function<void( 
                                                StrArg addr, 
                                                Result result, std::string reason )>;
    using                                   DisconnectFn = std::function<void( 
                                                StrArg addr, 
                                                Result result, std::string reason )>;


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

        virtual void                        onConnecting( ConnectingFn ) = 0;
        virtual void                        onConnect( ConnectFn ) = 0;
        virtual void                        onAuthing( AuthingFn ) = 0;
        virtual void                        onAuth( AuthFn ) = 0;
        virtual void                        onReady( ReadyFn ) = 0;
        virtual void                        onDisconnect( DisconnectFn ) = 0;

        virtual void                        connect(
                                                int timeoutSeconds,
                                                ConnectFn ) = 0;
        virtual Result                      connect(
                                                int timeoutSeconds,
                                                StrOut address ) = 0;

        virtual void                        auth(
                                                int timeoutSeconds,
                                                AuthFn ) = 0;
        virtual Result                      auth(
                                                int timeoutSeconds,
                                                StrOut email,
                                                uint64_t * sessionId ) = 0;

        virtual void                        ready(
                                                int timeoutSeconds,
                                                ReadyFn ) = 0;
        virtual Result                      ready(
                                                int timeoutSeconds,
                                                Result result ) = 0;

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

    struct IProxyAPI
    {
        using                               AuthCallback = std::function<void( uint16_t result, uint64_t sessionId )>;
        virtual void                        auth(
                                                uint64_t authToken,
                                                cpp::Memory access,
                                                AuthCallback authCallback ) = 0;
    };

    class Client : public IClient
    {
    public:
        void                                open(
                                                cpp::AsyncContext & io,
                                                StrArg addrs,
                                                StrArg email,
                                                StrArg access ) override;
        void                                close( ) override;

        void                                onConnecting( ConnectingFn ) override;
        void                                onConnect( ConnectFn ) override;
        void                                onAuthing( AuthingFn ) override;
        void                                onAuth( AuthFn ) override;
        void                                onReady( ReadyFn ) override;
        void                                onDisconnect( DisconnectFn ) override;

        void                                connect(
                                                int timeoutSeconds,
                                                ConnectFn ) override;
        Result                              connect(
                                                int timeoutSeconds,
                                                StrOut address ) override;

        void                                auth(
                                                int timeoutSeconds,
                                                AuthFn ) override;
        Result                              auth(
                                                int timeoutSeconds,
                                                StrOut email,
                                                uint64_t * sessionId ) override;

        void                                ready(
                                                int timeoutSeconds,
                                                ReadyFn ) override;
        Result                              ready(
                                                int timeoutSeconds,
                                                Result result ) override;

        void                                send(                               // request
                                                uint64_t toSessionId,
                                                uint16_t type,
                                                cpp::Memory data,
                                                BindFn bindFunction ) override;
        void                                send(                               // reply
                                                uint64_t toSessionId,
                                                uint16_t bind,
                                                uint16_t type,
                                                uint16_t result,
                                                cpp::Memory data ) override;

        void                                openUdp( uint16_t port ) override;
        void                                closeUdp( ) override;
        void                                sendUdp(
                                                uint64_t toSessionId,
                                                cpp::Memory data ) override;
    private:
        void                                doConnect( );
        void                                doAuth( );
    private:
        cpp::AsyncContext                   io;
        std::vector<std::string>            addrs;
        std::string                         email;
        std::string                         access;
        cpp::TcpClient                      tcp;
        std::string                         caFilename;

        ConnectingFn                        onConnectingHandler;
        ConnectFn                           onConnectHandler;
        AuthingFn                           onAuthingHandler;
        AuthFn                              onAuthHandler;
        ReadyFn                             onReadyHandler;
        DisconnectFn                        onDisconnectHandler;

        std::string                         addr;
        uint64_t                            sessionId;
        uint16_t                            bindIndex;
    };
}

namespace grim::net
{
    static_assert( sizeof( Message ) == sizeof( uint64_t ) * 3 );

    void Client::open(
        cpp::AsyncContext & io,
        StrArg addrs,
        StrArg email,
        StrArg access )
    {
        this->io = io;
        for (auto &  addr : addrs.split( "," ))
            { this->addrs.push_back( addr ); }
        this->email = email;
        this->access = access;

        doConnect( );
    }

    Result toResult( std::error_code ec ) {
        if ( !ec )
            { return ResultCode::Ok; }
        else if ( ec.value( ) == (int)std::errc::connection_refused )
            { return ResultCode::NoConnection; }
        else
            { return ResultCode::NoConnection; }
    }

    void Client::doConnect( )
    {
        int index = (int)(( cpp::Time::now( ).sinceEpoch( ).millis( ) / 10 ) % addrs.size( ));
        this->addr = addrs[index];

        onConnectingHandler( this->addr );
        tcp.connect( io, addr, 
            [this]( std::error_code connectResult )
            { 
                onConnectHandler( this->addr, toResult(connectResult), connectResult.message() );
                if ( connectResult )
                { 
                    doConnect( );
                }
                doAuth( );
            }, 
            [this]( std::string & recvBuffer )
            { 
            }, 
            [this]( std::error_code reason )
            { 
                onDisconnectHandler( this->addr, toResult( reason ), reason.message( ) );
                // disconnected
            }, caFilename );
    }

    void Client::doAuth( )
    {
        onAuthingHandler( this->addr, this->access );
        uint64_t authToken = grim::auth::login( this->email, "grimethos.com" );

    }

    void Client::close( )
    {
        tcp.disconnect( );
    }

    void Client::onConnecting( ConnectingFn )
    {
    }
    void Client::onConnect( ConnectFn )
    {
    }
    void Client::onAuthing( AuthingFn )
    {
    }
    void Client::onAuth( AuthFn )
    {
    }
    void Client::onReady( ReadyFn )
    {
    }
    void Client::onDisconnect( DisconnectFn )
    {
    }

    void Client::connect(
        int timeoutSeconds,
        ConnectFn )
    {
    }
    Result Client::connect(
        int timeoutSeconds,
        StrOut address )
    {
        return ResultCode::Timeout;
    }

    void Client::auth(
        int timeoutSeconds,
        AuthFn )
    {
    }

    Result Client::auth(
        int timeoutSeconds,
        StrOut email,
        uint64_t * sessionId )
    {
        return ResultCode::Timeout;
    }

    void Client::ready(
        int timeoutSeconds,
        ReadyFn )
    {
    }

    Result Client::ready(
        int timeoutSeconds,
        Result result )
    {
        return ResultCode::Timeout;
    }

    void Client::send(
        uint64_t toSessionId,
        uint16_t type,
        cpp::Memory data,
        BindFn bindFunction )
    {
    }

    void Client::send(
        uint64_t toSessionId,
        uint16_t bind,
        uint16_t type,
        uint16_t result,
        cpp::Memory data )
    {
    }

    void Client::openUdp( uint16_t port )
    {
    }

    void Client::closeUdp( )
    {
    }

    void Client::sendUdp(
        uint64_t toSessionId,
        cpp::Memory data )
    {
    }

}