#include <boost/asio/io_context.hpp>

#include "client_dss.hpp"
#include "logging.hpp"

namespace asio = boost::asio;

int main()
{
    dsmq::Logger::threshold( dsmq::Logger::Level::debug );

    asio::io_context context;
    dsmq::DSSEventClient dssEventClient { context };
    dssEventClient.start( "192.168.178.29", "8080", "7b1c4f70d6c5113c3753f6a67d4a228cf65c64ace02af0f9e93aa6e58dbc5438" );
    context.run();
}