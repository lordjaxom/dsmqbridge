#include <ostream>
#include <utility>

#include <nlohmann/json.hpp>

#include "dss_types.hpp"

using namespace std;
using namespace nlohmann;

namespace dsmq {
namespace dss {

Endpoint::Endpoint() = default;

Endpoint::Endpoint( string host, string port, string apikey )
        : host_ { move( host ) }
        , port_ { move( port ) }
        , apikey_ { move( apikey ) }
{}

void from_json( json const& src, Endpoint& dst )
{
    dst.host_ = src.at( "host" );
    dst.port_ = src.at( "port" );
    dst.apikey_ = src.at( "apikey" );
}

ostream& operator<<( ostream& os, Endpoint const& val )
{
    return os << "[dSS@" << val.host() << ":" << val.port() << "] ";
}

void from_json( json const& src, EventCallScene& dst )
{
    auto const& properties = src.at( "properties" );
    dst.zone_ = stoul( properties.at( "zoneID" ).get< string >());
    dst.group_ = stoul( properties.at( "groupID" ).get< string >());
    dst.scene_ = stoul( properties.at( "sceneID" ).get< string >());
}

} // namespace dss
} // namespace dsmq