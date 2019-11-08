#ifndef ATT_SEARCH_HPP_
#define ATT_SEARCH_HPP_

#include <cstdint>
#include <unordered_set>
#include <functional>

class L1CacheBase;

typedef std::function<bool(uint64_t)> hit_func_t;
typedef std::function<bool(std::unordered_set<uint64_t>&)> check_func_t;

extern void
split_random_set(
                 std::unordered_set<uint64_t> &candidate,   // potential lines for the eviction unordered_set
                 std::unordered_set<uint64_t> &picked_set,  // the lines being picked
                 uint32_t pick                    // number of lines to be picked
                 );

extern bool
targeted_evict_probe(
                     L1CacheBase * cache,             // the L1 cache that can be accessed
                     uint64_t target,                 // the target address to be evicted
                     std::unordered_set<uint64_t> &candidate,   // potential lines for the eviction set
                     std::unordered_set<uint64_t> &evict_set,   // confirmed lines in the eviction set
                     uint32_t cycle,                  // number of probe cycles
                     uint32_t write,                  // number of write cycles in each probe
                     uint32_t threshold,              // threshold for successfull eviction
                     hit_func_t hit                   // hit confirmation function
                     );

extern bool
targeted_evict_random_pick(
                           L1CacheBase * cache,             // the L1 cache that can be accessed
                           uint64_t target,                 // the target address to be evicted
                           std::unordered_set<uint64_t> &candidate,   // potential lines for the eviction set
                           std::unordered_set<uint64_t> &picked_set,  // the lines being picked
                           std::unordered_set<uint64_t> &evict_set,   // confirmed lines in the eviction set
                           uint32_t cycle,                  // number of probe cycles
                           uint32_t write,                  // number of write cycles in each probe
                           uint32_t threshold,              // threshold for successfull eviction
                           hit_func_t hit,                  // hit function
                           uint32_t pick                    // the number of lines to be picked
                           );

typedef std::function<bool(const std::unordered_set<uint64_t> &)> evict_check_t;

#endif
