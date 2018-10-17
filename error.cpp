#include "error.hpp"

using namespace std;

namespace dsmq {

/**
 * function dsmq_category
 */

namespace detail {

string dsmq_category::message( int value ) const
{
    switch ( static_cast< dsmq_errc >( value ) ) {
        case dsmq_errc::not_ok: return "server returned nok";
        case dsmq_errc::exception: return "exception caught";
        case dsmq_errc::server_error: return "server side error";
        case dsmq_errc::protocol_violation: return "protocol violation";
        case dsmq_errc::timeout: return "timeout";
    }
    return "unknown dsmq::dsmq_category error";
}

} // namespace detail

error_category const& dsmq_category()
{
    static detail::dsmq_category instance;
    return instance;
}


/**
 * function make_error_code
 */

error_code make_error_code( dsmq_errc e )
{
    return { static_cast< int >( e ), dsmq_category() };
}

} // namespace dsmq
