#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include "commandline.hpp"
#include "logging.hpp"
#include "manager.hpp"

using namespace std;
using namespace nlohmann;

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
    return props;
}

map< int, string > buildZonesDss2Mqtt( json& zones )
{
    map< int, string > result;
    for ( auto it = zones.begin() ; it != zones.end() ; ++it ) {
        result.emplace( it.value(), it.key() );
    }
    return result;
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

        Manager manager { readProperties( args.propertiesFile() ) };
        manager.run();

        auto zonesDss2Mqtt = buildZonesDss2Mqtt( props.at( "zones" ) );

        dssClient.subscribe< dss::EventCallScene >( [&]( auto event ) {
            logger.debug( "received callScene from zone ", event.zone(), ", group ", event.group(), ", scene ", event.scene() );

            auto zone = zonesDss2Mqtt.find( event.zone() );
            if ( zone == zonesDss2Mqtt.end() ) {
                return;
            }

            logger.debug( "mapped zone to ", zone->second );
        } );
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
