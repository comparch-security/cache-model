#ifndef CM_TEST_COMMON__HPP_
#define CM_TEST_COMMON__HPP_

#include "cache/skewed.hpp"
#include "attack/create.hpp"
#include "attack/search.hpp"
#include "util/report.hpp"
#include "util/query.hpp"
#include "util/statistics.hpp"
#include "util/cache_config_parser.hpp"
#include "util/traverse_config_parser.hpp"
#include "datagen/include/random_generator.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <boost/format.hpp>

Reporter_t reporter;
CacheCFG ccfg;
TraverseTestCFG tcfg;

typedef std::vector<CoherentCache *> cache_vector_t;
cache_vector_t l1_caches;
std::list<cache_vector_t *> all_caches;

hit_func_t hit;
check_func_t check;
traverse_test_t traverse;

void cache_init() {
  for(int level=0; level < MAX_CACHE_LEVEL; level++)
    if(ccfg.enable[level]) all_caches.push_back(new cache_vector_t(ccfg.number[level]));

  auto it = all_caches.begin();
  int level = 0;
  while(it != all_caches.end()) {
    bool is_l1  = level == 0;
    bool is_llc = (level <= MAX_CACHE_LEVEL - 1) && (ccfg.enable[level+1] == false);
    auto it_prev = it; if(!is_l1)  --it_prev;
    auto it_next = it; if(!is_llc) ++it_next;
    cache_vector_t *level_prev = is_l1  ? NULL : *it_prev;
    cache_vector_t *level_next = is_llc ? NULL : *it_next;

    if(level == 0) // L1 cache
      for(int i=0; i < (*it)->size(); i++)
        (**it)[i] = new L1CacheBase(i, i, 0, ccfg.cache_gen[level],  is_llc ? NULL : level_next, ccfg.hash_gen[level]);
    else if(is_llc)
      for(int i=0; i < (*it)->size(); i++)
        (**it)[i] = new LLCCacheBase(i, level+1, ccfg.cache_gen[level], level_prev);
    else
      for(int i=0; i < (*it)->size(); i++)
        (**it)[i] = new CoherentCache(i, level+1, -1, i, ccfg.cache_gen[level], level_prev, level_next, ccfg.hash_gen[level]);

    it++;
    level++;
  }

  l1_caches = *(all_caches.front());
  random_seed_gen64();
}

void cache_release() {
  for(auto& l : all_caches) {
    for(auto& c : *l) delete c;
    delete l;
  }
}

LocInfo get_target_cache(uint64_t addr, L1CacheBase *cache, uint32_t level, bool print = false) {
  std::list<LocInfo> locs;
  cache->query_loc(addr, &locs);
  if(print) print_locs(locs, 2);
  auto it = locs.begin();
  for(int i=1; i<level; i++) it++;
  return *it;
}

void set_hit_check_func(uint64_t addr, L1CacheBase *cache, uint32_t level, traverse_func_t traverse_func) {
  CacheBase *c = get_target_cache(addr, cache, level).cache;
  hit = std::bind(query_hit, std::placeholders::_1, c);
  check = std::bind(query_check, addr, c, std::placeholders::_1);
  traverse = traverse_test(traverse_func, hit, tcfg.ntests, tcfg.ntraverse, tcfg.threshold);
}

#endif
