#ifndef UTILITY_HH
#define UTILITY_HH

#include <cmath>

// Tracks the utility variables that we are interested in
class Utility
{
private:
  // Total time for which sending was taking place. Used to calculate throughput.
  double _tick_share_sending;
  unsigned int _packets_received;
  double _total_delay;

public:
  Utility( void ) : _tick_share_sending( 0 ), _packets_received( 0 ), _total_delay( 0 ) {}

  void sending_duration( const double & duration, const unsigned int num_sending ) { _tick_share_sending += duration / double( num_sending ); }
  void packets_received( const std::vector< Packet > & packets ) {
    _packets_received += packets.size();

    for ( auto &x : packets ) {
      assert( x.tick_received >= x.tick_sent );
      _total_delay += x.tick_received - x.tick_sent;
    }
  }

  double average_throughput( void ) const
  {
    if ( _tick_share_sending == 0 ) {
      return 0.0;
    }
    return double( _packets_received ) / _tick_share_sending;
  }

  double average_delay( void ) const
  {
    if ( _packets_received == 0 ) {
      return 0.0;
    }
    return double( _total_delay ) / double( _packets_received );
  }

  double utility( void ) const
  {
    if ( _tick_share_sending == 0 ) {
      return 0.0;
    }

    if ( _packets_received == 0 ) {
      return -INT_MAX;
    }

    const double throughput_utility = log2( average_throughput() );
    const double delay_penalty = log2( average_delay() / 100.0 );

    return throughput_utility - delay_penalty;
  }
};

#endif
