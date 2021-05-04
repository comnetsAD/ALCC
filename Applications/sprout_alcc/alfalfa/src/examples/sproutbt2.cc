#include <unistd.h>
#include <string>
#include <assert.h>
#include <list>
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include "sproutconn.h"
#include "select.h"

using namespace std;
using namespace Network;

int main( int argc, char *argv[] )
{
  struct timeval time2;
  char *ip;
  int port;
  ofstream WNDLog;
  char command[512];

  Network::SproutConnection *net;

  bool server = true;

  if ( argc > 2 ) {
    /* client */

    server = false;

    ip = argv[ 1 ];
    port = atoi( argv[ 2 ] );

    net = new Network::SproutConnection( "4h/Td1v//4jkYhqhLGgegw", ip, port );
  } else {
    net = new Network::SproutConnection( NULL, NULL );
  }

  fprintf( stderr, "Port bound is %d\n", net->port() );

  Select &sel = Select::get_instance();
  sel.add_fd( net->fd() );

  const int fallback_interval = 50;

  /* wait to get attached */
  if ( server ) {
  	sprintf (command, "%s", argv[1]);
    WNDLog.open(command);

    while ( 1 ) {
      int active_fds = sel.select( -1 );
      if ( active_fds < 0 ) {
	perror( "select" );
	exit( 1 );
      }

      if ( sel.read( net->fd() ) ) {
	net->recv();
      }

      if ( net->get_has_remote_addr() ) {
	break;
      }
    }
  }

  uint64_t time_of_next_transmission = timestamp() + fallback_interval;

  fprintf( stderr, "Looping...\n" );

  /* loop */
  while ( 1 ) {
  	int CWND = -1;
    int bytes_to_send = net->window_size(&CWND);

    /* actually send, maybe */
    if ( ( bytes_to_send > 0 ) || ( time_of_next_transmission <= timestamp() ) ) {
    	if (argc <= 2) {
        	gettimeofday(&time2,NULL);
        	WNDLog << time2.tv_sec << "." << time2.tv_usec << "," << CWND << "\n";
        	WNDLog.flush();
    		//printf("---- %ld.%06ld, %d \n", time2.tv_sec, time2.tv_usec, CWND);
    	}
      do {
	int this_packet_size = std::min( 1440, bytes_to_send );
	bytes_to_send -= this_packet_size;
	assert( bytes_to_send >= 0 );

	string garbage( this_packet_size, 'x' );

	int time_to_next = 0;
	if ( bytes_to_send == 0 ) {
	  time_to_next = fallback_interval;
	}

	net->send( garbage, time_to_next );
      } while ( bytes_to_send > 0 );

      time_of_next_transmission = std::max( timestamp() + fallback_interval,
					    time_of_next_transmission );
    }

    /* wait */
    int wait_time = time_of_next_transmission - timestamp();
    if ( wait_time < 0 ) {
      wait_time = 0;
    } else if ( wait_time > 10 ) {
      wait_time = 10;
    }

    int active_fds = sel.select( wait_time );
    if ( active_fds < 0 ) {
      perror( "select" );
      exit( 1 );
    }

    /* receive */
    if ( sel.read( net->fd() ) ) {
      string packet( net->recv() );
    }
  }
  WNDLog.close();
}
