module;

export module grim.auth.client;

import cpp.asio;
import cpp.file;
import cpp.windows;
import grim.arch.auth;

export namespace grim::auth
{
    class Client : public IClient
    {
    public:
                                                Client( );

        void                                    setAsyncContext( cpp::AsyncContext io );
        void                                    setAuthDataDir( cpp::FilePath & authDataDir );

        Result                                  login(
                                                    UserEmail email,
                                                    ServiceId serviceId,
                                                    int options,
                                                    int timeoutSeconds,
                                                    AuthToken * authToken ) override;

        void                                    login(
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
        static cpp::FilePath defaultAuthDataDir = cpp::windows::Shell::getPath( cpp::windows::Shell::UserAppData ) / "GrimEthos";
    }

    Client::Client( )
    {
        m_authDataDir = DefaultAuthDataDir( );
    }

    void Client::setAsyncContext( cpp::AsyncContext io )
    {
        m_io = std::move( io );
    }

    void Client::setAuthDataDir( cpp::FilePath & authDataDir )
    {
        m_authDataDir = authDataDir;
    }

    Result Client::login(
        UserEmail email,
        ServiceId serviceId,
        int options,
        int timeoutSeconds,
        AuthToken * authToken )
    {

    }

    void Client::login(
        UserEmail email,
        ServiceId serviceId,
        int options,
        int timeoutSeconds,
        LoginFn )
    {

    }

    void Client::authInit(
        AuthToken serviceAuthToken,
        authInitFn callback )
    {

    }

    void Client::auth(
        AuthToken serviceToken,
        DeviceIP userIp,
        AuthToken userToken,
        authFn callback )
    {

    }

}