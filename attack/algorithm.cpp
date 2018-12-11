#include "attack/algorithm.hpp"
#include "cache/cache.hpp"

bool targeted_trim_original(
                            L1CacheBase * cache,
                            uint64_t target,
                            std::set<uint64_t> &candidate,
                            hit_func_t hit
                            )
{
  std::set<uint64_t> picked_set, evict_set;
  while(candidate.size() > 0) {
    if(!targeted_evict_random_pick(l1_caches[0], target, candidate, picked_set, evict_set, hit, 1))
      evict_set.insert(*picked_set.begin());
    picked_set.clear();
  }
  candidate.insert(evict_set.begin(), evict_set.end());
  return true;
}

bool targeted_trim_divide_W_1(
                              L1CacheBase * cache,
                              uint64_t target,
                              std::set<uint64_t> &candidate,
                              hit_func_t hit,
                              uint32_t nway
                              )
{
  std::set<uint64_t> picked_set, evict_set;
  uint32_t pre_size = candidate.size(), count = 0;
  while(candidate.size() > 0) {
    // loop guard
    if(pre_size == candidate.size()) count++;
    else count = 0;
    if(count > SearchMaxIteration) return false;
    pre_size = candidate.size();

    uint32_t step = candidate.size() / (nway+1);
    if(step > 1) {
      for(uint32_t i=0; i<nway; i++) {
        if(!targeted_evict_random_pick(l1_caches[0], target, candidate, picked_set, evict_set, hit, step))
          evict_set.insert(picked_set.begin(), picked_set.end());
        picked_set.clear();
      }
      if(targeted_evict_probe(l1_caches[0], target, picked_set, evict_set, hit))
        candidate.clear();
      candidate.insert(evict_set.begin(), evict_set.end());
      evict_set.clear();
    } else {
      if(!targeted_evict_random_pick(l1_caches[0], target, candidate, picked_set, evict_set, hit, 1))
        evict_set.insert(*picked_set.begin());
      picked_set.clear();
    }
  }
  candidate.insert(evict_set.begin(), evict_set.end());
  return true; 
}

