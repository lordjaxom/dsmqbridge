#ifndef DS_MQTT_BRIDGE_MANAGER_HPP
#define DS_MQTT_BRIDGE_MANAGER_HPP

#include <memory>

#include <nlohmann/json_fwd.hpp>

namespace dsmq {

class Manager
{
    class Impl;

public:
    explicit Manager( nlohmann::json const& props );
    ~Manager();

    void run();

private:
    std::unique_ptr< Impl > impl_;
};

} // namespace dsmq

#endif //DS_MQTT_BRIDGE_MANAGER_HPP
