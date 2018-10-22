#include <cstring>
#include <ctime>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#if !defined( WIN32 )
#   include <sys/types.h>
#   include <unistd.h>
#endif

#include "logging.hpp"

using namespace std;

namespace dsmq {

namespace detail {

ostream& logTimestamp( ostream &os )
{
	auto timestamp { chrono::high_resolution_clock::now().time_since_epoch() };
	auto seconds { chrono::duration_cast< chrono::seconds >( timestamp ) };
	auto micros { chrono::duration_cast< chrono::microseconds >( timestamp - seconds ) };
	time_t tt { seconds.count() };
	tm* tm { localtime( &tt ) };

	return os
			<< setw( 4 ) << setfill( '0' ) << ( tm->tm_year + 1900 ) << "/"
			<< setw( 2 ) << setfill( '0' ) << ( tm->tm_mon + 1 ) << "/"
			<< setw( 2 ) << setfill( '0' ) << tm->tm_mday << " "
			<< setw( 2 ) << setfill( '0' ) << tm->tm_hour << ":"
			<< setw( 2 ) << setfill( '0' ) << tm->tm_min << ":"
			<< setw( 2 ) << setfill( '0' ) << tm->tm_sec << "."
			<< setw( 6 ) << setfill( '0' ) << micros.count();
}

ostream& logPid( ostream& os )
{
	return os << setw( 5 ) << setfill( ' ' ) << getpid();
}

template< size_t L >
string buildTag( string&& tag )
{
	if ( tag.length() == L ) {
		return move( tag );
	}

	string result;
	result.reserve( L );
	if ( tag.length() < L ) {
		result.append( ( L - tag.length() ) / 2, ' ' );
		result.append( tag );
		result.append( ( L - tag.length() ) - ( L - tag.length() ) / 2, ' ' );
	}
	else {
		result.append( "..." );
		result.append( tag.substr( tag.length() - L + 3, L - 3 ) );
	}
	return result;
}

} // namespace detail

Logger::Level const Logger::Level::debug   { "DEBUG", 3 };
Logger::Level const Logger::Level::info    { "INFO ", 2 };
Logger::Level const Logger::Level::warning { "WARN ", 1 };
Logger::Level const Logger::Level::error   { "ERROR", 0 };

Logger::Level const* Logger::level_ = &Logger::Level::warning;
shared_ptr< ostream > Logger::output_( &cerr, []( ostream const* ) {} );
recursive_mutex Logger::mutex_;

bool Logger::is( Level const& level )
{
	return level_->level >= level.level;
}

void Logger::threshold( Level const& level )
{
	level_ = &level;
}

void Logger::output( ostream& output )
{
	output_.reset( &output, []( ostream const* ) {} );
}

void Logger::output( char const* output )
{
	output_.reset( new ofstream( output, ios::out | ios::app ) );
}

Logger::Logger( string tag ) noexcept
	: tag_( detail::buildTag< tagLength >( move( tag ) ) )
{
}

} // namespace dsmq
