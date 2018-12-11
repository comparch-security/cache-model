#ifndef ATT_SEARCH_HPP_
#define ATT_SEARCH_HPP_

#include <cstdint>
#include <set>
#include <functional>

class L1CacheBase;

typedef std::function<bool(uint64_t)> hit_func_t;
typedef std::function<bool(std::set<uint64_t>&,uint64_t)> check_func_t;
typedef std::function<bool()> evict_func_t;

extern bool l1_hit(L1CacheBase *, uint64_t addr);
extern bool llc_hit(uint64_t addr);
extern bool l1_evict(uint32_t id);
extern bool llc_evict();

extern bool
targeted_evict_probe(
                     L1CacheBase * cache,             // the L1 cache that can be accessed
                     uint64_t target,                 // the target address to be evicted
                     std::set<uint64_t> &candidate,   // potential lines for the eviction set
                     std::set<uint64_t> &evict_set,   // confirmed lines in the eviction set
                     hit_func_t hit                   // hit confirmation function
                     );

extern bool
targeted_evict_random_pick(
                           L1CacheBase * cache,             // the L1 cache that can be accessed
                           uint64_t target,                 // the target address to be evicted
                           std::set<uint64_t> &candidate,   // potential lines for the eviction set
                           std::set<uint64_t> &picked_set,  // the lines being picked
                           std::set<uint64_t> &evict_set,   // confirmed lines in the eviction set
                           hit_func_t hit,                  // hit function
                           uint32_t pick                    // the number of lines to be picked
                           );

typedef std::function<bool(const std::set<uint64_t> &)> evict_check_t;

extern bool l1_evict_check(L1CacheBase *, const std::set<uint64_t> &);
extern bool l1_targeted_evict_check(uint64_t target, L1CacheBase *, const std::set<uint64_t> &);
extern bool llc_evict_check(const std::set<uint64_t> &);
extern bool llc_targeted_evict_check(uint64_t target, const std::set<uint64_t> &);

#endif
