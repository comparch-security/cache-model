#include "attack/algorithm.hpp"
#include "cache/cache.hpp"

static uint32_t loop_size, loop_count;

bool static loop_guard(uint32_t new_size) {
  if(loop_size == new_size) {
    loop_count++;
    if(loop_count > SearchMaxIteration)
      return false;
  } else {
    loop_count = 0;
    loop_size = new_size;
  }
  return true;
}

bool targeted_trim_original(
                            L1CacheBase * cache,
                            uint64_t target,
                            std::set<uint64_t> &candidate,
                            hit_func_t hit,
                            check_func_t check,
                            uint32_t nway
                            )
{
  std::set<uint64_t> picked_set, evict_set;
  loop_size = candidate.size(), loop_count = 0;
  while(candidate.size() > nway && loop_guard(candidate.size())) {
    if(!targeted_evict_random_pick(l1_caches[0], target, candidate, picked_set, evict_set, hit, 1))
      candidate.insert(*picked_set.begin());
    picked_set.clear();
  }
  return check(candidate, target);
}
