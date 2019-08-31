#include <iostream>
#include <stdexcept>

#include <getopt.h>

#include "commandline.hpp"
#include "string.hpp"

using namespace std;

namespace dsmq {

static char const* cmdLineMapShortToLong( int shortopt )
{
	switch ( shortopt ) {
		case 'h': return "help";
		case 'c': return "config-file";
		case 'l': return "log-file";
		default: throw invalid_argument( "cmdLineMapShortToLong( " + to_string( shortopt ) + ")" );
	}
}

CommandLine::CommandLine( char* const *argv, int argc )
	: propertiesFile_ { "dsmqbridge.json" }
{
	struct option options[] = {
		{ nullptr, no_argument,       nullptr, 'h' },
		{ nullptr, required_argument, nullptr, 'c' },
		{ nullptr, required_argument, nullptr, 'l' },
		{}
	};

	for ( auto& option : options ) {
		if ( option.val != 0 ) {
			option.name = cmdLineMapShortToLong( option.val );
		}
	}

	opterr = 0;

	int optchar;
	int optind;
	while ( ( optchar = getopt_long( argc, argv, ":hc:l:", options, &optind ) ) != -1 ) {
		switch ( optchar ) {
			case ':':
				throw CommandLineError( str( "missing argument to --", cmdLineMapShortToLong( optopt ) ) );

			case '?':
				throw CommandLineError( str( "unknown option -", static_cast< char >( optopt ) ) );

			case 'h':
				throw CommandLineError( str( "Usage: ", argv[ 0 ], " [OPTION]...\n",
						"  -c, --config-file=FILE      load configuration from FILE\n",
						"  -l, --log-file=FILE         write logs to FILE instead of standard error\n",
						"  -h, --help                  show this help and exit" ));

			case 'c':
				propertiesFile_ = optarg;
				break;

			case 'l':
				logFile_ = optarg;
				break;

			default:
				throw invalid_argument( string { "getopt_long( ... ) -> '" } + static_cast< char >( optchar ) + "'" );
		}
	}
}

} // namespace dsmq
