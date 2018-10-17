#include <utility>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

#include "client_dss.hpp"
#include "error.hpp"
#include "logging.hpp"

using namespace std;
using namespace nlohmann;

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

using tcp = asio::ip::tcp;

namespace dsmq {

static Logger logger( "client_dss" );

class DSSEventClient::Impl
{
public:
    explicit Impl( asio::io_context& context )
            : context_ { context } {}

    void start( string&& host, string&& port, string&& apikey )
    {
        asio::spawn( context_, [this, host = move( host ), port = move( port ), apikey = move( apikey )]( auto yield ) {
            try {
                ssl::context sslContext { ssl::context::sslv23_client };

                logger.info( "connecting to dSS at ", host, ":", port );

                tcp::resolver resolver { context_ };
                auto resolved{ resolver.async_resolve( host, port, yield ) };

                ssl::stream<tcp::socket> stream { context_, sslContext };
                asio::async_connect( stream.next_layer(), resolved, yield );
                stream.set_verify_mode( ssl::verify_none );
                stream.async_handshake( ssl::stream_base::client, yield );

                logger.debug( "logging in with application token" );

                http::request<http::empty_body> request{ http::verb::get, "/json/system/loginApplication?loginToken=" + apikey, 11 };
                request.set( http::field::host, host );
                request.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
                http::async_write( stream, request, yield );

                boost::beast::multi_buffer buffer;
                http::response<http::dynamic_body> response;
                http::async_read( stream, buffer, response, yield );
                if ( response.result() != http::status::ok ) {
                    throw system_error( make_error_code( dsmq_errc::server_error ) );
                }

                auto message = json::parse( boost::beast::buffers_to_string( response.body().data() ) );
                auto token = message["result"]["token"];

                logger.debug( "received session token ", token );

                // event: callScene

            } catch ( system_error const& e ) {
                logger.error( "system_error: ", e.what() );
            } catch ( boost::beast::system_error const& e ) {
                logger.error( "beast error: ", e.what() );
            }
        } );
    }

private:
    asio::io_context& context_;
};

DSSEventClient::DSSEventClient( asio::io_context& context )
        : impl_ { make_unique< Impl > ( context ) } {}

void DSSEventClient::start( string host, string port, string apikey )
{
    impl_->start( move( host ), move( port ), move( apikey ) );
}

DSSEventClient::~DSSEventClient() = default;

} // namespace dsmq