module;

#include <cstdint>
#include <string>
#include <format>
#include <vector>
#include <functional>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <external\cpp-httplib\httplib.h>

export module grim.proto.auth_client;
import cpp.memory;
import cpp.random;
import cpp.file;
import cpp.windows;
import cpp.bit.file;
import grim.proto.auth;

export namespace grim::auth
{
    uint64_t                                login(
                                                const cpp::Memory & email,
                                                const cpp::Memory & serviceId );
    bool                                    auth(
                                                uint64_t serviceToken,
                                                const cpp::Memory & userIp,
                                                uint64_t userToken,
                                                uint64_t * userId,
                                                std::string * email );
}

namespace grim::auth
{
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
                cpp::Encoder::intToHex( rng.rand( ) ) +
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

    void auth( DeviceIP authorizingIp, AuthToken authorizingToken, AuthToken requesterToken, ServiceId svc )
    {

    }
}