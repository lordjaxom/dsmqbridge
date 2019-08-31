#include <cmath>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <mosquitto.h>

#include "logging.hpp"
#include "mqtt_client.hpp"
#include "string.hpp"

using namespace std;

namespace asio = boost::asio;

namespace dsmq {
namespace mqtt {

static Logger logger( "mqtt_client" );

class Client::Impl
{
    using Lock = unique_lock< mutex >;

public:
    Impl( asio::io_context& context, Endpoint&& endpoint )
            : context_ { context }
            , endpoint_ { move( endpoint ) }
    {
        call_once( initialized, [] { mosquitto_lib_init(); } );

        mosq_ = mosquitto_new( endpoint_.clientId().c_str(), false, this );
        mosquitto_connect_callback_set(
                mosq_, []( mosquitto*, void* obj, int rc ) { static_cast< Impl* >( obj )->on_connect( rc ); } );
        mosquitto_disconnect_callback_set(
                mosq_, []( mosquitto*, void* obj, int rc ) { static_cast< Impl* >( obj )->on_disconnect( rc ); } );
        mosquitto_message_callback_set(
                mosq_, []( mosquitto*, void* obj, mosquitto_message const* msg ) { static_cast< Impl* >( obj )->on_message( *msg ); } );

        if ( int rc = mosquitto_loop_start( mosq_ ) ) {
            throw runtime_error( str( "couldn't start mqtt communications thread: ", mosquitto_strerror( rc )));
        }

        connect();
    }

    void publish( string&& topic, string&& payload )
    {
        logger.debug( endpoint_, "registering publication for ", topic );

        Lock lock { mutex_ };
        if ( connected_ ) {
            sendPublish( topic, payload );
        } else {
            publications_.emplace_back( move( topic ), move( payload ) );
        }
    }

    void subscribe( string&& topic, function< void ( string payload ) >&& handler )
    {
        logger.debug( endpoint_, "registering subscription for ", topic );

        Lock lock { mutex_ };
        auto subscription = subscriptions_.emplace( move( topic ), move( handler ) );
        if ( connected_ ) {
            sendSubscribe( subscription->first );
        }
    }

private:
    void connect()
    {
        logger.info( endpoint_, "connecting to broker" );

        if ( int rc = mosquitto_connect_async( mosq_, endpoint_.host().c_str(), endpoint_.port(), 60 ) ) {
            logger.error( endpoint_, "error initiating connection: ", mosquitto_strerror( rc ) );
            retryConnect();
        }
    }

    void retryConnect()
    {
        if ( retries_++ == 0 ) {
            logger.info( endpoint_, "reconnecting immediately" );
            connect();
            return;
        }

        auto retryTimeout = chrono::seconds( min( static_cast< long >( pow( 2, retries_ - 1 )), 10L ));

        logger.info( endpoint_, "retrying connection in ", retryTimeout.count(), " seconds" );

        auto timer = make_shared< asio::steady_timer >( context_, retryTimeout );
        timer->async_wait( [this, timer]( auto ec ) { if ( !ec ) this->connect(); } );
    }

    void sendPublish( string const& topic, string const& payload )
    {
        logger.debug( endpoint_, "publishing message to ", topic );

        if ( int rc = mosquitto_publish( mosq_, nullptr, topic.c_str(), payload.length(), payload.data(), 0, false )) {
            logger.error( endpoint_, "error publishing to ", topic, ": ", mosquitto_strerror( rc ));
            // TODO
        }
    }

    void sendSubscribe( string const& topic )
    {
        logger.info( endpoint_, "subscribing to topic ", topic );

        if ( int rc = mosquitto_subscribe( mosq_, nullptr, topic.c_str(), 0 )) {
            logger.error( endpoint_, "error subscribing to ", topic, ": ", mosquitto_strerror( rc ));
            // TODO
        }
    }

    void on_connect( int rc )
    {
        if ( rc ) {
            logger.error( endpoint_, "error establishing connection, retrying automatically: ", mosquitto_strerror( rc ));
            mosquitto_reconnect_async( mosq_ );
            return;
        }

        logger.info( endpoint_, "connection established successfully" );

        Lock lock { mutex_ };
        connected_ = true;
        retries_ = 0;
        for ( auto const& subscription : subscriptions_ ) {
            sendSubscribe( subscription.first );
        }
        for ( auto const& publication : publications_ ) {
            sendPublish( publication.first, publication.second );
        }
        publications_.clear();
    }

    void on_disconnect( int rc )
    {
        if ( !connected_ ) {
            return;
        }

        logger.error( endpoint_, "connection lost, retrying automatically: ", mosquitto_strerror( rc ));

        Lock lock( mutex_ );
        connected_ = false;
        mosquitto_reconnect_async( mosq_ );
    }

    void on_message( mosquitto_message const& message )
    {
        auto payload = make_shared< string >(
                static_cast< char const* >( message.payload ),
                static_cast< size_t >( message.payloadlen ) );

        Lock lock { mutex_ };
        auto subscriptions = subscriptions_.equal_range( message.topic );
        for_each( subscriptions.first, subscriptions.second, [&]( auto const& subscription ) {
            asio::post( context_, [payload, &subscription] { subscription.second( *payload ); } );
        } );
    }

    static once_flag initialized;

    asio::io_context& context_;
    Endpoint endpoint_;
    mosquitto* mosq_ {};
    bool connected_ {};
    size_t retries_ {};
    list< pair< string, string > > publications_;
    unordered_multimap< string, function< void ( string payload ) > > subscriptions_;
    mutex mutex_;
};

once_flag Client::Impl::initialized;

Client::Client( asio::io_context& context, Endpoint endpoint )
        : impl_ { make_unique< Impl >( context, move( endpoint ) ) } {}

Client::~Client() = default;

void Client::publish( string topic, string payload )
{
    impl_->publish( move( topic ), move( payload ));
}

void Client::subscribe( string topic, function< void( string payload ) > handler )
{
    impl_->subscribe( move( topic ), move( handler ));
}

} // namespace mqtt
} // namespace dsmq