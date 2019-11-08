#include "cache/cache.hpp"
#include "attack/algorithm.hpp"
#include "attack/definitions.hpp"

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
                            std::unordered_set<uint64_t> &candidate,
                            uint32_t cycle,
                            uint32_t write,
                            uint32_t threshold,
                            hit_func_t hit,
                            check_func_t check,
                            uint32_t nway
                            )
{
  std::unordered_set<uint64_t> picked_set, evict_set;
  loop_size = candidate.size(), loop_count = 0;
  while(candidate.size() > nway && loop_guard(candidate.size())) {
    if(!targeted_evict_random_pick(cache, target, candidate, picked_set, evict_set, cycle, write, threshold, hit, 1))
      candidate.insert(*picked_set.begin());
    picked_set.clear();
  }
  return check(candidate);
}
