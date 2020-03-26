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

#endif
