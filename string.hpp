#ifndef DS_MQTT_BRIDGE_STRING_HPP
#define DS_MQTT_BRIDGE_STRING_HPP

#include <sstream>
#include <string>

namespace dsmq {

namespace detail {

inline void str( std::ostream& os ) {}

template< typename Arg0, typename ...Args >
void str( std::ostream& os, Arg0&& arg0, Args&&... args )
{
    os << std::forward< Arg0 >( arg0 );
    str( os, std::forward< Args >( args )... );
}

} // namespace detail

template< typename ...Args >
std::string str( Args&&... args )
{
    std::ostringstream os;
    detail::str( os, std::forward< Args >( args )... );
    return os.str();
}

} // namespace dsmq

#endif //DS_MQTT_BRIDGE_STRING_HPP
