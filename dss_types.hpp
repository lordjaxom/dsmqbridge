#ifndef DS_MQTT_BRIDGE_DSS_TYPES_HPP
#define DS_MQTT_BRIDGE_DSS_TYPES_HPP

#include <iosfwd>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace dsmq {
namespace dss {

class Endpoint
{
    friend void from_json( nlohmann::json const& src, Endpoint& dst );

    friend std::ostream& operator<<( std::ostream& os, Endpoint const& val );

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

class EventCallScene
{
    friend void from_json( nlohmann::json const& src, EventCallScene& dst );

public:
    static constexpr char const* name = "callSceneBus";

    unsigned zone() const { return zone_; }
    unsigned group() const { return group_; }
    unsigned scene() const { return scene_; }

private:
    unsigned zone_ {};
    unsigned group_ {};
    unsigned scene_ {};
};

} // namespace dss
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_DSS_TYPES_HPP
