#ifndef CM_TEST_COMMON__HPP_
#define CM_TEST_COMMON__HPP_

#include "cache/cache.hpp"
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

std::vector<CoherentCache *>  l1_caches;
std::vector<CoherentCache *>  l2_caches;

hit_func_t hit;
check_func_t check;
traverse_test_t traverse;

void cache_init() {
  l1_caches.resize(ccfg.number[0]);
  if(ccfg.enable[1]) l2_caches.resize(ccfg.number[1]);

  for(int i=0; i<ccfg.number[0]; i++)
    l1_caches[i] = new L1CacheBase(i, i, 0, ccfg.cache_gen[0], ccfg.enable[1] ? &l2_caches : NULL,
                                   ccfg.hash_gen[0], ccfg.number[1]);

  if(ccfg.enable[1]) {
    for(int i=0; i<ccfg.number[1]; i++)
      l2_caches[i] = new LLCCacheBase(i, ccfg.cache_gen[1], &l1_caches);
  }

  random_seed_gen64();
}

void cache_release() {
  for(int i=0; i<ccfg.number[0]; i++) delete l1_caches[i];
  if(ccfg.enable[1]) for(int i=0; i<ccfg.number[1]; i++) delete l2_caches[i];
}

void set_hit_check_func(uint64_t addr, L1CacheBase *cache, uint32_t level, traverse_func_t traverse_func) {
  std::list<LocInfo> locs;
  cache->query_loc(addr, &locs);
  auto it = locs.begin();
  for(int i=1; i<level; i++) it++;
  CacheBase *c = it->cache;
  hit = std::bind(query_hit, std::placeholders::_1, c);
  check = std::bind(query_check, addr, c, std::placeholders::_1);
  traverse = traverse_test(traverse_func, hit, tcfg.ntests, tcfg.ntraverse, tcfg.threshold);
}

#endif
