#ifndef ATT_TRAVERSE_HPP_
#define ATT_TRAVERSE_HPP_

#include <cstdint>
#include <list>
#include <functional>

class L1CacheBase;

typedef std::function<void(L1CacheBase *, const std::list<uint64_t>&)> traverse_func_t;

void strategy_traverse_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset,
                              uint32_t window, uint32_t repeat, uint32_t step);

extern traverse_func_t strategy_traverse(uint32_t window, uint32_t repeat, uint32_t step);
extern traverse_func_t list_traverse(uint32_t window, uint32_t repeat);

void round_traverse_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset,
                           uint32_t repeat);

extern traverse_func_t round_traverse(uint32_t repeat);

typedef std::function<bool(uint64_t)> hit_func_t;


uint32_t traverse_test_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset, uint64_t target,
                              traverse_func_t traverse, hit_func_t hit,
                              uint32_t ntests, uint32_t ntraverse);

float traverse_test_ratio_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset, uint64_t target,
                                 traverse_func_t traverse, hit_func_t hit,
                                 uint32_t ntests, uint32_t ntraverse);

bool traverse_test_bool_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset, uint64_t target,
                                 traverse_func_t traverse, hit_func_t hit,
                                 uint32_t ntests, uint32_t ntraverse, uint32_t threshold);

typedef std::function<float(L1CacheBase *, const std::list<uint64_t>&, uint64_t)> traverse_test_ratio_t;
typedef std::function<bool(L1CacheBase *, const std::list<uint64_t>&, uint64_t) > traverse_test_t;

extern traverse_test_ratio_t traverse_test_ratio(traverse_func_t traverse, hit_func_t hit,
                                                 uint32_t ntests, uint32_t ntraverse);
extern traverse_test_t traverse_test(traverse_func_t traverse, hit_func_t hit,
                                     uint32_t ntests, uint32_t ntraverse, uint32_t threshold);

#endif
