#ifndef DS_MQTT_BRIDGE_COMMANDLINE_HPP
#define DS_MQTT_BRIDGE_COMMANDLINE_HPP

#include <string>

namespace dsmq {

class CommandLine
{
public:
    CommandLine( char* const* argv, int argc );

    std::string const& propertiesFile() const { return propertiesFile_; }
    std::string const& logFile() const { return logFile_; }
    std::string const& pidFile() const { return pidFile_; }
    bool daemon() const { return daemon_; }

private:
    std::string propertiesFile_;
    std::string logFile_;
    std::string pidFile_;
    bool daemon_ {};
};

} // namespace dsmq

#endif // DS_MQTT_BRIDGE_COMMANDLINE_HPP
