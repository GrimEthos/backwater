module;

#include <system_error>
#include <string>
#include <vector>

export module grim.net.client;

import grim.arch.net;
import cpp.asio.tcp;

export namespace grim::net
{
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
    void Client::open(
        cpp::AsyncContext & io,
        StrArg addrs,
        StrArg email,
        StrArg access )
    {
        this->io = io;
        for ( auto & addr : addrs.split( "," ) )
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
        int index = (int)( ( cpp::Time::now( ).sinceEpoch( ).millis( ) / 10 ) % addrs.size( ) );
        this->addr = addrs[index];

        if ( onConnectingHandler ) 
            { onConnectingHandler( this->addr ); }
        tcp.connect( io, addr,
            [this]( std::error_code connectResult )
            {
                if ( onConnectHandler )
                    { onConnectHandler( this->addr, toResult( connectResult ), connectResult.message( ) ); }
                if ( connectResult )
                    { doConnect( ); }
                else
                    { doAuth( ); }
            },
            [this]( std::string & recvBuffer )
            {
            },
            [this]( std::error_code reason )
            {
                if ( onDisconnectHandler )
                    { onDisconnectHandler( this->addr, toResult( reason ), reason.message( ) ); }
                // disconnected
            }, caFilename );
    }

    void Client::doAuth( )
    {
        if ( onAuthingHandler )
            { onAuthingHandler( this->addr, this->access ); }
        //uint64_t authToken = grim::auth::login( this->email, "grimethos.com" );
    }

    void Client::close( )
    {
        tcp.disconnect( );
    }

    void Client::onConnecting( ConnectingFn fn )
    {
        onConnectingHandler = std::move( fn );
    }

    void Client::onConnect( ConnectFn fn )
    {
        onConnectHandler = std::move( fn );
    }

    void Client::onAuthing( AuthingFn fn )
    {
        onAuthingHandler = std::move( fn );
    }

    void Client::onAuth( AuthFn fn )
    {
        onAuthHandler = std::move( fn );
    }

    void Client::onReady( ReadyFn fn )
    {
        onReadyHandler = std::move( fn );
    }

    void Client::onDisconnect( DisconnectFn fn )
    {
        onDisconnectHandler = std::move( fn );
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