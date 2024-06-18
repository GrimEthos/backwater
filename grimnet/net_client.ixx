module;

#include <system_error>
#include <string>
#include <vector>
#include <chrono>

export module grim.net.client;

import grim.arch.net;
import cpp.asio.tcp;
import cpp.thread;

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
        bool                                connect(
                                                int timeoutSeconds,
                                                StrOut address,
                                                Result * result, StrOut reason ) override;

        void                                auth(
                                                int timeoutSeconds,
                                                AuthFn ) override;
        bool                                auth(
                                                int timeoutSeconds,
                                                StrOut email,
                                                uint64_t * sessionId,
                                                Result * result, StrOut reason ) override;

        void                                ready(
                                                int timeoutSeconds,
                                                ReadyFn ) override;
        bool                                ready(
                                                int timeoutSeconds,
                                                Result * result, StrOut reason ) override;

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

        void                                onConnect( StrArg addr, Result result, std::string reason );
        void                                onAuth( StrArg email, uint64_t sessionId, Result result, std::string reason );
        void                                onReady( StrArg addr, Result result, std::string reason );
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

        ConnectFn                           connectHandler;
        AuthFn                              authHandler;
        ReadyFn                             readyHandler;

        uint64_t                            isConnected : 1;
        uint64_t                            isAuthed : 1;
        uint64_t                            isReady : 1;

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
        if ( !addrs.size( ) ) { return; }
        int index = (int)( ( cpp::Time::now( ).sinceEpoch( ).millis( ) / 10 ) % addrs.size( ) );
        this->addr = addrs[index];

        if ( onConnectingHandler ) 
            { onConnectingHandler( this->addr ); }
        tcp.connect( *io, addr,
            [this]( std::error_code connectResult )
            {
                onConnect( addr, toResult( connectResult ), connectResult.message( ) );
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

    void Client::onConnect( StrArg address, Result result, std::string reason )
    {
        if ( onConnectHandler )
            { onConnectHandler( address, result, reason ); }
        if ( connectHandler )
            { connectHandler( address, result, reason ); }
    }

    void Client::onAuth( StrArg email, uint64_t sessionId, Result result, std::string reason )
    {
        if ( onAuthHandler )
            { onAuthHandler( email, sessionId, result, reason ); }
        if ( authHandler )
            { authHandler( email, sessionId, result, reason ); }
    }

    void Client::onReady( StrArg addr, Result result, std::string reason )
    {
        if ( onReadyHandler )
            { onReadyHandler( addr, result, reason ); }
        if ( readyHandler )
            { readyHandler( addr, result, reason ); }
    }

    void Client::close( )
    {
        addrs.clear( );
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
        ConnectFn fn )
    {
        if ( connectHandler )
            { io.post( [=, this]( ) { fn( addr, ResultCode::Retry, "Retry" ); } ); return; }
        if ( addr.empty() )
            { io.post( [=, this]( ) { fn( addr, ResultCode::NoConnection, "No connection" ); } ); return; }
        if ( isConnected )
            { io.post( [=, this]( ) { fn( addr, ResultCode::Ok, "Ok" ); } ); return; }

        cpp::AsyncTimer timeout = io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [=,this]( )
            {
                connectHandler = nullptr;
                fn( addr, ResultCode::Timeout, "Timeout" );
            } );
        connectHandler = [=,this]( StrArg address, Result result, std::string reason )
            { 
                if ( result == ResultCode::Ok )
                {
                    cpp::AsyncTimer{ timeout }.cancel( );
                    connectHandler = nullptr;
                    fn( address, result, reason );
                }
                if ( addr.empty( ) )
                {
                    cpp::AsyncTimer{ timeout }.cancel( );
                    connectHandler = nullptr;
                    fn( addr, ResultCode::NoConnection, "No connection" );
                }
            };
    }

    bool Client::connect(
        int timeoutSeconds,
        StrOut address,
        Result * result, StrOut reason )
    {
        bool isDone = false;
        Result outResult = ResultCode::Timeout;
        connect( timeoutSeconds, [&]( StrArg inAddr, Result inResult, std::string inReason )
            {
                if ( address ) { *address = inAddr; }
                if ( result ) { *result = inResult; }
                if ( reason ) { *reason = inReason; }
                outResult = inResult;
                isDone = true;
            } );
        while ( !isDone ) { io.runOne( ); }
        return outResult == ResultCode::Ok;
    }

    void Client::auth(
        int timeoutSeconds,
        AuthFn )
    {
    }

    bool Client::auth(
        int timeoutSeconds,
        StrOut email,
        uint64_t * sessionId,
        Result * result, StrOut reason )
    {
        Result outResult = ResultCode::Timeout;
        return outResult == ResultCode::Ok;
    }

    void Client::ready(
        int timeoutSeconds,
        ReadyFn )
    {
    }

    bool Client::ready(
        int timeoutSeconds,
        Result * result, StrOut reason )
    {
        Result outResult = ResultCode::Timeout;
        return outResult == ResultCode::Ok;
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