#include <fstream>
#include <iostream>
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

void run( int argc, char* const argv[] )
{
    try {
        Logger::threshold( Logger::Level::debug );

        CommandLine args { argv, argc };
        if ( !args.logFile().empty()) {
            Logger::output( args.logFile().c_str());
        }

        logger.info( "dsmqbridge starting" );

        Manager manager { readProperties( args.propertiesFile()) };
        manager.run();
    } catch ( CommandLineError const& e ) {
        std::cerr << e.what();
    } catch ( exception const& e ) {
        logger.error( e.what() );
    }
}

} // namespace dsmq

int main( int argc, char* const argv[] )
{
    dsmq::run( argc, argv );
}
