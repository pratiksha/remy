#include <cstdio>
#include <vector>
#include <cassert>
#include <future>
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include "ratbreeder.hh"

using namespace std;

void RatBreeder::apply_best_split( WhiskerTree & whiskers, const unsigned int generation )
{
  const Evaluator eval( whiskers, _range );
  auto outcome( eval.score( {}, true ) );

  while ( 1 ) {
    auto my_whisker( outcome.used_whiskers.most_used( generation ) );
    assert( my_whisker );

    WhiskerTree bisected_whisker( *my_whisker, true );

    if ( bisected_whisker.num_children() == 1 ) {
      //      printf( "Got unbisectable whisker! %s\n", my_whisker->str().c_str() );
      auto mutable_whisker( *my_whisker );
      mutable_whisker.promote( generation + 1 );
      assert( outcome.used_whiskers.replace( mutable_whisker ) );
      continue;
    }

    //    printf( "Bisecting === %s === into === %s ===\n", my_whisker->str().c_str(), bisected_whisker.str().c_str() );
    assert( whiskers.replace( *my_whisker, bisected_whisker ) );
    break;
  }
}

Evaluator::Outcome RatBreeder::improve( WhiskerTree & whiskers )
{
  /* back up the original whiskertree */
  /* this is to ensure we don't regress */
  WhiskerTree best_so_far( whiskers );

  /* evaluate the whiskers we have */
  whiskers.reset_generation();
  unsigned int generation = 0;

  while ( 1 ) {
    const Evaluator eval( whiskers, _range );
    unordered_map< Whisker, double, boost::hash< Whisker > > evalcache;

    auto outcome( eval.score( {} ) );

    /* is there a whisker at this generation that we can improve? */
    auto my_whisker( outcome.used_whiskers.most_used( generation ) );

    /* if not, increase generation and promote all whiskers */
    if ( !my_whisker ) {
      generation++;
      whiskers.promote( generation );

      if ( (generation % 4) == 0 ) {
	apply_best_split( whiskers, generation );
	generation++;
	whiskers.promote( generation );
	break;
      }

      continue;
    }

    Whisker differential_whisker( *my_whisker );
    unsigned int diffgen = my_whisker->generation();

    /* otherwise, get all nexgen alternatives for that whisker and replace each in turn */
    while ( 1 ) {
      auto replacements( differential_whisker.next_generation() );
      double best_score = -INT_MAX;
      const Whisker *best_whisker = nullptr;

      printf( "Evaluating %lu replacements for %s.\n", replacements.size(), differential_whisker.str().c_str() );

      vector< pair< Whisker &, future< double > > > scores;
      vector< pair< Whisker &, double > > memoized_scores;

      /* find best case (using same randseed) */
      for ( auto &test_replacement : replacements ) {
	//	printf( "Evaluating %s... ", test_replacement.str().c_str() );
	if ( evalcache.find( test_replacement ) == evalcache.end() ) {
	  scores.emplace_back( test_replacement, async( launch::async, [] (const Evaluator &e, const Whisker &r) { return e.score( { r } ).score; }, eval, test_replacement ) );
	} else {
	  memoized_scores.emplace_back( test_replacement, evalcache.at( test_replacement ) );
	}
      }

      for ( auto &x : memoized_scores ) {
	const double score( x.second );
	//	printf( "score = %f\n", score );
	if ( score > best_score ) {
	  best_whisker = &x.first;
	  best_score = score;
	}
      }

      for ( auto &x : scores ) {
	const double score( x.second.get() );
	evalcache.insert( make_pair( x.first, score ) );
	//	printf( "score = %f\n", score );
	if ( score > best_score ) {
	  best_whisker = &x.first;
	  best_score = score;
	}
      }

      vector< pair< Whisker &, future< double > > > scores2;
      vector< pair< Whisker &, double > > memoized_scores2;

      assert( best_whisker );
      printf("best whisker is now %s\n", best_whisker->str().c_str());
      auto new_replacements( best_whisker->next_generation_intersend() );

      printf( "Evaluating %lu intersend replacements for %s.\n", replacements.size(), best_whisker->str().c_str() );

      /* find best case (using same randseed) */
      for ( auto &test_replacement : new_replacements ) {
	//	printf( "Evaluating %s... ", test_replacement.str().c_str() );
	if ( evalcache.find( test_replacement ) == evalcache.end() ) {
	  scores2.emplace_back( test_replacement, async( launch::async, [] (const Evaluator &e, const Whisker &r) { return e.score( { r } ).score; }, eval, test_replacement ) );
	} else {
	  memoized_scores2.emplace_back( test_replacement, evalcache.at( test_replacement ) );
	}
      }

      for ( auto &x : memoized_scores2 ) {
	const double score( x.second );
	//	printf( "score = %f\n", score );
	if ( score > best_score ) {
	  best_whisker = &x.first;
	  best_score = score;
	}
      }

      for ( auto &x : scores2 ) {
	const double score( x.second.get() );
	evalcache.insert( make_pair( x.first, score ) );
	//	printf( "score = %f\n", score );
	if ( score > best_score ) {
	  best_whisker = &x.first;
	  best_score = score;
	}
      }

      assert( best_whisker );
      printf("best whisker after intersend is now %s\n", best_whisker->str().c_str());

      /* replace with best nexgen choice and repeat */
      if ( best_score > outcome.score ) {
	printf( "Replacing with whisker that scored %.12f => %.12f (+%.12f)\n", outcome.score, best_score,
		best_score - outcome.score );
	//	printf( "=> %s\n", best_whisker->str().c_str() );
	assert( whiskers.replace( *best_whisker ) );
	differential_whisker = *best_whisker;
	differential_whisker.demote( diffgen );
	outcome.score = best_score;
	printf( "whiskers %s\n", whiskers.str().c_str() );
      } else {
	assert( whiskers.replace( *best_whisker ) );
	printf( "Done with search.\n" );
	printf( "** whiskers %s\n", whiskers.str().c_str() );
	break;
      }
    }
  }

  /* carefully evaluate what we have vs. the previous best */
  const Evaluator eval2( {}, _range );
  const auto new_score = eval2.score( whiskers, false, 10 );
  const auto old_score = eval2.score( best_so_far, false, 10 );

  if ( old_score.score >= new_score.score ) {
    fprintf( stderr, "Regression, old=%f, new=%f\n", old_score.score, new_score.score );
    whiskers = best_so_far;
    return old_score;
  }

  return new_score;
}
