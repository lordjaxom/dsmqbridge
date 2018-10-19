#ifndef DS_MQTT_BRIDGE_MQTT_CLIENT_HPP
#define DS_MQTT_BRIDGE_MQTT_CLIENT_HPP

#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>

#include "mqtt_types.hpp"

namespace dsmq {
namespace mqtt {

class Client
{
    class Impl;

public:
    Client( boost::asio::io_context& context, Endpoint endpoint );
    ~Client();

    void publish( std::string topic, std::string payload );
    void subscribe( std::string topic, std::function< void ( std::string payload ) > handler );

private:
    std::unique_ptr< Impl > impl_;
};

} // namespace mqtt
} // namespace dsmq

#endif //DS_MQTT_BRIDGE_MQTT_CLIENT_HPP
