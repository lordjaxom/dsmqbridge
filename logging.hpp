#ifndef DS_MQTT_BRIDGE_LOGGING_HPP
#define DS_MQTT_BRIDGE_LOGGING_HPP

#include <cstddef>
#include <mutex>
#include <ostream>
#include <string>
#include <utility>

namespace dsmq {

namespace detail {

struct LogOutputDeleter
{
    void operator()( std::ostream const* p );
};

std::ostream& logTimestamp( std::ostream &os );
std::ostream& logPid( std::ostream &os );

inline void logWrite( std::ostream &os )
{
	os << std::endl;
}

template< typename Arg0, typename ...Args >
void logWrite( std::ostream& os, Arg0&& arg0, Args&&... args )
{
	os << std::forward< Arg0 >( arg0 );
    logWrite( os, std::forward<Args>( args )... );
}

template< typename ...Args >
void logMessage( std::ostream& os, std::string const &tag, char const *level, Args &&... args )
{
    logWrite( os, logTimestamp, " [", logPid, "] [", tag, "] [", level, "] ", std::forward< Args >( args )... );
}

} // namespace detail

class Logger
{
	friend struct detail::LogOutputDeleter;

	using OutputPtr = std::unique_ptr< std::ostream, detail::LogOutputDeleter >;
	using Lock = std::lock_guard< std::recursive_mutex >;

	static constexpr std::size_t tagLength = 15;

public:
	struct Level
	{
        static Level const debug;
        static Level const info;
        static Level const warning;
        static Level const error;

		char const* name;
		unsigned level;
	};

private:
	static bool is( Level const& level );

	static Level const* level_;
	static char const* outputFile_;
	static OutputPtr output_;
	static std::recursive_mutex mutex_;

public:
	static void threshold( Level const& level );
	static void output( std::ostream& output );
	static void output( char const* outputFile );
	static void reopen();

	explicit Logger( char const* tag ) noexcept;
	Logger( Logger const& ) = delete;

	template< typename ...Args >
	void debug( Args&&... args )
	{
		log( Level::debug, std::forward< Args >( args )... );
	}

	template< typename ...Args >
	void info( Args&&... args )
	{
		log( Level::info, std::forward< Args >( args )... );
	}

	template< typename ...Args >
	void warning( Args&&... args )
	{
		log( Level::warning, std::forward< Args >( args )... );
	}

	template< typename ...Args >
	void error( Args&&... args )
	{
		log( Level::error, std::forward< Args >( args )... );
	}

private:
	template< typename ...Args >
	void log( Level const& level, Args&&... args )
	{
		if ( is( level )) {
            Lock lock( mutex_ );
            initialize();
			detail::logMessage( *output_, tag_, level.name, std::forward< Args >( args )... );
		}
	}

	void initialize();

	char const* rawTag_;
	std::string tag_;
};

} // namespace dsmq

#endif // DS_MQTT_BRIDGE_LOGGING_HPP
