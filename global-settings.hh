/* 
   Global settings for the purpose of being able to change
   low-level defaults (e.g. datasize) without bubbling things down.
   
   For use in running lots of tests.
 */

#ifndef GLOBAL_SETTINGS_HH
#define GLOBAL_SETTINGS_HH

#include <vector>


class GlobalSettings {
public:
  bool use_globals;
  unsigned int memory_datasize;
  std::vector<int> axis_values; // axes of interest to bisect on

  GlobalSettings()
    : use_globals( true ),
      memory_datasize( 3 ),
      axis_values()
  {
    axis_values.push_back(0);
    axis_values.push_back(2);
  }

};

extern GlobalSettings g_RemySettings;

#endif
