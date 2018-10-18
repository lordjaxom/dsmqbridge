#ifndef DS_MQTT_BRIDGE_DSS_ENDPOINT_HPP
#define DS_MQTT_BRIDGE_DSS_ENDPOINT_HPP

#include <string>

#include <nlohmann/json_fwd.hpp>

namespace dsmq {
namespace dss {

class Endpoint
{
    friend void from_json( nlohmann::json const& src, Endpoint& dst );

public:
    Endpoint();
    Endpoint( std::string host, std::string port, std::string apikey );

    std::string const& host() const { return host_; }
    std::string const& port() const { return port_; }
    std::string const& apikey() const { return apikey_; }

private:
    std::string host_;
    std::string port_;
    std::string apikey_;
};

} // namespace dss
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_DSS_ENDPOINT_HPP
