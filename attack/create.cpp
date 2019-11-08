#include "attack/create.hpp"
#include "cache/cache.hpp"

bool targeted_evict_set_creator(
                                uint32_t num,
                                std::unordered_set<uint64_t>& candidate,
                                L1CacheBase * cache,
                                uint64_t target,
                                uint32_t cycle,
                                uint32_t write,
                                uint32_t threshold,
                                hit_func_t hit
                                )
{
  std::unordered_set<uint64_t> empty_set;
  get_random_set(candidate, num, 1ll << 60);
  return targeted_evict_probe(cache, target, candidate, empty_set, cycle, write, threshold, hit);
}

bool obtain_targeted_evict_set(
                               uint32_t num,
                               std::unordered_set<uint64_t>& candidate,
                               L1CacheBase * cache,
                               uint64_t target,
                               uint32_t cycle,
                               uint32_t write,
                               uint32_t threshold,
                               hit_func_t hit,
                               uint32_t trial
                               )
{
  uint32_t count = 0;
  while(!targeted_evict_set_creator(num, candidate, cache, target, cycle, write, threshold, hit)) {
    candidate.clear();
    if(trial == 0 || count < trial) count++;
    else return false;
  }
  return true; 
}

