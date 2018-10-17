#ifndef DS_MQTT_BRIDGE_ERROR_HPP
#define DS_MQTT_BRIDGE_ERROR_HPP

#include <system_error>
#include <type_traits>

namespace dsmq {

/**
 * enum class dsmq_errc
 */

enum class dsmq_errc : int
{
    not_ok,
    exception,
    server_error,
    protocol_violation,
    timeout
};


/**
 * function dsmq_category
 */

namespace detail {

class dsmq_category
        : public std::error_category
{
public:
    char const* name() const noexcept override { return "prnet::prnet_category"; }

    std::string message( int value ) const override;
};

} // namespace detail

std::error_category const& dsmq_category();


/**
 * function make_error_code
 */

std::error_code make_error_code( dsmq_errc e );

} // namespace prnet


/**
 * namespace std hooks
 */

namespace std {

template<>
struct is_error_code_enum< dsmq::dsmq_errc >
        : public std::true_type {};

} // namespace std

#endif // DS_MQTT_BRIDGE_ERROR_HPP
