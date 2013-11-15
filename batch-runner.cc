#include <stdio.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "evaluator.hh"
#include "global-settings.hh"
#include "ratbreeder.hh"

using namespace std;


void train_remy(std::vector<NetConfig> configs) {
  WhiskerTree whiskers;

  RatBreeder breeder( configs );

  unsigned int run = 0;

  printf( "********** Training **********\n" );

  const NetConfig defaultnet;

  printf( "Optimizing for mean_on_duration = %f, mean_off_duration = %f\n",
	  defaultnet.mean_on_duration, defaultnet.mean_off_duration );

  //  printf( "Initial rules (use if=FILENAME to read from disk): %s\n", whiskers.str().c_str() );
  //  printf( "#######################\n" );

  printf( "Not saving output! Use remy.cc to save output.");

  while ( 1 ) {
    auto outcome = breeder.improve( whiskers );
    printf( "run = %u, score = %f\n", run, outcome.score );

    for ( auto &run : outcome.throughputs_delays ) {
      printf( "===\nconfig: %s\n", run.first.str().c_str() );
      for ( auto &x : run.second ) {
	printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
      }
    }

    fflush( NULL );
    run++;
  }
}


void test_remy( void ) {


}


int main( int argc, char *argv[] )
{
  WhiskerTree whiskers;
  string output_filename;

  for ( int i = 1; i < argc; i++ ) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "if=" ) {
      string filename( arg.substr( 3 ) );
      int fd = open( filename.c_str(), O_RDONLY );
      if ( fd < 0 ) {
	perror( "open" );
	exit( 1 );
      }

      RemyBuffers::WhiskerTree tree;
      if ( !tree.ParseFromFileDescriptor( fd ) ) {
	fprintf( stderr, "Could not parse %s.\n", filename.c_str() );
	exit( 1 );
      }
      whiskers = WhiskerTree( tree );

      if ( close( fd ) < 0 ) {
	perror( "close" );
	exit( 1 );
      }
    } else if ( arg.substr( 0, 3 ) == "of=" ) {
      output_filename = string( arg.substr( 3 ) );
    }
  }

  g_RemySettings.axis_values = {1, 2};

  Evaluator::ConfigRange range;
  range.link_packets_per_ms = make_pair( 0.1, 2.0 ); /* 10 Mbps to 20 Mbps */
  range.rtt_ms = make_pair( 100, 200 ); /* ms */
  range.max_senders = 2;
  range.lo_only = true;

  std::vector<NetConfig> configs;

  configs.push_back( NetConfig().set_link_ppt( range.link_packets_per_ms.first ).set_delay( range.rtt_ms.first ).set_num_senders( range.max_senders ) );

  configs.push_back( NetConfig().set_link_ppt( range.link_packets_per_ms.first ).set_delay( range.rtt_ms.first ).set_num_senders( 1 ) );

  configs.push_back( NetConfig().set_link_ppt( range.link_packets_per_ms.first ).set_delay( range.rtt_ms.first ).set_num_senders( 2 ).set_on_duration( 500 ).set_off_duration( 500 ) );

  printf( "Optimizing for link packets_per_ms in [%f, %f], ",
	  range.link_packets_per_ms.first,
	  range.link_packets_per_ms.second );
  printf( "rtt_ms in [%f, %f], ",
	  range.rtt_ms.first,
	  range.rtt_ms.second );
  printf( "num_senders = 1-%d.\n",
	  range.max_senders );

  train_remy(configs);

  return 0;
}
