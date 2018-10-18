#ifndef DS_MQTT_BRIDGE_DSS_SERVICE_HPP
#define DS_MQTT_BRIDGE_DSS_SERVICE_HPP

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include "dss_endpoint.hpp"

namespace boost { namespace asio { class io_context; } }
namespace boost { namespace asio { namespace ssl { class context; } } }

namespace dsmq {
namespace dss {

class Service
{
    class Impl;

public:
    Service( boost::asio::io_context& context, boost::asio::ssl::context& sslContext, Endpoint endpoint );
    ~Service();

private:
    std::unique_ptr< Impl > impl_;
};

} // namespace dss
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_DSS_SERVICE_HPP
