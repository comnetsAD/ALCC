#include "memory-without-slow-rewma.hh"

using namespace std;

static const double alpha = 1.0 / 8.0;

static const double slow_alpha = 1.0 / 256.0;

void Memory::packets_received( const vector< Packet > & packets, const unsigned int flow_id, const double link_rate_normalizing_factor )
{
  int seq_num = packets.front().seq_num;

  for ( const auto &x : packets ) {
    if ( x.flow_id != flow_id ) {
      continue;
    }

    //Warning: This does not detect packet losses at the boundaries of a group
    if ( seq_num < x.seq_num ){
      ++ _num_packets_lost;
    }
    ++ seq_num;

    const double rtt = x.tick_received - x.tick_sent;
    if ( _last_tick_sent == 0 || _last_tick_received == 0 ) {
      _last_tick_sent = x.tick_sent;
      _last_tick_received = x.tick_received;
      _last_receiver_timestamp = x.receiver_timestamp;
      _min_rtt = rtt;
    } else {
      _rec_send_ewma = (1 - alpha) * _rec_send_ewma + alpha * (x.tick_sent - _last_tick_sent) * link_rate_normalizing_factor;
      _rec_rec_ewma = (1 - alpha) * _rec_rec_ewma + alpha * (x.receiver_timestamp - _last_receiver_timestamp) * link_rate_normalizing_factor;
      _slow_rec_rec_ewma = (1 - slow_alpha) * _slow_rec_rec_ewma + slow_alpha * (x.receiver_timestamp - _last_receiver_timestamp) * link_rate_normalizing_factor;

      _last_tick_sent = x.tick_sent;
      _last_tick_received = x.tick_received;
      _last_receiver_timestamp = x.receiver_timestamp;

      _min_rtt = min( _min_rtt, rtt );
      _rtt_ratio = double( rtt ) / double( _min_rtt );
      assert( _rtt_ratio >= 1.0 );

      ++ _num_packets_received;
    }
  }
}

string Memory::str( void ) const
{
  char tmp[ 256 ];
  snprintf( tmp, 256, "sewma=%f, rewma=%f, rttr=%f, slowrewma=%f", _rec_send_ewma, _rec_rec_ewma, _rtt_ratio, _slow_rec_rec_ewma );
  return tmp;
}

const Memory & MAX_MEMORY( void )
{
  static const Memory max_memory( { 163840, 163840, 163840 } );
  return max_memory;
}

RemyBuffers::Memory Memory::DNA( void ) const
{
  RemyBuffers::Memory ret;
  ret.set_rec_send_ewma( _rec_send_ewma );
  ret.set_rec_rec_ewma( _rec_rec_ewma );
  ret.set_rtt_ratio( _rtt_ratio );
  //ret.set_slow_rec_rec_ewma( _slow_rec_rec_ewma );
  return ret;
}

/* If fields are missing in the DNA, we want to wildcard the resulting rule to match anything */
#define get_val_or_default( protobuf, field, limit ) \
  ( (protobuf).has_ ## field() ? (protobuf).field() : (limit) ? 0 : 163840 )

Memory::Memory( const bool is_lower_limit, const RemyBuffers::Memory & dna )
  : _rec_send_ewma( get_val_or_default( dna, rec_send_ewma, is_lower_limit ) ),
    _rec_rec_ewma( get_val_or_default( dna, rec_rec_ewma, is_lower_limit ) ),
    _rtt_ratio( get_val_or_default( dna, rtt_ratio, is_lower_limit ) ),
    //_slow_rec_rec_ewma( get_val_or_default( dna, slow_rec_rec_ewma, is_lower_limit ) ),
    _slow_rec_rec_ewma( 0 ),
    _last_tick_sent( 0 ),
    _last_tick_received( 0 ),
    _last_receiver_timestamp( 0 ),
    _min_rtt( 0 ),
    _num_packets_received( 0 ),
    _num_packets_lost( 0 )
{
}

size_t hash_value( const Memory & mem )
{
  size_t seed = 0;
  boost::hash_combine( seed, mem._rec_send_ewma );
  boost::hash_combine( seed, mem._rec_rec_ewma );
  boost::hash_combine( seed, mem._rtt_ratio );
  //boost::hash_combine( seed, mem._slow_rec_rec_ewma );

  return seed;
}
