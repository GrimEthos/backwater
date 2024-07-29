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