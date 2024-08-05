module;

#include <cassert>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <system_error>
#include <vector>

export module grim.net.client;

import cpp.asio.tcp;
import cpp.buffer;
import cpp.log;
import cpp.thread;
import grim.arch.net;
import grim.auth;

export namespace grim::net
{
    class Client : public IClient
    {
    public:
        void                                open(
                                                cpp::AsyncContext & io,
                                                StrArg email,
                                                StrArg addrs );
        void                                open(
                                                cpp::AsyncContext & io,
                                                StrArg email,
                                                auth::AuthToken authToken,
                                                StrArg addrs );
        void                                close( );

        void                                onIdentifying( IdentifyingFn ) override;
        void                                onIdentify( IdentifyFn ) override;
        void                                onConnecting( ConnectingFn ) override;
        void                                onConnect( ConnectFn ) override;
        void                                onAuthing( AuthingFn ) override;
        void                                onAuth( AuthFn ) override;
        void                                onReady( ReadyFn ) override;
        void                                onDisconnect( DisconnectFn ) override;

        void                                identify( int timeoutSeconds, IdentifyFn );
        bool                                identify(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut email,
                                                StrOut pendingUrl );
        void                                connect( int timeoutSeconds, ConnectFn ) override;
        bool                                connect(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut address,
                                                StrOut reason ) override;
        void                                auth( int timeoutSeconds, AuthFn ) override;
        bool                                auth(
                                                int timeoutSeconds,
                                                Result * result,
                                                StrOut email,
                                                uint64_t * sessionId ) override;
        void                                ready( int timeoutSeconds, ReadyFn ) override;
        bool                                ready(
                                                int timeoutSeconds,
                                                Result * result ) override;

        void                                send(                               // request
                                                uint64_t toSessionId,
                                                uint8_t type,
                                                cpp::Memory data,
                                                BindFn bindFunction ) override;
        void                                send(                               // reply
                                                uint64_t toSessionId,
                                                uint16_t moniker,
                                                uint16_t bind,
                                                uint8_t type,
                                                uint8_t result,
                                                cpp::Memory data ) override;

        cpp::AsyncContext &                 getAsyncContext( );
    private:
        void                                notifyIdentifying( const std::string & email );
        void                                notifyIdentify( net::Result result, const std::string & email, const std::string & pendingUrl );
        void                                notifyConnecting( const std::string & addr );
        void                                notifyConnect( net::Result result, const std::string & addr, const std::string & reason );
        void                                notifyAuthing( const std::string & email );
        void                                notifyAuth( Result result, StrArg email, uint64_t sessionId );
        void                                notifyReady( );

        void                                doIdentify( );
        void                                didIdentify( grim::auth::Result result, grim::auth::AuthToken authToken );
        void                                doConnect( );
        void                                doHello( );
        void                                didHello( Result result, StrArg email, uint64_t sessionId );
        void                                doRello( );
        void                                didRello( Result result );
        void                                authReady( grim::auth::Result result );

        void                                handlerStart( int timeoutSeconds, std::function<void( )> fn );
        Result                              handlerWait( );
    private:
        cpp::AsyncContext                   io;
        std::vector<std::string>            addrs;
        std::string                         email;
        std::string                         access;
        cpp::TcpClient                      tcp;
        std::string                         caFilename;
        grim::auth::Client                  grimauth;
        grim::auth::AuthToken               authToken;

        IdentifyingFn                       onIdentifyingHandler;
        IdentifyFn                          onIdentifyHandler;
        ConnectingFn                        onConnectingHandler;
        ConnectFn                           onConnectHandler;
        AuthingFn                           onAuthingHandler;
        AuthFn                              onAuthHandler;
        ReadyFn                             onReadyHandler;
        DisconnectFn                        onDisconnectHandler;

        IdentifyFn                          identifyHandler;
        ConnectFn                           connectHandler;
        AuthFn                              authHandler;
        ReadyFn                             readyHandler;
        cpp::AsyncTimer                     handlerTimer;
        cpp::AsyncCondition<Result>         handlerCond;

        uint64_t                            isIdentified : 1;
        uint64_t                            isConnected : 1;
        uint64_t                            isAuthed : 1;
        uint64_t                            isReady : 1;

        std::string                         addr;
        uint64_t                            sessionId;
        uint16_t                            bindIndex;
        std::map<uint16_t, BindFn>          bindMap;
    };
}

namespace grim::net
{
    struct ProxyApi : public INetServerApi, IProxyApi 
    {
        enum class                          MessageType { Hello, Rello, AuthServer, FindServer };

                                            ProxyApi( Client & client );

        void                                hello( auth::AuthToken authToken, OnHello ) override;
        void                                rello( uint64_t sessionId, OnRello ) override;
        void                                authServer( StrArg svcName, int nodeId, AuthServerReply reply ) override;
        void                                findServer( StrArg svcName, int nodeId, FindServerReply reply ) override;
    private:
        Client & m_client;
    };

    void Client::open(
        cpp::AsyncContext & io,
        StrArg email,
        StrArg addrs )
    {
        open( io, email, auth::AuthToken{ 0 }, addrs );
    }

    void Client::open(
        cpp::AsyncContext & io,
        StrArg email,
        auth::AuthToken authToken,
        StrArg addrs )
    {
        this->io = io;
        this->grimauth.setAsyncContext( io );
        for ( auto & addr : addrs.split( "," ) )
            { this->addrs.push_back( addr ); }
        this->email = email;
        this->authToken = authToken;
        this->access = access;
        this->sessionId = 0;

        if ( this->authToken.value == 0 )
            { doIdentify( ); }
        else
            { doConnect( ); }
    }

    Result toResult( uint64_t code ) {
        if ( code == (uint64_t)Result::Ok )
            { return Result::Ok; }
        if ( code >= (uint64_t)Result::Arg && code < (uint64_t)Result::Unknown )
            { return (Result)code; }
        return Result::Unknown;
    }

    Result toResult( std::error_code ec ) {
        if ( !ec )
            { return Result::Ok; }
        else if ( ec.value( ) == (int)std::errc::connection_refused )
            { return Result::Route; }
        else
            { return Result::Route; }
    }

    void Client::notifyIdentifying( const std::string & email )
    {
        if ( onIdentifyingHandler )
            { onIdentifyingHandler( email ); }
    }

    void Client::notifyIdentify( net::Result result, const std::string & email, const std::string & pendingUrl )
    {
        if ( onIdentifyHandler )
            { onIdentifyHandler( result, addr, pendingUrl ); }
        if ( identifyHandler )
            { identifyHandler( result, addr, pendingUrl ); }
    }

    void Client::notifyConnecting( const std::string & addr )
    {
        if ( onConnectingHandler )
            { onConnectingHandler( addr ); }
    }

    void Client::notifyConnect( net::Result result, const std::string & addr, const std::string & reason )
    {
        if ( onConnectHandler )
            { onConnectHandler( result, addr, reason ); }
        if ( connectHandler )
            { connectHandler( result, addr, reason ); }
    }

    void Client::notifyAuthing( const std::string & email )
    {
        if ( onAuthingHandler )
            { onAuthingHandler( email ); }
    }

    void Client::notifyAuth( net::Result result, StrArg email, uint64_t sessionId )
    {
        if ( onAuthHandler )
            { onAuthHandler( result, email, sessionId ); }
        if ( authHandler )
            { authHandler( result, email, sessionId ); }
    }

    void Client::notifyReady( )
    {
        if ( onReadyHandler )
            { onReadyHandler( Result::Ok ); }
        if ( readyHandler )
            { readyHandler( Result::Ok ); }
    }

    void Client::doIdentify( )
    {
        using namespace std::placeholders;
        int options = grim::auth::LoginOption::Interactive;
        int timeout = 5;
        if ( authToken.value )
        {
            grimauth.login(
                this->authToken,
                timeout,
                std::bind( &Client::didIdentify, this, _1, _2 ) );
        }
        else
        {
            grimauth.login(
                grim::auth::UserEmail{ email },
                grim::auth::ServiceId{ "backwater.grimethos.com" },
                options,
                5,
                std::bind( &Client::didIdentify, this, _1, _2 ) );
        }
        notifyIdentifying( email );
    }

    void Client::didIdentify( grim::auth::Result result, grim::auth::AuthToken authToken )
    {
        std::string pendingUrl;
        switch ( result )
        {
        case grim::auth::Result::Ok:
            this->authToken = authToken;
            notifyIdentify( Result::Ok, email, "" );
            doConnect( );
            break;
        case grim::auth::Result::Pending:
            notifyIdentify( Result::Ok, email, pendingUrl );
            doConnect( );
            break;
        case grim::auth::Result::Denied:
            notifyIdentify( Result::Access, email, pendingUrl );
            break;
        case grim::auth::Result::Timeout:
            notifyIdentify( Result::Timeout, email, pendingUrl );
            io.waitFor( cpp::Duration::ofSeconds( 10 ), [this]( ) { doIdentify( ); } );
        case grim::auth::Result::Retry:
            notifyIdentify( Result::Retry, email, pendingUrl );
            io.waitFor( cpp::Duration::ofSeconds( 60 ), [this]( ) { doIdentify( ); } );
            break;
        }
    }

    void Client::doConnect( )
    {
        if ( !addrs.size( ) ) { return; }
        int index = (int)( ( cpp::Time::now( ).sinceEpoch( ).millis( ) / 10 ) % addrs.size( ) );
        this->addr = addrs[index];

        notifyConnecting( this->addr );
        tcp.connect( *io, addr,
            [this]( std::error_code connectResult )
            {
                notifyConnect( addr, toResult( connectResult ), connectResult.message( ) );
                if ( connectResult )
                    { doConnect( ); }
                else if ( sessionId == 0 )
                    { doHello( ); }
                else
                    { doRello( ); }
            },
            [this]( std::string & recvBuffer )
            {
            },
            [this]( std::error_code reason )
            {
                isConnected = isAuthed = isReady = false;
                if ( onDisconnectHandler )
                    { onDisconnectHandler( this->addr, toResult( reason ), reason.message( ) ); }
                doConnect( );
                // disconnected
            }, caFilename );
    }

    void Client::doHello( )
    {
        using namespace std::placeholders;

        ProxyApi proxy{ *this };
        proxy.hello( authToken, std::bind( &Client::didHello, this, _1, _2, _3 ) );
    }

    void Client::didHello( Result result, StrArg email, uint64_t sessionId )
    {
        if ( result != Result::Ok )
        {
            cpp::Log::error( "onHello() : result={}", std::to_underlying( result ) );
            io.waitFor( cpp::Duration::ofSeconds( 60 ), [this]( ) { doAuthLogin( ); } );
            return;
        }
        this->sessionId = sessionId;
        this->isAuthed = true;
        notifyAuth( this->email, this->sessionId, result );
        notifyReady( );
    }

    void Client::doRello( )
    {
        using namespace std::placeholders;

        ProxyApi proxy{ *this };
        proxy.rello( sessionId, std::bind( &Client::didRello, this, _1 ) );
    }

    void Client::didRello( Result result )
    {
        if ( result != Result::Ok )
        {
            doAuthLogin( );
            return;
        }
        this->isAuthed = true;
        notifyAuth( this->email, this->sessionId, result );
        notifyReady( );
    }

    void Client::onConnect( StrArg address, Result result, std::string reason )
    {
        if ( onConnectHandler )
            { onConnectHandler( address, result, reason ); }
        if ( connectHandler )
            { connectHandler( address, result, reason ); }
    }

    void Client::onAuth( StrArg email, uint64_t sessionId, Result result )
    {
        if ( onAuthHandler )
            { onAuthHandler( email, sessionId, result ); }
        if ( authHandler )
            { authHandler( email, sessionId, result ); }
    }

    void Client::onReady( Result result )
    {
        if ( onReadyHandler )
            { onReadyHandler( result ); }
        if ( readyHandler )
            { readyHandler( result ); }
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

    void Client::handlerStart( int timeoutSeconds, std::function<void()> fn )
    {
        handlerCond.reset( );
        handlerTimer = io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this, fn]( )
            { fn( ); handlerCond.notify( Result::Timeout ); } );
    }

    Result Client::handlerWait( )
    {
        return handlerCond.wait( );
    }

    void Client::connect(
        int timeoutSeconds,
        ConnectFn fn )
    {
        if ( connectHandler )
            { io.post( [=, this]( ) { fn( addr, Result::Retry, "Retry" ); } ); return; }
        if ( addr.empty() )
            { io.post( [=, this]( ) { fn( addr, Result::Route, "Route" ); } ); return; }
        if ( isConnected )
            { io.post( [=, this]( ) { fn( addr, Result::Ok, "Ok" ); } ); return; }

        handlerStart( timeoutSeconds, [this, fn]()
            { 
                fn( addr, Result::Timeout, "Timeout" ); 
                connectHandler = nullptr; 
            } );
        connectHandler = [=,this]( StrArg address, Result result, std::string reason )
            { 
                if ( result == Result::Ok )
                {
                    handlerTimer.cancel( );
                    fn( address, result, reason );
                }
                if ( addr.empty( ) )
                {
                    handlerTimer.cancel( );
                    fn( addr, Result::Route, "Route" );
                }
                connectHandler = nullptr;
            };
    }

    bool Client::connect(
        int timeoutSeconds,
        StrOut address,
        Result * result, StrOut reason )
    {
        bool isDone = false;
        Result outResult = Result::Timeout;
        connect( timeoutSeconds, [&]( StrArg inAddr, Result inResult, std::string inReason )
            {
                if ( address ) { *address = inAddr; }
                if ( result ) { *result = inResult; }
                if ( reason ) { *reason = inReason; }
                outResult = inResult;
                isDone = true;
            } );
        while ( !isDone ) { io.runOne( ); }
        return outResult == Result::Ok;
    }

    void Client::auth( int timeoutSeconds, AuthFn fn )
    {
        // if isReady, post result immediately
        if ( isAuthed )
            { io.post( [this, fn]( ) { fn( email, 0, Result::Ok ); } ); return; }

        // start a timer that will return timeout if it elapses before the readyHandler is called
        authHandler = std::move( fn );
        handlerTimer = io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this]( )
            { authHandler( email, 0, Result::Timeout ); } );
    }

    bool Client::auth(
        int timeoutSeconds,
        StrOut emailOut,
        uint64_t * sessionIdOut,
        Result * result )
    {
        *result = Result::Ok;
        if ( !isAuthed )
        {
            auto isAuthedCond = io.createCondition<Result>( );
            authHandler = [&]( StrArg email, uint64_t sessionId, Result authResult )
                { isAuthedCond.notify( authResult ); };
            *result = isAuthedCond.wait( );
        }
        *emailOut = email;
        *sessionIdOut = 0;

        return *result == Result::Ok;
    }

    void Client::ready(
        int timeoutSeconds,
        ReadyFn fn )
    {
        // if isReady, post result immediately
        if ( isReady )
            { io.post( [this, fn]( ) { fn( Result::Ok ); } ); return; }

        // start a timer that will return timeout if it elapses before the readyHandler is called
        readyHandler = std::move( fn );
        handlerTimer = io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [this]( )
            { readyHandler( Result::Timeout ); } );
    }

    bool Client::ready(
        int timeoutSeconds,
        Result * result )
    {
        *result = Result::Ok;
        if ( !isReady )
        {
            auto isReadyCond = io.createCondition<Result>( );
            readyHandler = [&]( Result readyResult )
                { isReadyCond.notify( readyResult ); };
            *result = isReadyCond.wait( );
        }
        return *result == Result::Ok;
    }

    void Client::send(
        uint64_t toSessionId,
        uint8_t type,
        cpp::Memory data,
        BindFn bindFunction )
    {
        uint16_t bind = 0;
        if ( bindFunction )
        {
            while ( !bind || bindMap.count( bind ) ) { bind = bindIndex++; }
            bindMap[bind] = std::move( bindFunction );
        }

        uint64_t t = cpp::Time::now( ).sinceEpoch( ).micros( );
        uint16_t moniker = ( t & 0xffff ) ^ ( ( t >> 16 ) & 0xffff ) ^ ( ( t >> 32 ) & 0xffff ) ^ ( ( t >> 48 ) & 0xffff ) ^ bind;

        assert( data.length( ) < 0xffff * 8 );
        uint16_t len = (uint16_t)( ( data.length( ) + 7 ) / 8 );
        size_t padding = ( 8 - ( data.length( ) % 8 ) ) % 8;

        uint8_t result = 0;

        auto request = cpp::StringBuffer::writeTo( 24 );
        request.putBinary( len, ByteOrder );
        request.putBinary( moniker, ByteOrder );
        request.putBinary( bind, ByteOrder );
        request.putBinary( type, ByteOrder );
        request.putBinary( result, ByteOrder );
        request.putBinary( toSessionId, ByteOrder );
        request.putBinary( sessionId, ByteOrder );

        tcp.send( request.getAll( ) );
        tcp.send( data );
        if ( padding )
            { tcp.send( std::string( padding, '\0' ) ); }
    }

    void Client::send(
        uint64_t toSessionId,
        uint16_t moniker,
        uint16_t bind,
        uint8_t type,
        uint8_t result,
        cpp::Memory data )
    {
        assert( data.length( ) < 0xffff * 8 );
        uint16_t len = (uint16_t)( ( data.length( ) + 7 ) / 8 );
        size_t padding = ( 8 - ( data.length( ) % 8 ) ) % 8;

        auto request = cpp::StringBuffer::writeTo( 24 );
        request.putBinary( len, ByteOrder );
        request.putBinary( moniker, ByteOrder );
        request.putBinary( bind, ByteOrder );
        request.putBinary( type, ByteOrder );
        request.putBinary( result, ByteOrder );
        request.putBinary( toSessionId, ByteOrder );
        request.putBinary( sessionId, ByteOrder );

        tcp.send( request.getAll( ) );
        tcp.send( data );
        if ( padding )
            { tcp.send( std::string( padding, '\0' ) ); }
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


    cpp::AsyncContext & Client::getAsyncContext( )
    {
        return io;
    }


    ProxyApi::ProxyApi( Client & client )
        : m_client(client)
    {
    }

    void ProxyApi::hello( auth::AuthToken authToken, HelloReply handler )
    {
        auto request = cpp::StringBuffer::writeTo(256);
        request.putBinary( authToken.value, ByteOrder );

        // fake response
        m_client.getAsyncContext( ).post( [handler]( ) 
            { 
                handler( Result::Ok, "placeholder@x.com", 1 );
            } );

        /*
        m_client.send( 0, (int)MessageType::Hello, request.getAll( ), [handler]( const Message & msg, StrArg data )
            {
                Result result = toResult( msg.result );
                std::string email;
                uint64_t sessionId;

                try
                {
                    cpp::DataBuffer reply{ data };
                    reply.getBinary( email, ByteOrder );
                    reply.getBinary( sessionId, ByteOrder );
                }
                catch ( std::exception & ) { result = Result::Unknown; }

                handler( result, email, sessionId );
            } );
        */
    }

    void ProxyApi::rello( uint64_t sessionId, RelloReply handler )
    {
        auto request = cpp::StringBuffer::writeTo( 256 );
        request.putBinary( sessionId, ByteOrder );

        m_client.send( 0, (int)MessageType::Rello, request.getAll( ), [handler]( const Message & msg, StrArg data )
            {
                Result result = toResult( msg.result );
                handler( result );
            } );
    }

    void ProxyApi::authServer( StrArg svcName, int nodeId, AuthServerReply handler )
    {
        auto request = cpp::StringBuffer::writeTo( 256 );
        request.putBinary( svcName, ByteOrder );
        request.putBinary( nodeId, ByteOrder );

        m_client.send( 0, (int)MessageType::AuthServer, request.getAll( ), [handler]( const Message & msg, StrArg data )
            {
                Result result = toResult( msg.result );
                std::string email;
                uint64_t sessionId;

                try
                {
                    cpp::DataBuffer reply{ data };
                    reply.getBinary( email, ByteOrder );
                    reply.getBinary( sessionId, ByteOrder );
                }
                catch ( std::exception & ) { result = Result::Unknown; }

                handler( result, email, sessionId );
            } );
    }

    void ProxyApi::findServer( StrArg svcName, int nodeId, FindServerReply handler )
    {
        auto request = cpp::StringBuffer::writeTo( 256 );
        request.putBinary( svcName, ByteOrder );
        request.putBinary( nodeId, ByteOrder );

        m_client.send( 0, (int)MessageType::AuthServer, request.getAll( ), [handler]( const Message & msg, StrArg data )
            {
                Result result = toResult( msg.result );
                uint64_t sessionId;

                try
                {
                    cpp::DataBuffer reply{ data };
                    reply.getBinary( sessionId, ByteOrder );
                }
                catch ( std::exception & ) { result = Result::Unknown; }

                handler( result, sessionId );
            } );
    }

}