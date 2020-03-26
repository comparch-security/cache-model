#ifndef ATT_SEARCH_HPP_
#define ATT_SEARCH_HPP_

#include "attack/traverse.hpp"

class L1CacheBase;

typedef std::function<bool(std::list<uint64_t>&)> check_func_t;

extern void
split_random_set
(
 std::list<uint64_t> &candidate,    // potential lines for the eviction unordered_set
 std::list<uint64_t> &picked_set,   // the lines being picked
 uint32_t pick                      // number of lines to be picked
 );

extern bool
targeted_evict_random_pick
(
 L1CacheBase * cache,               // the L1 cache that can be accessed
 uint64_t target,                   // the target address to be evicted
 std::list<uint64_t> &candidate,    // potential lines for the eviction set
 std::list<uint64_t> &picked_set,   // the lines being picked
 std::list<uint64_t> &evict_set,    // confirmed lines in the eviction set
 traverse_test_t traverse,          // traverse function
 uint32_t pick                      // the number of lines to be picked
 );

extern bool
targeted_trim_original
(
 L1CacheBase * cache,               // the L1 cache that can be accessed
 uint64_t target,                   // the target address to be evicted
 std::list<uint64_t> &candidate,    // potential lines for the eviction set
 traverse_test_t traverse,          // traverse function
 check_func_t check                 // eviction set check function
 );

extern bool
targeted_trim_divide
(
 L1CacheBase * cache,               // the L1 cache that can be accessed
 uint64_t target,                   // the target address to be evicted
 std::list<uint64_t> &candidate,    // potential lines for the eviction set
 traverse_test_t traverse,          // traverse function
 check_func_t check,                // eviction set check function
 uint32_t split                     // number of split in each pass
 );

extern bool
targeted_trim_divide_random
(
 L1CacheBase * cache,               // the L1 cache that can be accessed
 uint64_t target,                   // the target address to be evicted
 std::list<uint64_t> &candidate,    // potential lines for the eviction set
 traverse_test_t traverse,          // traverse function
 check_func_t check,                // eviction set check function
 uint32_t split                     // number of split in each pass
 );

#endif
