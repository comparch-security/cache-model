#ifndef ATT_CREATE_HPP_
#define ATT_CREATE_HPP_

#include "attack/traverse.hpp"

extern bool
obtain_targeted_evict_set
(
 uint32_t num,                      // number of lines to be generated
 std::list<uint64_t>& candidate,    // the generated candidates
 L1CacheBase * cache,               // the L1 cache that can be accessed
 uint64_t target,                   // the target address to be evicted
 traverse_test_t traverse,          // traverse function
 uint32_t trial                     // the maximal number of trials
 );

extern bool
produce_targeted_evict_set
(
 uint32_t candidate_size,           // number of lines in the candidate set
 std::list<uint64_t>& evset,        // the generated eviction set
 uint32_t evset_size,               // expected size of the eviction set
 L1CacheBase * cache,               // the L1 cache that can be accessed
 uint32_t level,                    // the level of cache to be targeted
 uint64_t target                    // the target address to be evicted
 );

extern uint32_t
obtain_cache_prime_set
(
 uint32_t num,                      // number of lines to begin with
 std::list<uint64_t>& prime_set,    // the obtained prime set
 L1CacheBase * cache,               // the L1 cache that can be accessed
 hit_func_t hit                     // a hit check function
 );

#endif
