#include "cache/cache.hpp"
#include "attack/search.hpp"
#include "attack/create.hpp"
#include "attack/definitions.hpp"

const uint32_t NTrial = 16;

void split_random_set(
                      std::unordered_set<uint64_t> &candidate,
                      std::unordered_set<uint64_t> &picked_set,
                      uint32_t pick
                      )
{
  std::unordered_set<uint32_t> picked_index;
  get_random_set(picked_index, pick, candidate.size());
  uint32_t i = 0;
  for(auto it=candidate.begin(); it != candidate.end(); ++i, ++it)
    if(picked_index.count(i)) picked_set.insert(*it);
  for(auto it:picked_set) candidate.erase(it);
}

bool targeted_evict_probe(
                          L1CacheBase * cache,
                          uint64_t target,
                          std::unordered_set<uint64_t> &candidate,
                          std::unordered_set<uint64_t> &evict_set,
                          uint32_t cycle,
                          uint32_t write,
                          uint32_t threshold,
                          hit_func_t hit
                          )
{
  uint32_t count = 0;
  for(uint32_t i=0; i<cycle; i++) {
    cache->read(target);
    for(uint32_t j=0; j<write; j++) {
      for(auto line: candidate) cache->read(line);
      for(auto line: evict_set) cache->read(line);
    }
    if(!hit(target)) {
      count++;
    }
  }
  return count > threshold;
}

bool targeted_evict_random_pick(
                                L1CacheBase * cache,
                                uint64_t target,
                                std::unordered_set<uint64_t> &candidate,
                                std::unordered_set<uint64_t> &picked_set,
                                std::unordered_set<uint64_t> &evict_set,
                                uint32_t cycle,
                                uint32_t write,
                                uint32_t threshold,
                                hit_func_t hit,
                                uint32_t pick
                                )
{
  split_random_set(candidate, picked_set, pick);
  return targeted_evict_probe(cache, target, candidate, evict_set, cycle, write, threshold, hit);
}
