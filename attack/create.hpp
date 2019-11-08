#ifndef ATT_CREATE_HPP_
#define ATT_CREATE_HPP_

#include <cstdint>
#include <unordered_set>
#include <functional>
#include "datagen/include/random_generator.h"
#include "attack/search.hpp"

template<typename T>
void get_random_set(
                    std::unordered_set<T> &random_set, // the set containing the random numbers
                    uint32_t num,            // number of random number to be generated
                    uint64_t max             // the maximam number to be generated
                    )
{
  if(max <= num) {
    for(uint32_t i=0; i<max; i++)
      random_set.insert(random_set.begin(), i);
  } else {
    for(uint32_t i=0; random_set.size()<num; i++)
      random_set.insert(random_set.begin(), random_uint_uniform(60, 0, max-1));
  }
}

typedef std::function<bool(uint32_t, std::unordered_set<uint64_t>&)> evict_set_creator_func_t;
typedef std::function<void(uint32_t, std::unordered_set<uint64_t>&)> obtain_evict_set_func_t;

extern bool
targeted_evict_set_creator(
                           uint32_t num,                  // number of lines to be generated
                           std::unordered_set<uint64_t>& candidate, // the generated candidates
                           L1CacheBase * cache,           // the L1 cache that can be accessed
                           uint64_t target,               // the target address to be evicted
                           uint32_t cycle,                // number of probe cycles
                           uint32_t write,                // number of write cycles in each probe
                           uint32_t threshold,            // threshold for successfull eviction
                           hit_func_t hit                 // hit function
                           );

extern bool
obtain_targeted_evict_set(
                          uint32_t num,                  // number of lines to be generated
                          std::unordered_set<uint64_t>& candidate, // the generated candidates
                          L1CacheBase * cache,           // the L1 cache that can be accessed
                          uint64_t target,               // the target address to be evicted
                          uint32_t cycle,                // number of probe cycles
                          uint32_t write,                // number of write cycles in each probe
                          uint32_t threshold,            // threshold for successfull eviction
                          hit_func_t hit,                // hit function
                          uint32_t trial                 // the maximal number of trials
                          );

#endif
