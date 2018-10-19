#include <utility>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <nlohmann/json.hpp>

#include "dss_client.hpp"
#include "manager.hpp"
#include "mqtt_client.hpp"

using namespace std;
using namespace nlohmann;

namespace asio = boost::asio;
namespace ssl = asio::ssl;

namespace dsmq {

class Manager::Impl
{
public:
    explicit Impl( json const& props )
            : mqtt_ { context_, props.at( "MQTT" ) }
            , dss_ { context_, sslContext_, props.at( "dSS" ) }
    {
        dss_.subscribe< dss::EventCallScene >( [this]( auto event ) { this->on_callScene( move( event ) ); } );
    }

    void run()
    {
        context_.run();
    }

private:
    void on_callScene( dss::EventCallScene&& event )
    {
        
    }

    asio::io_context context_;
    ssl::context sslContext_ { ssl::context::sslv23_client };
    mqtt::Client mqtt_;
    dss::Client dss_;
};

Manager::Manager( json const& props )
        : impl_ { make_unique< Impl >( props ) } {}

Manager::~Manager() = default;

void Manager::run()
{
    impl_->run();
}

} // namespace dsmq