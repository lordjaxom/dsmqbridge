#ifndef DS_MQTT_BRIDGE_CLIENT_DSS_HPP
#define DS_MQTT_BRIDGE_CLIENT_DSS_HPP

#include <memory>
#include <string>

namespace boost {
namespace asio {
class io_context;
} // namespace asio
} // namespace boost

namespace dsmq {

class DSSEventClient
{
private:
    class Impl;

public:
    explicit DSSEventClient( boost::asio::io_context& context );
    ~DSSEventClient();

    void start( std::string host, std::string port, std::string apikey );

private:
    std::unique_ptr< Impl > impl_;
};

} // namespace dsmq

#endif //DS_MQTT_BRIDGE_CLIENT_DSS_HPP
