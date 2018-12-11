#ifndef ATT_ALGORITHM_HPP_
#define ATT_ALGORITHM_HPP_

#include "attack/search.hpp"

extern bool
targeted_trim_original(
                       L1CacheBase * cache,             // the L1 cache that can be accessed
                       uint64_t target,                 // the target address to be evicted
                       std::set<uint64_t> &candidate,   // potential lines for the eviction set
                       hit_func_t hit                   // hit function
                       );


#endif
