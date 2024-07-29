module;

#include <chrono>
#include <functional>
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
                                                Result * result ) override;

        void                                ready(
                                                int timeoutSeconds,
                                                ReadyFn ) override;
        bool                                ready(
                                                int timeoutSeconds,
                                                Result * result ) override;

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
        void                                notifyConnecting( const std::string & addr );
        void                                notifyConnect( const std::string & addr, net::Result result, const std::string & reason );
        void                                notifyAuthing( const std::string & email, const std::string & privs );
        void                                notifyAuth( StrArg email, uint64_t sessionId, Result result );
        void                                notifyReady( );

        void                                doConnect( );
        void                                doAuthLogin( );
        void                                onAuthLogin( grim::auth::Result result, grim::auth::AuthToken authToken );
        void                                doHello( );
        void                                onHello( Result result, StrArg email, uint64_t sessionId );
        void                                doRello( );
        void                                onRello( Result result );
        void                                authReady( grim::auth::Result result );


        void                                onConnect( StrArg addr, Result result, std::string reason );
        void                                onAuth( StrArg email, uint64_t sessionId, Result result );
        void                                onReady( Result result );
    private:
        cpp::AsyncContext                   io;
        std::vector<std::string>            addrs;
        std::string                         email;
        std::string                         access;
        cpp::TcpClient                      tcp;
        std::string                         caFilename;
        grim::auth::Client                  grimauth;
        grim::auth::AuthToken               authToken;

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
    struct ProxyApi : public IProxyApi 
    {
        enum class                          MessageType { Hello, Rello, AuthServer, FindServer };

                                            ProxyApi( Client & client );

        void                                hello( auth::AuthToken authToken, HelloReply reply ) override;
        void                                rello( uint64_t sessionId, RelloReply reply ) override;
        void                                authServer( StrArg svcName, int nodeId, AuthServerReply reply ) override;
        void                                findServer( StrArg svcName, int nodeId, FindServerReply reply ) override;

    private:
        Client & m_client;
    };

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
            { return Result::Ok; }
        else if ( ec.value( ) == (int)std::errc::connection_refused )
            { return Result::Route; }
        else
            { return Result::Route; }
    }

    void Client::notifyConnecting( const std::string & addr )
    {
        if ( onConnectingHandler )
            { onConnectingHandler( addr ); }
    }

    void Client::notifyConnect( const std::string & addr, net::Result result, const std::string & reason )
    {
        if ( onConnectHandler )
            { onConnectHandler( addr, result, reason ); }
        if ( connectHandler )
            { connectHandler( addr, result, reason ); }
    }

    void Client::notifyAuthing( const std::string & email, const std::string & privs )
    {
        if ( onAuthingHandler )
            { onAuthingHandler( email, privs ); }
    }

    void Client::notifyAuth( StrArg email, uint64_t sessionId, Result result )
    {
        if ( onAuthHandler )
            { onAuthHandler( email, sessionId, result ); }
        if ( authHandler )
            { authHandler( email, sessionId, result ); }
    }

    void Client::notifyReady( )
    {
        if ( onReadyHandler )
            { onReadyHandler( Result::Ok ); }
        if ( readyHandler )
            { readyHandler( Result::Ok ); }
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
                    { doAuthLogin( ); }
                else
                    { doRello( ); }
            },
            [this]( std::string & recvBuffer )
            {
            },
            [this]( std::error_code reason )
            {
                if ( onDisconnectHandler )
                    { onDisconnectHandler( this->addr, toResult( reason ), reason.message( ) ); }
                doConnect( );
                // disconnected
            }, caFilename );
    }

    void Client::doAuthLogin( )
    {
        using namespace std::placeholders;
        int options = grim::auth::LoginOption::Interactive;
        grimauth.login(
            grim::auth::UserEmail{ email },
            grim::auth::ServiceId{ "backwater.grimethos.com" },
            options,
            5,
            std::bind( &Client::onAuthLogin, this, _1, _2 ) );
        notifyAuthing( email, "" );
    }

    void Client::onAuthLogin( grim::auth::Result result, grim::auth::AuthToken authToken )
    {
        using grim::auth::Result;

        switch ( result )
        {
        case Result::Ok:
            authToken = authToken;
            doHello( );
            break;
        case Result::Pending:
            // this shouldn't happen
            break;
        case Result::Denied:
        case Result::Timeout:
            break;
        case Result::Retry:
            io.waitFor( cpp::Duration::ofSeconds( 60 ), [this]( ) { doAuthLogin( ); } );
            break;
        }
    }

    void Client::doHello( )
    {
        using namespace std::placeholders;

        ProxyApi proxy{ *this };
        proxy.hello( authToken, std::bind( &Client::onHello, this, _1, _2, _3 ) );
    }

    void Client::onHello( Result result, StrArg email, uint64_t sessionId )
    {
        if ( result != Result::Ok )
        {
            cpp::Log::error( "onHello() : result={}", std::to_underlying( result ) );
            io.waitFor( cpp::Duration::ofSeconds( 60 ), [this]( ) { doAuthLogin( ); } );
            return;
        }
    }

    void Client::doRello( )
    {
        using namespace std::placeholders;

        ProxyApi proxy{ *this };
        proxy.rello( sessionId, std::bind( &Client::onRello, this, _1 ) );
    }

    void Client::onRello( Result result )
    {
        if ( result != Result::Ok )
        {
            doAuthLogin( );
            return;
        }
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

        cpp::AsyncTimer timeout = io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [=,this]( )
            {
                fn( addr, Result::Timeout, "Timeout" );
                connectHandler = nullptr;
            } );
        connectHandler = [=,this]( StrArg address, Result result, std::string reason )
            { 
                if ( result == Result::Ok )
                {
                    cpp::AsyncTimer{ timeout }.cancel( );
                    fn( address, result, reason );
                }
                if ( addr.empty( ) )
                {
                    cpp::AsyncTimer{ timeout }.cancel( );
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

    void Client::auth(
        int timeoutSeconds,
        AuthFn )
    {
    }

    bool Client::auth(
        int timeoutSeconds,
        StrOut email,
        uint64_t * sessionId,
        Result * result )
    {
        Result outResult = Result::Timeout;
        return outResult == Result::Ok;
    }

    void Client::ready(
        int timeoutSeconds,
        ReadyFn )
    {
    }

    bool Client::ready(
        int timeoutSeconds,
        Result * result )
    {
        Result outResult = Result::Timeout;
        return outResult == Result::Ok;
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


    ProxyApi::ProxyApi( Client & client )
        : m_client(client)
    {
    }

    void ProxyApi::hello( auth::AuthToken authToken, HelloReply handler )
    {
        cpp::StringBuffer request;
        request.putBinary( authToken, ByteOrder );

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
                catch ( std::exception & e ) { result = Result::Unknown; }

                handler( result, email, sessionId );
            } );
    }

    void ProxyApi::rello( uint64_t sessionId, RelloReply handler )
    {
        cpp::StringBuffer request;
        request.putBinary( sessionId, ByteOrder );

        m_client.send( 0, (int)MessageType::Rello, request.getAll( ), [handler]( const Message & msg, StrArg data )
            {
                Result result = toResult( msg.result );
                handler( result );
            } );
    }

    void ProxyApi::authServer( StrArg svcName, int nodeId, AuthServerReply handler )
    {
        cpp::StringBuffer request;
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
                catch ( std::exception & e ) { result = Result::Unknown; }

                handler( result, email, sessionId );
            } );
    }

    void ProxyApi::findServer( StrArg svcName, int nodeId, FindServerReply handler )
    {
        cpp::StringBuffer request;
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
                catch ( std::exception & e ) { result = Result::Unknown; }

                handler( result, sessionId );
            } );
    }

}