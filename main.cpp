#include <fstream>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <nlohmann/json.hpp>

#include "commandline.hpp"
#include "dss_client.hpp"
#include "logging.hpp"

using namespace std;
using namespace nlohmann;

namespace asio = boost::asio;
namespace ssl = asio::ssl;

namespace dsmq {

static Logger logger( "main" );

json readProperties( string const& fileName )
{
    ifstream ifs { fileName, ios::in };
    if ( !ifs ) {
        throw system_error( errno, system_category(), "couldn't open " + fileName );
    }

    json props;
    ifs >> props;
    return move( props );
}

void run( int argc, char* const argv[] )
{
    try {
        Logger::threshold( Logger::Level::debug );

        CommandLine args { argv, argc };
        if ( !args.logFile().empty() ) {
            Logger::output( args.logFile().c_str() );
        }

        logger.info( "dS_MQTT_Bridge starting" );

        auto props = readProperties( args.propertiesFile() );

        asio::io_context context;
        ssl::context sslContext { ssl::context::sslv23_client };

        dss::Client dssClient { context, sslContext, props.at( "dSS" ) };
        dssClient.subscribe( "callScene" );
        dssClient.eventLoop();

        context.run();
    } catch ( exception const& e ) {
        logger.error( e.what() );
    }
}

} // namespace dsmq

int main( int argc, char* const argv[] )
{
    dsmq::run( argc, argv );
}
