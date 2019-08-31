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

static Logger logger( "logging" );

namespace detail {

void LogOutputDeleter::operator()( std::ostream const* p )
{
    if ( Logger::outputFile_ ) {
        delete p;
    }
}

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
string logBuildTag( char const* rawTag )
{
	string tag = rawTag;
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
char const* Logger::outputFile_;
Logger::OutputPtr Logger::output_;
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
	output_.reset( &output );
	outputFile_ = nullptr;
}

void Logger::output( char const* outputFile )
{
	output_.reset( new ofstream( outputFile, ios::out | ios::app ));
	outputFile_ = outputFile;
}

void Logger::reopen()
{
	if ( !outputFile_ ) {
		return;
	}

	output( outputFile_ );

	logger.info( "reopening logfile" );
}

Logger::Logger( char const* tag ) noexcept
	: rawTag_( tag )
{
}

void Logger::initialize()
{
	if ( !output_ ) {
		output( cerr );
	}
	if ( tag_.empty()) {
		tag_ = detail::logBuildTag< tagLength >( rawTag_ );
	}
}

} // namespace dsmq
