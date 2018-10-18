#ifndef DS_MQTT_BRIDGE_DSS_CLIENT_HPP
#define DS_MQTT_BRIDGE_DSS_CLIENT_HPP

#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/spawn.hpp>
#include <nlohmann/json_fwd.hpp>

#include "dss_endpoint.hpp"

namespace dsmq {
namespace dss {

class Client
{
    class Impl;

public:
    Client( boost::asio::io_context& context, boost::asio::ssl::context& sslContext, Endpoint endpoint );
    ~Client();

    nlohmann::json request( std::string const& op, std::string const& query, boost::asio::yield_context yield );

private:
    std::unique_ptr< Impl > impl_;
};

} // namespace dss
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_DSS_CLIENT_HPP
