#include "attack/traverse.hpp"
#include "cache/cache.hpp"

void strategy_traverse_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset,
                              uint32_t window, uint32_t repeat, uint32_t step)
{
  auto first = evset.cbegin();
  auto last = std::next(first, window);
  while(first != evset.cend()) {
    for(auto it=first; it!=last; it++)
      for(int i=0; i<repeat; i++)
        cache->read(*it);
    std::advance(first, step);
    std::advance(last, step);
  }
}

traverse_func_t strategy_traverse(uint32_t window, uint32_t repeat, uint32_t step)
{
  return std::bind(strategy_traverse_kernel, std::placeholders::_1, std::placeholders::_2,
                   window, repeat, step);
}

traverse_func_t list_traverse(uint32_t window, uint32_t repeat)
{
  return strategy_traverse(window, repeat, 1);
}

void round_traverse_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset,
                           uint32_t repeat)
{
  for(auto it=evset.cbegin(); it!=evset.cend(); it++)
    for(int i=0; i<repeat; i++)
      cache->read(*it);
  for(auto it=evset.crbegin(); it!=evset.crend(); it++)
    for(int i=0; i<repeat; i++)
      cache->read(*it);
}

traverse_func_t round_traverse(uint32_t repeat)
{
  return std::bind(round_traverse_kernel, std::placeholders::_1, std::placeholders::_2,
                   repeat);
}

uint32_t traverse_test_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset, uint64_t target,
                          traverse_func_t traverse, hit_func_t hit,
                          uint32_t ntests, uint32_t ntraverse)
{
  uint32_t success = 0;
  for(int i=0; i<ntests; i++) {
    cache->read(target);
    for(int j=0; j<ntraverse; j++) traverse(cache, evset);
    if(!hit(target)) success++;
  }
  return success;
}

float traverse_test_ratio_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset, uint64_t target,
                                 traverse_func_t traverse, hit_func_t hit,
                                 uint32_t ntests, uint32_t ntraverse)
{
  return traverse_test_kernel(cache, evset, target, traverse, hit, ntests, ntraverse)/((float)(ntests));
}

bool traverse_test_bool_kernel(L1CacheBase *cache, const std::list<uint64_t>& evset, uint64_t target,
                               traverse_func_t traverse, hit_func_t hit,
                               uint32_t ntests, uint32_t ntraverse, uint32_t threshold)
{
  return traverse_test_kernel(cache, evset, target, traverse, hit, ntests, ntraverse) > threshold;
}

traverse_test_ratio_t traverse_test_ratio(traverse_func_t traverse, hit_func_t hit,
                                          uint32_t ntests, uint32_t ntraverse)
{
  return std::bind(traverse_test_ratio_kernel,
                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                   traverse, hit, ntests, ntraverse);
}

traverse_test_t traverse_test(traverse_func_t traverse, hit_func_t hit,
                              uint32_t ntests, uint32_t ntraverse, uint32_t threshold)
{
  return std::bind(traverse_test_bool_kernel,
                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                   traverse, hit, ntests, ntraverse, threshold);
}
