#include <ostream>

#include <nlohmann/json.hpp>

#include "mqtt_types.hpp"

using namespace std;

namespace dsmq {
namespace mqtt {

void from_json( nlohmann::json const& src, Endpoint& dst )
{
    dst.host_ = src.at( "host" );
    dst.port_ = src.at( "port" );
    dst.clientId_ = src.at( "clientId" );
}

ostream& operator<<( ostream& os, Endpoint const& val )
{
    return os << "[MQTT@" << val.host() << ":" << val.port() << "] ";
}

} // namespace mqtt
} // namespace dsmq