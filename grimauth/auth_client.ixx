module;

#include <exception>
#include <memory>

export module grim.auth.client;

import cpp.asio;
import cpp.file;
import cpp.windows;
import grim.arch.auth;

export namespace grim::auth
{
    constexpr const char * DefaultUrl = "https://auth.grimethos.com";
    constexpr const char * DefaultFolder = "GrimEthos";

    class Client : public IClient
    {
    public:
                                                Client( );

        void                                    setAsyncContext( cpp::AsyncContext io );
        void                                    setAuthDataDir( cpp::FilePath authDataDir );

        //! performs timecode() and id(). If necessary (i.e. result == Pending) performs check() in 
        //! a loop until the user approves or denies the login or a timeout occurs.  If the 
        //! `LoginOption::Interactive` is set, a browser window to the login console will be opened.
        Result                                  login(
                                                    UserEmail email,
                                                    ServiceId serviceId,
                                                    int options,
                                                    int timeoutSeconds,
                                                    AuthToken * authToken ) override;

        cpp::AsyncCall                          login(
                                                    UserEmail email,
                                                    ServiceId serviceId,
                                                    int options,
                                                    int timeoutSeconds,
                                                    LoginFn ) override;

        cpp::AsyncCall                          login(
                                                    AuthToken authToken,
                                                    int timeoutSeconds,
                                                    LoginFn ) override;

        void                                    authInit(
                                                    AuthToken serviceAuthToken,
                                                    authInitFn callback ) override;

        void                                    auth(
                                                    AuthToken serviceToken,
                                                    DeviceIP userIp,
                                                    AuthToken userToken,
                                                    authFn callback ) override;

        static const cpp::FilePath &            DefaultAuthDataDir();
    private:
        cpp::AsyncContext                       m_io;
        cpp::FilePath                           m_authDataDir;
    };
}

namespace grim::auth
{
    const cpp::FilePath & Client::DefaultAuthDataDir( )
    {
        static cpp::FilePath defaultAuthDataDir = 
            cpp::windows::Shell::getPath( cpp::windows::Shell::UserAppData ) / DefaultFolder;
        return defaultAuthDataDir;
    }

    Client::Client( )
    {
        m_authDataDir = DefaultAuthDataDir( );
    }

    void Client::setAsyncContext( cpp::AsyncContext io )
    {
        m_io = std::move( io );
    }

    void Client::setAuthDataDir( cpp::FilePath authDataDir )
    {
        m_authDataDir = std::move( authDataDir );
    }

    Result Client::login(
        UserEmail email,
        ServiceId serviceId,
        int options,
        int timeoutSeconds,
        AuthToken * authToken )
    {
        if ( !m_io.get( ) ) { throw std::exception{ "" }; }
        return Result::Ok;
    }

    struct LoginContext : 
        public cpp::AsyncCancellable
    {
        bool isCancelled = false;
        cpp::AsyncTimer timer;
        void cancel( ) override
        { 
            isCancelled = true; 
            timer.cancel( );
        }
    };

    /*
    uint64_t login( const cpp::Memory & email, const cpp::Memory & serviceId )
    {
        cpp::Random rng;
        cpp::FilePath authdataPath = cpp::windows::Shell::getPath( cpp::windows::Shell::KnownFolder::UserAppData ) / "GrimEthos" / ".grimauth";
        cpp::bit::BitFile authdata{ authdataPath };

        std::string passkeyLabel = std::format( "passKey[{}]", email.view );

        // hostId='abcdef0123456789'
        // hostKey='abcdef0123456789'
        // passKey[x@y.com]='abcdef0123456789'
        auto hostId = authdata.get( "hostId" );
        auto hostKey = authdata.get( "hostKey" );
        auto passKey = authdata.get( passkeyLabel );
        // if no passKey, generate one
        if ( !passKey )
        {
            std::string newPassKey =
                cpp::Encoder::intToHex( rng.rand( ), 0, true, false ) +
                cpp::Encoder::intToHex( rng.rand( ) ) +
                cpp::Encoder::intToHex( rng.rand( ) ) +
                cpp::Encoder::intToHex( rng.rand( ) );
            authdata.set( passkeyLabel, newPassKey );
            passKey = authdata.get( passkeyLabel );
        }

        httplib::SSLClient http( "https://auth.grimethos.com" );
        http.set_ca_cert_path( "./ca-bundle.crt" );
        //http.enable_server_certificate_verification( false );
        http.set_connection_timeout( 0, 300000 ); // 300 milliseconds
        http.set_read_timeout( 5, 0 ); // 5 seconds
        http.set_write_timeout( 5, 0 ); // 5 seconds

        std::string body;
        // timecode
        body = "";
        auto res = http.Get( "/api/timecode", {
                { "email", "john" },
                { "service_id", "coder" } },
                [&]( const char * data, size_t data_length ) {
                body.append( data, data_length );
                return true; } );
        if ( res->status != 200 )
        { throw std::exception( "timecode failed" ); }

        // id
        body = "";
        res = http.Get( "/api/id", {
                { "email", email.data( ) },
                { "service_id", serviceId.data( ) },
                { "timestamp", "coder" },
                { "passcode", "coder" },
                { "host_id", "coder" },
                { "host_name", "coder" },
                { "secret", "coder" } },
                [&]( const char * data, size_t data_length ) {
                body.append( data, data_length );
                return true; } );
        if ( res->status != 200 )
        { throw std::exception( "id failed" ); }

        // maybe save credential
        // maybe open console
        // wait for auth
        //while ( !confirm() )
        //    { sleep( 1s ); }
        uint64_t authToken = 0;
        return authToken;
    }
    */

    cpp::AsyncCall Client::login(
        UserEmail email,
        ServiceId serviceId,
        int options,
        int timeoutSeconds,
        LoginFn handler )
    {
        auto context = std::make_shared<LoginContext>( );
        context->timer = m_io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [handler]( ) 
            { 
                handler( grim::auth::Result::Timeout, { 0 } ); 
            } );
        // to do - all these results are fake
        if ( email.value == "timeout@test.com" )
            {  }
        else if ( email.value == "denied@test.com" )
            { m_io.post( [=]( ) { context->timer.cancel( ); handler( grim::auth::Result::Denied, { 0 } ); } ); }
        else if ( email.value == "pending@test.com" )
            { m_io.post( [=]( ) { context->timer.cancel( ); handler( grim::auth::Result::Pending, { 0 } ); } ); }
        else if ( email.value == "retry@test.com" )
            { m_io.post( [=]( ) { context->timer.cancel( ); handler( grim::auth::Result::Retry, { 0 } ); } ); }
        else
            { m_io.post( [=]( ) 
                { 
                    context->timer.cancel( ); 
                    handler( grim::auth::Result::Ok, { 1 } );
                } ); }
        return context;
    }

    cpp::AsyncCall Client::login(
        AuthToken authToken,
        int timeoutSeconds,
        LoginFn handler )
    {
        auto context = std::make_shared<LoginContext>( );
        context->timer = m_io.waitFor( cpp::Duration::ofSeconds( timeoutSeconds ), [handler]( )
            {
                handler( grim::auth::Result::Timeout, { 0 } );
            } );
        m_io.post( [=]( )
            {
                context->timer.cancel( );
                handler( grim::auth::Result::Ok, { 1 } );
            } );
        return context;
    }

    void Client::authInit(
        AuthToken serviceAuthToken,
        authInitFn handler )
    {
        // to do - all these results are fake
        m_io.post( [=]( ) { handler( grim::auth::Result::Ok ); } );
    }

    void Client::auth(
        AuthToken serviceToken,
        DeviceIP userIp,
        AuthToken userToken,
        authFn handler )
    {
        // to do - all these results are fake
        m_io.post( [=]( ) { handler( grim::auth::Result::Ok, { 1 }, { "monkeysmarts@gmail.com" } ); } );
    }

}