#include <utility>

#include <boost/asio/io_context.hpp>

#include "dss_client.hpp"
#include "dss_service.hpp"

using namespace std;

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;

namespace dsmq {
namespace dss {

class Service::Impl
{
public:
    Impl(  boost::asio::io_context& context, boost::asio::ssl::context& sslContext, Endpoint&& endpoint )
            : context_ { context }
            , client_ { context, sslContext, move( endpoint ) } {}

private:
    asio::io_context& context_;
    Client client_;
};

Service::Service( boost::asio::io_context& context, boost::asio::ssl::context& sslContext, Endpoint endpoint )
        : impl_ { make_unique< Impl >( context, sslContext, move( endpoint ) ) } {}

Service::~Service() = default;

} // namespace dss
} // namespace dsmq