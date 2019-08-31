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
		case 'p': return "pid-file";
		case 'd': return "daemonize";
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
		{ nullptr, required_argument, nullptr, 'p' },
		{ nullptr, no_argument,       nullptr, 'd' },
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
	while ( ( optchar = getopt_long( argc, argv, ":hc:l:p:d", options, &optind ) ) != -1 ) {
		switch ( optchar ) {
			case ':':
				throw CommandLineError( str( "missing argument to --", cmdLineMapShortToLong( optopt ) ) );

			case '?':
				throw CommandLineError( str( "unknown option -", static_cast< char >( optopt ) ) );

			case 'h':
				throw CommandLineError( str( "Usage: ", argv[ 0 ], "[...]" ) );

			case 'c':
				propertiesFile_ = optarg;
				break;

			case 'l':
				logFile_ = optarg;
				break;

			case 'p':
				pidFile_ = optarg;
				break;

			case 'd':
				daemon_ = true;
				break;

			default:
				throw invalid_argument( string { "getopt_long( ... ) -> '" } + static_cast< char >( optchar ) + "'" );
		}
	}
}

} // namespace dsmq
