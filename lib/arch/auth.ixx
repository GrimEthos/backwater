module;

#include <cstdint>
#include <string>
#include <format>
#include <vector>
#include <functional>

export module grim.arch.auth;

import cpp.asio;
import cpp.memory;
import cpp.random;
import cpp.file;
import cpp.windows;
import cpp.bit.file;

export namespace grim::auth
{
    struct                                  UserId      { uint64_t value; };
    struct                                  UserEmail   { std::string value; };
    struct                                  UserPIN     { std::string value; };
    struct                                  HostId      { uint64_t value; };
    struct                                  HostKey     { std::string value; }; // 32B value (256b)
    struct                                  PassKey     { std::string value; }; // 32B value (256b)
    struct                                  Password    { std::string value; }; // SHA-256(PassKey + UserPIN)
    struct                                  Passcode    { std::string value; }; // SHA-256(Password + HostKey + Timecode)
    struct                                  DeviceId    { uint64_t value; };
    struct                                  DeviceIP    { std::string value; };
    struct                                  DeviceName  { std::string value; };
    struct                                  Timestamp   { uint32_t value; };
    struct                                  Timecode    { Timestamp t; uint32_t nonce; };
    struct                                  AuthToken   { uint64_t value; };
    struct                                  ServiceId   { std::string value; };

    struct LoginOption
    {
        static constexpr int                Default     = 0;
        static constexpr int                Interactive = 1 << 0;
    };
    enum class                              Result { Ok, Pending, Denied, Timeout, Retry };

    struct IClient
    {        
        //! login
        virtual Result                      login(
                                                UserEmail email,
                                                ServiceId serviceId,
                                                int options,
                                                int timeoutSeconds,
                                                AuthToken * authToken ) = 0;
        //! performs timecode() and id(). If necessary (i.e. result == Pending) performs check() in 
        //! a loop until the user approves or denies the login or a timeout occurs.  If the 
        //! `LoginOption::Interactive` is set, a browser window to the login console will be opened.
        using                               LoginFn = std::function<void( Result, AuthToken )>;
        virtual cpp::AsyncCall              login(
                                                UserEmail email,
                                                ServiceId serviceId,
                                                int options,
                                                int timeoutSeconds,
                                                LoginFn ) = 0;

        virtual cpp::AsyncCall               login(
                                                AuthToken authToken,
                                                int timeoutSeconds,
                                                LoginFn ) = 0;
        //! authInit
        using                               authInitFn = std::function<void( Result )>;
        virtual void                        authInit(
                                                AuthToken serviceAuthToken,
                                                authInitFn callback ) = 0;
        //! auth
        using                               authFn = std::function<void( Result, UserId, UserEmail )>;
        virtual void                        auth(
                                                AuthToken serviceToken,
                                                DeviceIP userIp,
                                                AuthToken userToken,
                                                authFn callback ) = 0;

        struct Data;
    };

    struct IClient::Data
    {
        // makePasscode
        virtual Passcode                    makePasscode(
                                                PassKey,
                                                UserPIN,
                                                Timecode,
                                                HostKey ) = 0;
        virtual Result                      getHostInfo(
                                                cpp::FilePath authDataDir, 
                                                HostId * hostId, 
                                                HostKey * hostKey ) = 0;
        virtual Result                      listEmails( 
                                                cpp::FilePath authDataDir, 
                                                std::vector<UserEmail> * emailList ) = 0;
        virtual Result                      getPasskey( 
                                                cpp::FilePath authDataDir, 
                                                UserEmail, 
                                                PassKey * passkey ) = 0;
        virtual Result                      savePasskey( 
                                                cpp::FilePath authDataDir, 
                                                UserEmail, 
                                                PassKey ) = 0;
        virtual Result                      openConsole( UserEmail email ) = 0;
        virtual Result                      openConsole( AuthToken authToken ) = 0;
    };

    struct IServer
    {
        struct AuthApi;
        struct UserApi;
        struct Emailer;
        struct Security;
        struct Data;
    };

    struct IServer::AuthApi
    {
        //! idInit
        using                               timecodeCallback = std::function<void( Result, Timecode )>;
        virtual void                        timecode(
                                                DeviceIP ip,
                                                cpp::Memory email,
                                                cpp::Memory svc,
                                                timecodeCallback callback ) = 0;

        //! id - creates an auth token which can be used for authentication.  This token must be
        //! approved by the user (via the auth console) before an auth request will succeed.
        using                               idCallback = std::function<void( Result, AuthToken, HostId, HostKey )>;
        virtual void                        id(
                                                DeviceIP ip,
                                                cpp::Memory email,
                                                cpp::Memory svc,
                                                Timestamp timestamp,
                                                Passcode passcode,
                                                HostId hostId,
                                                cpp::Memory hostName,
                                                cpp::Memory secret,
                                                idCallback callback ) = 0;
        //! check - returns whether an authToken has been approved or denied
        using                               checkCallback = std::function<void( Result )>;
        virtual void                        check(
                                                DeviceIP ip,
                                                AuthToken authToken,
                                                checkCallback callback ) = 0;
        //! confirm
        using                               confirmCallback = std::function<void( Result )>;
        virtual void                        confirm(
                                                DeviceIP ip,
                                                AuthToken authToken,
                                                cpp::Memory confirmCode,
                                                confirmCallback callback ) = 0;
        //! authInit
        using                               authInitCallback = std::function<void( Result )>;
        virtual void                        authInit(
                                                DeviceIP serviceIp,
                                                AuthToken serviceToken,
                                                authInitCallback callback ) = 0;
        //! auth
        using                               authCallback = std::function<void( Result, UserId, UserEmail )>;
        virtual void                        auth(
                                                DeviceIP serviceIp,
                                                AuthToken serviceToken,
                                                DeviceIP userIp,
                                                AuthToken userToken,
                                                authCallback callback ) = 0;
    };


    enum ApprovalFlag
    {
        Approved            = 1<<0, //
        Denied              = 1<<1, //
        Timeout             = 1<<2,
        PendingConfirm      = 1<<3, // waiting on email confirmation
        PendingNewDevice    = 1<<4, // waiting on new device confirmation
        PendingNewHost      = 1<<5, // waiting on new HostId confirmation
        PendingNewService   = 1<<6, // waiting on new service login confirmation
        PendingNewRegion    = 1<<7, // waiting on new ip login confirmation (svc requires stable ip)
        PendingNewIp        = 1<<8, // waiting on new ip login confirmation (svc requires stable ip)
        PendingNewLogin     = 1<<9, // waiting on new login confirmation (svc config requires re-confirm)
    };
    using Approval = int;
    struct UserLogin
    { 
        AuthToken                           authToken; 
        ServiceId                           svcId; 
        HostId                              hostId; 
        DeviceIP                            ip; 
        Timestamp                           timestamp; 
        Approval                            status; 
    };
    struct UserDevice
    { 
        HostId                              hostId; 
        Timestamp                           firstTime; 
        Timestamp                           lastTime; 
        std::vector<DeviceName>             names; 
        std::vector<DeviceIP>               ips; 
        std::vector<ServiceId>              svcs; 
    };
    struct UserRestrictions
    {
        std::vector<DeviceIP>               ips; 
        std::vector<ServiceId>              svcs; 
    };


    //! * A device is registered by calling `id` using a null HostId.
    //! * A device is correlated from `id` using HostId, IP, and HostName:
    //!     * HostId always correlates to a single device'
    //!     * IP + HostName correlates HostIds
    //! * A device name is 'HostName (region)'
    struct IServer::UserApi
    {
        //! getLogin
        using                               getLoginFn = std::function<void(
                                                Result result, 
                                                const UserLogin & login, 
                                                const UserDevice & device )>;
        virtual void                        getLogin( AuthToken authToken, getLoginFn );

        //! approveLogin
        using                               approveLoginFn = std::function<void( Result result )>;
        virtual void                        approveLogin( AuthToken authToken, approveLoginFn );

        //! denyLogin
        using                               denyLoginFn = std::function<void( Result result )>;
        virtual void                        denyLogin( AuthToken authToken, denyLoginFn );

        //! getUserInfo
        using                               getUserInfoCallback = std::function<void( 
                                                Result result, 
                                                const std::vector<UserEmail> & emails, 
                                                const UserRestrictions & restrictions )>;
        virtual void                        getUserInfo( UserEmail email, getUserInfoCallback ) = 0;

        //! listDevice
        using                               listDeviceCallback = std::function<void( 
                                                Result result, 
                                                int page, int pageSize, 
                                                const std::vector<UserDevice> & devices )>;
        virtual void                        listDevice( UserEmail email, int page, int pageSize, listDeviceCallback ) = 0;

        //! listAuth
        using                               listLoginCallback = std::function<void( 
                                                Result result, 
                                                int page, int pageSize, 
                                                const std::vector<UserLogin> & logins )>;
        virtual void                        listLogin( UserEmail email, HostId hostId, ServiceId svcId, int page, int pageSize, listLoginCallback ) = 0;
    };

    struct IServer::Emailer
    {
        enum class                          Status { Ok, Error, Pending };

        using                               sendCallback = std::function<void( uint64_t emailContext )>;
        virtual void                        send( UserEmail to, UserEmail from, std::string content, sendCallback ) = 0;

        using                               getCallback = std::function<void( time_t createTime, time_t sendTime, int status )>;
        virtual void                        get( uint64_t emailContext, getCallback ) = 0;
    };

    struct IServer::Data
    {
        using                               createHostCallback = std::function<void( Result )>;
        virtual void                        createHost(
                                                HostId hostId,
                                                HostKey hostKey,
                                                cpp::Memory hostName,
                                                DeviceIP ip,
                                                createHostCallback ) = 0;

        struct                              Host { HostId hostId; };
        using                               getHostCallback = std::function<void( Result, Host )>;
        virtual void                        getHost( HostId hostId, getHostCallback ) = 0;


        using                               createLoginCallback = std::function<void( Result, Timecode )>;
        virtual void                        createLogin(
                                                DeviceIP ip,
                                                cpp::Memory email,
                                                cpp::Memory svcId,
                                                Passcode passcode,
                                                HostId hostId,
                                                cpp::Memory hostName,
                                                cpp::Memory secret,
                                                createLoginCallback ) = 0;

        using                               updateUserCallback = std::function<void( Result )>;
        virtual void                        updateUser(
                                                UserId userId,
                                                UserEmail email,
                                                cpp::Memory secret ) = 0;

        using                               updateUserSubnetCallback = std::function<void( Result )>;
        virtual void                        updateUserSubnet(
                                                UserId userId,
                                                cpp::Memory subnet,
                                                bool isAllowed ) = 0;

        using                               removeUserSubnetCallback = std::function<void( Result )>;
        virtual void                        removeUserSubnet(
                                                UserId userId,
                                                cpp::Memory subnet,
                                                bool isAllowed ) = 0;

        using                               updateUserServiceCallback = std::function<void( Result )>;
        virtual void                        updateUserService(
                                                UserId userId,
                                                ServiceId svcId,
                                                bool isAllowed ) = 0;

        using                               removeUserServiceCallback = std::function<void( Result )>;
        virtual void                        removeUserService(
                                                UserId userId,
                                                ServiceId svcId ) = 0;

        using                               createAuthCallback = std::function<void( Result )>;
        virtual void                        createAuth(
                                                HostId hostId,
                                                HostKey hostKey,
                                                cpp::Memory hostName,
                                                DeviceIP ip,
                                                createHostCallback ) = 0;
    };

    struct IServer::Security
    {
        enum class                          Result { Ok, Error };

        using                               auditCallback = std::function<void( Result )>;
        virtual void                        audit( DeviceIP, UserEmail, auditCallback ) = 0;
        virtual void                        audit( DeviceIP, Timecode, HostId, auditCallback ) = 0;
        virtual void                        audit( DeviceIP, AuthToken, auditCallback ) = 0;

        virtual void                        recordInit( DeviceIP, UserEmail ) = 0;
        virtual void                        recordId( DeviceIP, UserEmail, HostId, bool isSuccess ) = 0;
        virtual void                        recordConfirm( DeviceIP, AuthToken, bool isSuccess ) = 0;
        virtual void                        recordAuth( DeviceIP, AuthToken ) = 0;
        virtual void                        recordRead( DeviceIP, AuthToken ) = 0;
        virtual void                        recordWrite( DeviceIP, AuthToken ) = 0;
    };

}

