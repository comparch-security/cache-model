#include "attack/create.hpp"
#include "cache/cache.hpp"

#include <iostream>

bool targeted_evict_set_creator(
                                uint32_t num,
                                std::set<uint64_t>& candidate,
                                L1CacheBase * cache,
                                uint64_t target,
                                hit_func_t hit
                                )
{
  std::set<uint64_t> empty_set;
  get_random_set(candidate, num, 1ll << 60);
  return targeted_evict_probe(cache, target, candidate, empty_set, hit);
}

bool obtain_targeted_evict_set(
                               uint32_t num,
                               std::set<uint64_t>& candidate,
                               L1CacheBase * cache,
                               uint64_t target,
                               hit_func_t hit,
                               uint32_t trial
                               )
{
  uint32_t count = 0;
  while(!targeted_evict_set_creator(num, candidate, cache, target, hit)) {
    candidate.clear();
    if(trial == 0 || count < trial) count++;
    else return false;
  }
  return true; 
}

