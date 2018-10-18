#include <utility>

#include <nlohmann/json.hpp>

#include "dss_endpoint.hpp"

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

} // namespace dss
} // namespace dsmq