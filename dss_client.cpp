#include <map>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
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
#include <nlohmann/json.hpp>

#include "dss_client.hpp"
#include "error.hpp"
#include "logging.hpp"

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
    explicit Impl( asio::io_context& context, ssl::context& sslContext, Endpoint&& endpoint )
            : context_ { context }
            , sslContext_ { sslContext }
            , endpoint_( move( endpoint ) ) {}

    void subscribe( string&& event )
    {
        events_.push_back( move( event ) );
    }

    void eventLoop()
    {
        asio::spawn( [this]( auto yield ) {
            for ( auto const &event : events_ ) {
                this->request( "event/subscribe", "subscriptionID=1&name=" + event, true, yield );
            }

            for (;;) {
                this->request( "event/get", "subscriptionID=1", true, yield );
            }
        } );
    }

private:
    string path( string const& op, string const& query )
    {
        ostringstream os;
        os << "/json/" << op << '?' << query;
        if ( token_ ) {
            os << "&token=" << *token_;
        }
        return os.str();
    }

    json request( string const& op, string const& query, bool needsToken, asio::yield_context yield )
    {
        if ( needsToken && !token_ ) {
            token_ = request( "system/loginApplication", "loginToken=" + endpoint_.apikey(), false, yield )
                    .at( "token" ).get< string >();
        }

        logger.debug( "sending request ", op, " to dSS at ", endpoint_.host(), ":", endpoint_.port() );

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

        logger.debug( "received response for ", op, ": ", boost::beast::buffers( response.body().data()));

        auto message = json::parse( boost::beast::buffers_to_string( response.body().data()));
        if ( !message.at( "ok" )) {
            throw system_error( make_error_code( dsmq_errc::not_ok ), message.at( "message" ));
        }
        return message.count( "result" ) > 0 ? move( message.at( "result" ) ) : json( true );
    }

    asio::io_context& context_;
    ssl::context& sslContext_;
    Endpoint endpoint_;
    boost::optional< string > token_;
    vector< string > events_;
};

Client::Client( asio::io_context& context, ssl::context& sslContext, Endpoint endpoint )
        : impl_ { make_unique< Impl >( context, sslContext, move( endpoint ) ) }
{}

Client::~Client() = default;

void Client::subscribe( string event )
{
    impl_->subscribe( move( event ) );
}

void Client::eventLoop()
{
    impl_->eventLoop();
}

} // namespace dss
} // namespace dsmq