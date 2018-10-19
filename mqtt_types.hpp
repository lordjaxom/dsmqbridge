#ifndef DS_MQTT_BRIDGE_MQTT_TYPES_HPP
#define DS_MQTT_BRIDGE_MQTT_TYPES_HPP

#include <iosfwd>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace dsmq {
namespace mqtt {

class Endpoint
{
    friend void from_json( nlohmann::json const& src, Endpoint& dst );

    friend std::ostream& operator<<( std::ostream& os, Endpoint const& val );

public:
    std::string const& host() const { return host_; }
    int port() const { return port_; }
    std::string const& clientId() const { return clientId_; }

private:
    std::string host_;
    int port_ {};
    std::string clientId_;
};

} // namespace mqtt
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_MQTT_TYPES_HPP
