#include <algorithm>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>
#include <experimental/optional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
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

    optional< string > ds2mq( unsigned val ) const
    {
        auto it = ds2mq_.find( val );
        return it != ds2mq_.end() ? optional< string > { it->second } : nullopt;
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

    optional< string > groupDS2Mq( string const& zone, unsigned group, MappingTable const& groupTable ) const
    {
        if ( auto result = groupTable.ds2mq( group ) ) {
            auto it = groupsByMq_.find( zone );
            if ( it == groupsByMq_.end() || find( it->second.begin(), it->second.end(), *result ) != it->second.end() ) {
                return result;
            }
        }
        return nullopt;
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

    optional< string > sceneDS2Mq( string const& zone, unsigned scene, MappingTable const& sceneTable ) const
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
            , mqtt_ { context_, props.at( "MQTT" ) }
            , dss_ { context_, sslContext_, props.at( "dSS" ) }
    {
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
    string topicName( string const& zone, string const& group ) const
    {
        return ( boost::format( topicTemplate_ ) % zone % group ).str();
    }

    void forwardMq( string const &zone, string const &group, string const &scene )
    {
        auto it = knownMqScenes_.find( make_pair< string const&, string const& >( zone, group ) );
        if ( it == knownMqScenes_.end() || it->second != scene ) {
            auto topic = topicName( zone, group );
            logger.debug( "forwarding MQ scene ", scene, " to ", topic );
            mqtt_.publish( move( topic ), scene );
        }
    }

    void forwardDS( unsigned zone, unsigned group, unsigned scene )
    {
        auto key = make_pair( zone, group );
        auto it = knownDSScenes_.find( key );
        if ( it == knownDSScenes_.end() || it->second != scene ) {
            logger.debug( "forwarding dS scene ", scene, " to zone ", zone, ", group ", group );
            dss_.callScene( zone, group, scene );
            knownDSScenes_[ key ] = scene;
        }
    }

    void on_callScene( dss::EventCallScene&& event )
    {
        logger.debug( "received dSS callScene from zone ", event.zone(), ", group ", event.group(), ", scene ", event.scene() );

        if ( auto targetZone = zoneTable_.ds2mq( event.zone())) {
            if ( auto targetScene = zoneTable_.sceneDS2Mq( *targetZone, event.scene(), sceneTable_ )) {
                if ( event.group() == 0 ) {
                    for ( auto const &targetGroup : zoneTable_.groupsByMq( *targetZone, groupTable_ )) {
                        knownDSScenes_[make_pair( event.zone(), *groupTable_.mq2ds( targetGroup ))] = event.scene();
                        forwardMq( *targetZone, targetGroup, *targetScene );
                    }
                } else if ( auto targetGroup = zoneTable_.groupDS2Mq( *targetZone, event.group(), groupTable_ )) {
                    knownDSScenes_[make_pair( event.zone(), event.group())] = event.scene();
                    forwardMq( *targetZone, *targetGroup, *targetScene );
                }
            }
        }
    }

    void on_callScene( string const& zone, string const& group, string const& scene )
    {
        logger.debug( "received MQ callScene from zone ", zone, ", group ", group, ", scene ", scene );

        knownMqScenes_[make_pair( zone, group )] = scene;

        auto targetZone = zoneTable_.mq2ds( zone );
        auto targetGroup = groupTable_.mq2ds( group );
        auto targetScene = zoneTable_.sceneMq2DS( zone, scene, sceneTable_ );
        forwardDS( *targetZone, *targetGroup, *targetScene );
    }

    asio::io_context context_;
    ssl::context sslContext_ { ssl::context::sslv23_client };
    string topicTemplate_;
    ZoneTable zoneTable_;
    MappingTable groupTable_;
    MappingTable sceneTable_;
    mqtt::Client mqtt_;
    dss::Client dss_;
    map< pair< unsigned, unsigned >, unsigned > knownDSScenes_;
    map< pair< string, string >, string > knownMqScenes_;
};

Manager::Manager( json const& props )
        : impl_ { make_unique< Impl >( props ) } {}

Manager::~Manager() = default;

void Manager::run()
{
    impl_->run();
}

} // namespace dsmq