cmake_minimum_required(VERSION 3.10)
project(dS_MQTT_Bridge)

cmake_host_system_information(RESULT HOSTNAME QUERY HOSTNAME)
include("CMakeLocal.${HOSTNAME}.cmake")

set(CMAKE_CXX_STANDARD 14)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj -m64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")
endif()

find_package(Boost 1.66.0 COMPONENTS system coroutine REQUIRED)
set(Boost_DEFINITIONS BOOST_COROUTINES_NO_DEPRECATION_WARNING)

find_path(json_INCLUDE_DIRS nlohmann/json.hpp HINTS "${JSON_ROOT}/include")

find_path(openssl_INCLUDE_DIRS openssl/ssl.h HINTS "${OPENSSL_ROOT}/include")
find_library(openssl_ssl_LIBRARY ssl HINTS "${OPENSSL_ROOT}/lib")
find_library(openssl_crypto_LIBRARY crypto HINTS "${OPENSSL_ROOT}/lib")
set(openssl_LIBRARIES "${openssl_ssl_LIBRARY}" "${openssl_crypto_LIBRARY}")
message("Libs: ${openssl_LIBRARIES}")

add_executable(dS_MQTT_Bridge
        logging.cpp
        logging.hpp
        main.cpp
        dss_client.cpp
        dss_client.hpp
        error.cpp
        error.hpp
        dss_service.cpp
        dss_service.hpp
        dss_endpoint.cpp
        dss_endpoint.hpp
        commandline.cpp
        commandline.hpp)
target_compile_definitions(dS_MQTT_Bridge PUBLIC ${Boost_DEFINITIONS})
target_include_directories(dS_MQTT_Bridge PUBLIC ${Boost_INCLUDE_DIRS} ${json_INCLUDE_DIRS} ${utf8_INCLUDE_DIRS} ${openssl_INCLUDE_DIRS})
target_link_libraries(dS_MQTT_Bridge ${openssl_LIBRARIES} ${Boost_LIBRARIES})
if(WIN32)
    target_link_libraries(dS_MQTT_Bridge ws2_32)
else()
    target_link_libraries(dS_MQTT_Bridge pthread)
endif()
