#include "evaluator.hh"
#include "network.hh"
#include "network.cc"
#include "rat-templates.cc"

const unsigned int TICK_COUNT = 100000;

Evaluator::Evaluator( const WhiskerTree & s_whiskers, std::vector<NetConfig> configs)
  : _prng( global_PRNG()() ), /* freeze the PRNG seed for the life of this Evaluator */
    _whiskers( s_whiskers ),
    _configs( configs )
{
}

Evaluator::Outcome Evaluator::score( const std::vector< Whisker > & replacements,
				     const bool trace, const unsigned int carefulness ) const
{
  PRNG run_prng( _prng );

  WhiskerTree run_whiskers( _whiskers );
  for ( const auto &x : replacements ) {
    assert( run_whiskers.replace( x ) );
  }

  run_whiskers.reset_counts();

  /* run tests */
  Outcome the_outcome;
  for ( auto &x : _configs ) {
    /* run once */
    //    printf("%s\n", x.str().c_str());
    Network<Rat> network1( Rat( run_whiskers, trace ), run_prng, x );
    network1.tick( TICK_COUNT * carefulness );

    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }

  the_outcome.used_whiskers = run_whiskers;

  return the_outcome;
}
