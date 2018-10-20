#ifndef DS_MQTT_BRIDGE_DSS_CLIENT_HPP
#define DS_MQTT_BRIDGE_DSS_CLIENT_HPP

#include <functional>
#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/utility/string_view_fwd.hpp>
#include <nlohmann/json_fwd.hpp>

#include "dss_types.hpp"

namespace dsmq {
namespace dss {

class Client
{
    class Impl;

public:
    Client( boost::asio::io_context& context, boost::asio::ssl::context& sslContext, Endpoint endpoint );
    ~Client();

    template< typename Event >
    void subscribe( std::function< void ( Event event ) > handler )
    {
        subscribe( Event::name, move( handler ) );
    }

    void eventLoop();

    void callScene( unsigned zone, unsigned group, unsigned scene );

private:
    void subscribe( char const* name, std::function< void ( nlohmann::json const& event ) >&& handler );

    std::unique_ptr< Impl > impl_;
};

} // namespace dss
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_DSS_CLIENT_HPP
