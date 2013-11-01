#include <stdio.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "evaluator.hh"

using namespace std;

int main( int argc, char *argv[] )
{
  WhiskerTree whiskers;
  unsigned int num_senders = 2;
  double link_ppt = 1.0;
  double delay = 100.0;

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
    } else if ( arg.substr( 0, 5 ) == "nsrc=" ) {
      num_senders = atoi( arg.substr( 5 ).c_str() );
      fprintf( stderr, "Setting num_senders to %d\n", num_senders );
    } else if ( arg.substr( 0, 5 ) == "link=" ) {
      link_ppt = atof( arg.substr( 5 ).c_str() );
      fprintf( stderr, "Setting link packets per ms to %f\n", link_ppt );
    } else if ( arg.substr( 0, 4 ) == "rtt=" ) {
      delay = atof( arg.substr( 4 ).c_str() );
      fprintf( stderr, "Setting delay to %f ms\n", delay );
    }
  }

  Evaluator::ConfigRange range;
  range.link_packets_per_ms = make_pair( link_ppt, 0 ); /* 1 Mbps to 10 Mbps */
  range.rtt_ms = make_pair( delay, 0 ); /* ms */
  range.max_senders = num_senders;
  range.lo_only = true;


  ///// Now create the configurations to test on. /////

  std::vector<NetConfig> configs;

  /* first load "anchors" */
  configs.push_back( NetConfig().set_link_ppt( range.link_packets_per_ms.first ).set_delay( range.rtt_ms.first ).set_num_senders( range.max_senders ) );

  configs.push_back( NetConfig().set_link_ppt( range.link_packets_per_ms.first ).set_delay( range.rtt_ms.first ).set_num_senders( 1 ) );

  configs.push_back( NetConfig().set_link_ppt( range.link_packets_per_ms.first ).set_delay( range.rtt_ms.first ).set_num_senders( 2 ).set_on_duration( 500 ).set_off_duration( 500 ) );

  Evaluator eval( whiskers, configs );
  auto outcome = eval.score( {}, false, 10 );
  printf( "score = %f\n", outcome.score );
  double norm_score = 0;

  for ( auto &run : outcome.throughputs_delays ) {
    printf( "===\nconfig: %s\n", run.first.str().c_str() );
    for ( auto &x : run.second ) {
      printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
      norm_score += log2( x.first / run.first.link_ppt ) - log2( x.second / run.first.delay );
    }
  }

  printf( "normalized_score = %f\n", norm_score );


  printf( "Windows:" );
  unsigned int total_count = 0;

  for ( unsigned int i = 0; i <= MAX_WINDOW; i++ ) {
    total_count += outcome.used_whiskers.used_windows()[ i ];
  }

  for ( unsigned int i = 0; i <= MAX_WINDOW; i++ ) {
    unsigned int count = outcome.used_whiskers.used_windows()[ i ];
    if ( count ) {
      printf( " [%u=%.4f]", i, double( count ) / double( total_count ) );
    }
  }
  printf( "\n" );

  printf( "Whiskers: %s\n", outcome.used_whiskers.str().c_str() );

  return 0;
}
