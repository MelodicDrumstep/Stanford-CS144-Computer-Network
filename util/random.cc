#include "random.hh"

#include <algorithm>
#include <array>

using namespace std;

// #define FIX_RANDOM_SEED

default_random_engine get_random_engine()
{
  #ifdef FIX_RANDOM_SEED
    // For debugging only
    const uint32_t seed = 12345;
    return default_random_engine(seed);
  #else
    auto rd = random_device();
    array<uint32_t, 1024> seed_data {};
    generate( seed_data.begin(), seed_data.end(), [&] { return rd(); } );
    seed_seq seed( seed_data.begin(), seed_data.end() );
    return default_random_engine( seed );
  #endif
}
