module;
#include <cstdint>
#include <string>
#include <format>
#include <vector>
#include <functional>

export module grim.proto.auth;

import cpp.memory;
import cpp.random;
import cpp.file;
import cpp.windows;
import cpp.bit.file;

/*
    std::filesystem::path DefaultGrimAuthPath = "./.grimauth";

    bool isInteractive;
    uint64_t authToken;

    // blocking & interactive
    isInteractive = true;
    authToken = grim::auth::login(
        "monkeysmarts@gmail.com",
        "backwater.grimethos.com",
        DefaultGrimAuthPath,
        isInteractive );

    // non-blocking & interactive
    isInteractive = true;
    grim::auth::login(
        io,
        "monkeysmarts@gmail.com",
        "backwater.grimethos.com",
        DefaultGrimAuthPath,
        isInteractive,
        []( grim::auth::Result result, uint64_t token, std::string_view authUrl )
        {
            authToken = token;
        } );

    // non-blocking & passive
    isInteractive = false;
    authToken = grim::auth::login(
        io,
        "monkeysmarts@gmail.com",
        "backwater.grimethos.com",
        DefaultGrimAuthPath,
        false,
        []( grim::auth::Result result, uint64_t token, std::string_view authUrl )
        {
            authToken = token;
        } );
*/

export namespace grim
{
    namespace auth
    {
        struct                              UserId       { uint64_t value; };
        struct                              UserEmail    { std::string value; };
        struct                              UserPIN      { std::string value; };
        struct                              HostId       { uint64_t value; };
        struct                              HostKey      { std::string value; }; // 32B value (256b)
        struct                              PassKey      { std::string value; }; // 32B value (256b)
        struct                              Password     { std::string value; }; // SHA-256(PassKey + UserPIN)
        struct                              Passcode     { std::string value; }; // SHA-256(Password + HostKey + Timecode.nonce)
        struct                              DeviceId     { uint64_t value; };
        struct                              DeviceIP     { std::string value; };
        struct                              DeviceName   { std::string value; };
        struct                              Timestamp    { uint32_t value; };
        struct                              Timecode     { Timestamp t; uint32_t nonce; };
        struct                              AuthToken    { uint64_t value; };
        struct                              ServiceId    { std::string value; };

        struct IAuth
        {
            enum class                      Result { Ok, Error };
            // makePasscode
            virtual Passcode                makePasscode(
                                                PassKey, 
                                                UserPIN, 
                                                Timecode, 
                                                HostKey ) = 0;
            //! idInit
            using                           timecodeCallback = std::function<void( Result, Timecode )>;
            virtual void                    timecode(
                                                DeviceIP ip,
                                                cpp::Memory email, 
                                                cpp::Memory svc, 
                                                timecodeCallback callback ) = 0;

            //! id - creates an auth token which can be used for authentication.  This token must be
            //! approved by the user (via the auth console) before an auth request will succeed.
            using                           idCallback = std::function<void( Result, AuthToken, HostId, HostKey )>;
            virtual void                    id(
                                                DeviceIP ip,
                                                cpp::Memory email,
                                                cpp::Memory svc,
                                                Timestamp timestamp,
                                                Passcode passcode,
                                                HostId hostId,
                                                cpp::Memory hostName,
                                                cpp::Memory secret,
                                                idCallback callback ) = 0;
            //! confirm
            using                           confirmCallback = std::function<void( Result )>;
            virtual void                    confirm(
                                                DeviceIP ip,
                                                AuthToken authToken,
                                                cpp::Memory confirmCode,
                                                confirmCallback callback ) = 0;
            //! authInit
            using                           authInitCallback = std::function<void( Result )>;
            virtual void                    authInit(
                                                DeviceIP serviceIp,
                                                AuthToken serviceToken,
                                                authInitCallback callback ) = 0;
            //! auth
            using                           authCallback = std::function<void( Result, UserId, UserEmail )>;
            virtual void                    auth(
                                                DeviceIP serviceIp,
                                                AuthToken serviceToken,
                                                DeviceIP userIp,
                                                AuthToken userToken,
                                                authCallback callback ) = 0;
        };


        enum Approval 
        { 
            Approved,           //
            Denied,             //
            Timeout,    
            PendingConfirm,     // waiting on email confirmation
            PendingNewDevice,   // waiting on new device confirmation
            PendingNewHost,     // waiting on new HostId confirmation
            PendingNewService,  // waiting on new service login confirmation
            PendingNewRegion,   // waiting on new ip login confirmation (svc requires stable ip)
            PendingNewIp,       // waiting on new ip login confirmation (svc requires stable ip)
            PendingNewLogin,    // waiting on new login confirmation (svc config requires re-confirm)
        };
        
        
        //! * A device is registered by calling `id` using a null HostId.
        //! * A device is correlated from `id` using HostId, IP, and HostName:
        //!     * HostId always correlates to a single device'
        //!     * IP + HostName correlates HostIds
        //! * A device name is 'HostName (region)'
        struct IAuthUser
        {
            enum class                      Result { Ok, Error };

            //! getUserInfo
            struct                          UserInfo { UserId id; std::vector<UserEmail> emails; };
            using                           getUserInfoCallback = std::function<void( Result result, const UserInfo & email )>;
            virtual void                    getUserInfo( UserId, getUserInfoCallback ) = 0;

            //! listDevice
            struct                          DeviceLogin
                                                { HostId hostId; ServiceId svcId; Timestamp timestamp; Approval approval; };
            struct                          UserDevice 
                                                { DeviceId deviceId; std::vector<DeviceName> names; std::vector<DeviceIP> ips; std::vector<DeviceLogin> logins; };
            using                           listDeviceCallback = std::function<void( Result result, const std::vector<UserDevice> & devices )>;
            virtual void                    listDevice( UserId, listDeviceCallback ) = 0;

            //! listAuth
            struct                          UserAuth { DeviceId deviceId; DeviceName deviceName; DeviceIP ip; ServiceId svcId; Timestamp timestamp; Approval approval; };
            using                           listAuthCallback = std::function<void( Result result, int page, int pageSize, const std::vector<UserAuth> & auths )>;
            virtual void                    listAuth( UserId, bool pendingOnly, int page, int pageSize, listAuthCallback ) = 0;
        };

        struct IEmailer 
        {
            enum class                      Status { Ok, Error, Pending };

            using                           sendCallback = std::function<void( uint64_t emailContext )>;
            virtual void                    send( UserEmail to, UserEmail from, std::string content, sendCallback ) = 0;

            using                           getCallback = std::function<void( time_t createTime, time_t sendTime, int status )>;
            virtual void                    get( uint64_t emailContext, getCallback ) = 0;
        };

        struct IAuthDB
        {
            enum class                      Result { Ok, Error };

            using                           createTimecodeCallback = std::function<void( Result, Timecode )>;
            virtual void                    createTimecode( cpp::Memory email, cpp::Memory svc, createTimecodeCallback ) = 0;

            using                           getTimecodeCallback = std::function<void( Result, Timecode, UserEmail, ServiceId )>;
            virtual void                    getTimecode( Timecode timecode, getTimecodeCallback ) = 0;

            using                           createHostCallback = std::function<void( Result )>;
            virtual void                    createHost( 
                                                HostId hostId,
                                                HostKey hostKey,
                                                cpp::Memory hostName,
                                                DeviceIP ip,
                                                createHostCallback ) = 0;

            struct                          Host { HostId hostId; };
            using                           getHostCallback = std::function<void( Result, Host )>;
            virtual void                    getHost( HostId hostId, getHostCallback ) = 0;


            using                           createLoginCallback = std::function<void( Result, Timecode )>;
            virtual void                    createLogin( 
                                                DeviceIP ip,
                                                cpp::Memory email, 
                                                cpp::Memory svcId,
                                                Passcode passcode,
                                                HostId hostId,
                                                cpp::Memory hostName,
                                                cpp::Memory secret, 
                                                createLoginCallback ) = 0;

            using                           updateUserCallback = std::function<void( Result )>;
            virtual void                    updateUser(
                                                UserId userId,
                                                UserEmail email,
                                                cpp::Memory secret ) = 0;

            using                           updateUserRegionCallback = std::function<void( Result )>;
            virtual void                    updateUserRegion(
                                                UserId userId,
                                                cpp::Memory region,
                                                bool isAllowed ) = 0;

            using                           removeUserRegionCallback = std::function<void( Result )>;
            virtual void                    removeUserRegion(
                                                UserId userId,
                                                cpp::Memory region ) = 0;

            using                           updateUserSubnetCallback = std::function<void( Result )>;
            virtual void                    updateUserSubnet(
                                                UserId userId,
                                                cpp::Memory subnet,
                                                bool isAllowed ) = 0;

            using                           removeUserSubnetCallback = std::function<void( Result )>;
            virtual void                    removeUserSubnet(
                                                UserId userId,
                                                cpp::Memory subnet,
                                                bool isAllowed ) = 0;

            using                           updateUserServiceCallback = std::function<void( Result )>;
            virtual void                    updateUserService(
                                                UserId userId,
                                                ServiceId svcId,
                                                bool isAllowed ) = 0;

            using                           removeUserServiceCallback = std::function<void( Result )>;
            virtual void                    removeUserService(
                                                UserId userId,
                                                ServiceId svcId ) = 0;

            using                           createAuthCallback = std::function<void( Result )>;
            virtual void                    createAuth(
                                                HostId hostId,
                                                HostKey hostKey,
                                                cpp::Memory hostName,
                                                DeviceIP ip,
                                                createHostCallback ) = 0;
        };

        struct ISecurity
        {
            enum class                      Result { Ok, Error };

            using                           auditCallback = std::function<void( Result )>;
            virtual void                    audit( DeviceIP, UserEmail, auditCallback ) = 0;
            virtual void                    audit( DeviceIP, Timecode, HostId, auditCallback ) = 0;
            virtual void                    audit( DeviceIP, AuthToken, auditCallback ) = 0;

            virtual void                    recordInit( DeviceIP, UserEmail ) = 0;
            virtual void                    recordId( DeviceIP, UserEmail, HostId, bool isSuccess ) = 0;
            virtual void                    recordConfirm( DeviceIP, AuthToken, bool isSuccess ) = 0;
            virtual void                    recordAuth( DeviceIP, AuthToken ) = 0;
            virtual void                    recordRead( DeviceIP, AuthToken ) = 0;
            virtual void                    recordWrite( DeviceIP, AuthToken ) = 0;
        };

        struct IServer
        {
            IAuth &                         auth;
            IAuthUser &                     authUser;
            IEmailer &                      emailer;
            ISecurity &                     security;
            IAuthDB &                       authDB;
        };
    };
}

namespace grim::auth
{
}