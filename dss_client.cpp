#include <map>
#include <sstream>
#include <utility>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/version.hpp>
#include <boost/optional.hpp>
#include <boost/utility/string_view.hpp>
#include <nlohmann/json.hpp>

#include "dss_client.hpp"
#include "error.hpp"
#include "logging.hpp"
#include "string.hpp"

using namespace std;
using namespace nlohmann;

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

using tcp = asio::ip::tcp;

namespace dsmq {
namespace dss {

static Logger logger( "client_dss" );

class Client::Impl
{
public:
    Impl( asio::io_context& context, ssl::context& sslContext, Endpoint&& endpoint )
            : context_ { context }
            , sslContext_ { sslContext }
            , endpoint_( move( endpoint ) ) {}

    void subscribe( char const* name, function< void ( json const& event ) >&& handler )
    {
        eventHandlers_.emplace( name, move( handler ) );
    }

    void eventLoop()
    {
        asio::spawn( [this]( auto yield ) {
            try {
                this->eventLoop( yield );
                logger.debug( "exiting spawn for eventloop" );
            } catch ( system_error const& e ) {
                logger.error( endpoint_, "system_error in event loop: ", e.what() );
            } catch ( boost::beast::system_error const& e ) {
                logger.error( endpoint_, "beast::system_error in event loop: ", e.what() );
            }
        } );
    }

    void callScene( unsigned zone, unsigned group, unsigned scene )
    {
        asio::spawn( [this, zone, group, scene]( auto yield ) {
            try {
                this->request( "zone/callScene", str( "id=", zone, "&groupID=", group, "&sceneNumber=", scene ), true, yield );
            } catch ( system_error const& e ) {
                logger.error( endpoint_, "system_error in callScene: ", e.what() );
            } catch ( boost::beast::system_error const& e ) {
                logger.error( endpoint_, "beast::system_error in callScene: ", e.what() );
            }
        } );
    }

private:
    string path( string const& op, string const& query )
    {
        return str( "/json/", op, "?", query, token_ ? "&token=" : "", token_ ? *token_ : "" );
    }

    json request( string const& op, string const& query, bool needsToken, asio::yield_context yield )
    {
        if ( needsToken && !token_ ) {
            token_ = request( "system/loginApplication", str( "loginToken=", endpoint_.apikey() ), false, yield )
                    .at( "token" ).get< string >();
        }

        logger.debug( endpoint_, "sending request ", op );

        tcp::resolver resolver { context_ };
        auto resolved { resolver.async_resolve( endpoint_.host(), endpoint_.port(), yield ) };

        ssl::stream< tcp::socket > stream { context_, sslContext_ };
        asio::async_connect( stream.next_layer(), resolved, yield );
        stream.set_verify_mode( ssl::verify_none );
        stream.async_handshake( ssl::stream_base::client, yield );

        http::request< http::empty_body > request { http::verb::get, path( op, query ), 11 };
        request.set( http::field::host, endpoint_.host() );
        request.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
        http::async_write( stream, request, yield );

        boost::beast::multi_buffer buffer;
        http::response< http::dynamic_body > response;
        http::async_read( stream, buffer, response, yield );
        if ( response.result() != http::status::ok ) {
            throw system_error( make_error_code( dsmq_errc::server_error ));
        }

        logger.debug( endpoint_, "received response for ", op, ": ", boost::beast::buffers( response.body().data()));

        auto message = json::parse( boost::beast::buffers_to_string( response.body().data()));
        if ( !message.at( "ok" )) {
            throw system_error( make_error_code( dsmq_errc::not_ok ), message.at( "message" ));
        }
        return message.count( "result" ) > 0 ? move( message.at( "result" ) ) : json( true );
    }

    void processEvents( json const& events )
    {
        for ( auto const& event : events ) {
            auto range = eventHandlers_.equal_range( event.at( "name" ).get< string >() );
            for_each( range.first, range.second, [&event]( auto const& eventHandler ) { eventHandler.second( event ); } );
        }
    }

    void eventLoop( asio::yield_context yield )
    {
        assert( !eventLoop_ );

        eventLoop_ = true;
        for ( auto const& eventHandler : eventHandlers_ ) {
            request( "event/subscribe", str( "subscriptionID=1&name=", eventHandler.first ), true, yield ); // FIXME
        }

        asio::steady_timer timeout { context_ };
        timeout.async_wait( []( error_code ec ) {
            if ( ec == make_error_code( asio::error::operation_aborted )) {
                return;
            }
            logger.error( "timeout waiting for events" );
        } );
        while ( eventLoop_ ) {
            logger.debug( "start of eventloop" );
            timeout.expires_after( chrono::seconds( 31 ));
            processEvents( request( "event/get", "subscriptionID=1&timeout=30000", true, yield ).at( "events" ));
            logger.debug( "end of eventloop" );
        }
        logger.debug( "left eventloop" );
    }

    asio::io_context& context_;
    ssl::context& sslContext_;
    Endpoint endpoint_;
    boost::optional< string > token_;
    multimap< boost::string_view, function< void ( json const& event ) > > eventHandlers_;
    bool eventLoop_ {};
};

Client::Client( asio::io_context& context, ssl::context& sslContext, Endpoint endpoint )
        : impl_ { make_unique< Impl >( context, sslContext, move( endpoint ) ) } {}

Client::~Client() = default;

void Client::subscribe( char const* name, std::function< void( nlohmann::json const& event ) >&& handler )
{
    impl_->subscribe( name, move( handler ) );
}

void Client::eventLoop()
{
    impl_->eventLoop();
}

void Client::callScene( unsigned zone, unsigned group, unsigned scene )
{
    impl_->callScene( zone, group, scene );
}

} // namespace dss
} // namespace dsmq