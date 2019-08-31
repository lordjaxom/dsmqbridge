#ifndef DS_MQTT_BRIDGE_COMMANDLINE_HPP
#define DS_MQTT_BRIDGE_COMMANDLINE_HPP

#include <stdexcept>
#include <string>

namespace dsmq {

class CommandLineError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class CommandLine
{
public:
    CommandLine( char* const* argv, int argc );

    std::string const& propertiesFile() const { return propertiesFile_; }
    std::string const& logFile() const { return logFile_; }

private:
    std::string propertiesFile_;
    std::string logFile_;
};

} // namespace dsmq

#endif // DS_MQTT_BRIDGE_COMMANDLINE_HPP
