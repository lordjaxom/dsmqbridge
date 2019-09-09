#include <csignal>
#include <algorithm>
#include <list>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <experimental/optional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>

#include "dss_client.hpp"
#include "logging.hpp"
#include "manager.hpp"
#include "mqtt_client.hpp"

using namespace std;
using namespace std::experimental;
using namespace nlohmann;

namespace asio = boost::asio;
namespace ssl = asio::ssl;

namespace dsmq {

static Logger logger( "manager" );

class MappingTable
{
    friend void from_json( json const& src, MappingTable& dst )
    {
        for ( auto const& item : src ) {
            dst.mqs_.push_back( item.at( "MQ" ));
            dst.mq2ds_.emplace( item.at( "MQ" ), item.at( "dS" ));
            dst.ds2mq_.emplace( item.at( "dS" ), item.at( "MQ" ));
        }
    }

public:
    vector< string > const& mqs() const { return mqs_; }

    optional< unsigned > mq2ds( string const& val ) const
    {
        auto it = mq2ds_.find( val );
        return it != mq2ds_.end() ? optional< unsigned > { it->second } : nullopt;
    }

    string const* ds2mq( unsigned val ) const
    {
        auto it = ds2mq_.find( val );
        return it != ds2mq_.end() ? &it->second : nullptr;
    }

private:
    vector< string > mqs_;
    unordered_map< string, unsigned > mq2ds_;
    unordered_map< unsigned, string > ds2mq_;
};

class ZoneTable : MappingTable
{
    friend void from_json( json const& src, ZoneTable& dst )
    {
        from_json( src, static_cast< MappingTable& >( dst ));
        for ( auto const& item : src ) {
            if ( item.count( "groups" ) > 0 ) {
                dst.groupsByMq_.emplace( item.at( "MQ" ), item.at( "groups" ));
            }
            if ( item.count( "scenes" ) > 0 ) {
                dst.sceneOverrides_.emplace( item.at( "MQ" ), item.at( "scenes" ));
            }
        }
    }

public:
    using MappingTable::mqs;
    using MappingTable::mq2ds;
    using MappingTable::ds2mq;

    vector< string > const& groupsByMq( string const& zone, MappingTable const& groupTable ) const
    {
        auto it = groupsByMq_.find( zone );
        return it != groupsByMq_.end() ? it->second : groupTable.mqs();
    }

    string const* groupDS2Mq( string const& zone, unsigned group, MappingTable const& groupTable ) const
    {
        if ( auto result = groupTable.ds2mq( group ) ) {
            auto it = groupsByMq_.find( zone );
            if ( it == groupsByMq_.end() || find( it->second.begin(), it->second.end(), *result ) != it->second.end() ) {
                return result;
            }
        }
        return nullptr;
    }

    optional< unsigned > sceneMq2DS( string const& zone, string const& scene, MappingTable const& sceneTable ) const
    {
        auto it = sceneOverrides_.find( zone );
        if ( it != sceneOverrides_.end()) {
            if ( auto result = it->second.mq2ds( scene )) {
                return result;
            }
        }
        return sceneTable.mq2ds( scene );
    }

    string const* sceneDS2Mq( string const& zone, unsigned scene, MappingTable const& sceneTable ) const
    {
        auto it = sceneOverrides_.find( zone );
        if ( it != sceneOverrides_.end()) {
            if ( auto result = it->second.ds2mq( scene )) {
                return result;
            }
        }
        return sceneTable.ds2mq( scene );
    }

private:
    unordered_map< string, vector< string > > groupsByMq_;
    unordered_map< string, MappingTable > sceneOverrides_;
};

class Manager::Impl
{
public:
    explicit Impl( json const& props )
            : topicTemplate_ { props.at( "topicTemplate" ).get< string >() }
            , zoneTable_ { props.at( "zones" ) }
            , groupTable_ { props.at( "groups" ) }
            , sceneTable_ { props.at( "scenes" ) }
            , reloadSignals_ { context_ }
            , mqtt_ { context_, props.at( "MQTT" ) }
            , dss_ { context_, sslContext_, props.at( "dSS" ) }
    {
#if !defined( WIN32 )
        reloadSignals_.add( SIGHUP );
#endif
        subscribeReloadSignals();

        for ( auto const& zone : zoneTable_.mqs() ) {
            for ( auto const& group : zoneTable_.groupsByMq( zone, groupTable_ ) ) {
                mqtt_.subscribe(
                        topicName( zone, group ),
                        [this, &zone, &group]( auto payload ) { this->on_callScene( zone, group, move( payload )); } );
            }
        }
        dss_.subscribe< dss::EventCallScene >( [this]( auto event ) { this->on_callScene( move( event ) ); } );
        dss_.eventLoop();
    }

    void run()
    {
        context_.run();
    }

private:
    void subscribeReloadSignals()
    {
        reloadSignals_.async_wait( [this]( auto ec, int signal ) {
            if ( ec != make_error_code( asio::error::operation_aborted )) {
                Logger::reopen();
                this->subscribeReloadSignals();
            }
        } );
    }

    string topicName( string const& zone, string const& group ) const
    {
        return ( boost::format( topicTemplate_ ) % zone % group ).str();
    }

    void forwardMq( string const &zone, string const &group, string const &scene )
    {
        auto topic = topicName( zone, group );
        logger.info( "forwarding MQTT scene ", scene, " to topic ", topic );
        mqtt_.publish( move( topic ), scene );

        auto it = forwardedMqScenes_.emplace( piecewise_construct, forward_as_tuple( zone, group, scene ),
                forward_as_tuple( context_, chrono::milliseconds( 500 )));
        it->second.async_wait( [this, it]( auto ec ) {
            if ( ec != make_error_code( asio::error::operation_aborted )) {
                forwardedMqScenes_.erase( it );
            }
        } );
    }

    void forwardDS( unsigned zone, unsigned group, unsigned scene )
    {
        logger.info( "forwarding dSS scene ", scene, " to zone ", zone, ", group ", group );
        dss_.callScene( zone, group, scene );

        auto it = forwardedDSScenes_.emplace( piecewise_construct, forward_as_tuple( zone, group, scene ),
                forward_as_tuple( context_, chrono::seconds( 5 )));
        it->second.async_wait( [this, it]( auto ec ) {
            if ( ec != make_error_code( asio::error::operation_aborted )) {
                forwardedDSScenes_.erase( it );
            }
        } );
    }

    void on_callScene( dss::EventCallScene&& event )
    {
        logger.debug( "received dSS callScene from zone ", event.zone(), ", group ", event.group(), ", scene ", event.scene() );

        auto range = forwardedDSScenes_.equal_range( forward_as_tuple( event.zone(), event.group(), event.scene()));
        if ( range.first != range.second ) {
            forwardedDSScenes_.erase( range.first );
            return;
        }

        if ( auto targetZone = zoneTable_.ds2mq( event.zone())) {
            if ( auto targetScene = zoneTable_.sceneDS2Mq( *targetZone, event.scene(), sceneTable_ )) {
                if ( event.group() == 0 ) {
                    for ( auto const &targetGroup : zoneTable_.groupsByMq( *targetZone, groupTable_ )) {
                        forwardMq( *targetZone, targetGroup, *targetScene );
                    }
                } else if ( auto targetGroup = zoneTable_.groupDS2Mq( *targetZone, event.group(), groupTable_ )) {
                    forwardMq( *targetZone, *targetGroup, *targetScene );
                }
            }
        }
    }

    void on_callScene( string const& zone, string const& group, string const& scene )
    {
        logger.debug( "received MQ callScene from zone ", zone, ", group ", group, ", scene ", scene );

        auto range = forwardedMqScenes_.equal_range( forward_as_tuple( zone, group, scene ));
        if ( range.first != range.second ) {
            forwardedMqScenes_.erase( range.first );
            return;
        }

        auto targetZone = zoneTable_.mq2ds( zone );
        auto targetGroup = groupTable_.mq2ds( group );
        auto targetScene = zoneTable_.sceneMq2DS( zone, scene, sceneTable_ );
        forwardDS( *targetZone, *targetGroup, *targetScene );
    }

    string topicTemplate_;
    ZoneTable zoneTable_;
    MappingTable groupTable_;
    MappingTable sceneTable_;
    asio::io_context context_;
    ssl::context sslContext_ { ssl::context::sslv23_client };
    asio::signal_set reloadSignals_;
    mqtt::Client mqtt_;
    dss::Client dss_;
    multimap< tuple< unsigned, unsigned, unsigned >, asio::steady_timer > forwardedDSScenes_;
    multimap< tuple< string, string, string >, asio::steady_timer > forwardedMqScenes_;
};

Manager::Manager( json const& props )
        : impl_ { make_unique< Impl >( props ) } {}

Manager::~Manager() = default;

void Manager::run()
{
    impl_->run();
}

} // namespace dsmq