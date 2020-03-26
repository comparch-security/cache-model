#include "attack/create.hpp"
#include "cache/cache.hpp"
#include "util/random.hpp"

bool obtain_targeted_evict_set(
                               uint32_t num,
                               std::list<uint64_t>& candidate,
                               L1CacheBase * cache,
                               uint64_t target,
                               traverse_test_t traverse,
                               uint32_t trial
                               )
{
  for(int i=0; trial==0 || i<trial; i++) {
    candidate.clear();
    get_random_list(candidate, num, 1ull << 60);
    if(traverse(cache, candidate, target))
      return true;
  }
  return false;
}

