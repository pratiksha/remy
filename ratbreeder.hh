#ifndef RATBREEDER_HH
#define RATBREEDER_HH

#include "evaluator.hh"
#include "network.hh"

class RatBreeder
{
private:
  //  Evaluator::ConfigRange _range;
  std::vector<NetConfig> _configs;

  void apply_best_split( WhiskerTree & whiskers, const unsigned int generation );

public:
  //  RatBreeder( Evaluator::ConfigRange & s_range ) : _range( s_range ) {}
  RatBreeder( std::vector<NetConfig> & configs ) : _configs( configs ) {}

  Evaluator::Outcome improve( WhiskerTree & whiskers );
};

#endif
