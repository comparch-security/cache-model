#ifndef ATT_ALGORITHM_HPP_
#define ATT_ALGORITHM_HPP_

#include "attack/search.hpp"

extern bool
targeted_trim_original(
                       L1CacheBase * cache,             // the L1 cache that can be accessed
                       uint64_t target,                 // the target address to be evicted
                       std::unordered_set<uint64_t> &candidate,   // potential lines for the eviction set
                       uint32_t cycle,                  // number of probe cycles
                       uint32_t write,                  // number of write cycles in each probe
                       uint32_t threshold,              // threshold for successfull eviction
                       hit_func_t hit,                  // hit function
                       check_func_t check,              // eviction set check function
                       uint32_t nway                    // number of ways
                       );


#endif
