#include "test/common.hpp"
#include <cmath>

int main(int argc, char* argv[]) {
  if(argc <= 6) {
    //                        0             1                 2          3         4         5      6-n
    std::cerr << "Usage: evset-attack <cache-config> <traverse-cfg> cache-level max-evict tests evsizes[0..n]" << std::endl;
    return 1;
  }

  if(!cache_config_parser("config/cache.json", argv[1], &ccfg)) return 1;
  traverse_func_t traverse_func = traverse_config_parser("config/traverse.json", argv[2], &tcfg);

  uint32_t cache_level    = std::stoi(std::string(argv[3]));
  uint32_t ccfg_level     = cache_level - 1;
  uint64_t period         = std::stoi(std::string(argv[4]));
  uint32_t max_tests      = std::stoi(std::string(argv[5]));

  std::vector<size_t> evset_sizes(argc-6);
  std::vector<uint32_t> stat_handlers(argc-6);

  int stat_n = argc-6;
  
  for(int i=0; i<stat_n; i++) {
    evset_sizes[i] = std::stoi(std::string(argv[6+i]));
    stat_handlers[i] = init_mean_stat();
  }

  cache_init();

  L1CacheBase *entry = (L1CacheBase *)l1_caches[0];

  reporter.register_cache_access_tracer(2);

  for(int test=0; test<max_tests; test++) {
    std::unordered_set<uint64_t> evset;
    uint64_t target = get_random_uint64(1ull << 60);
    CacheBase *target_cache = get_target_cache(target, entry, 2).cache;
    hit = std::bind(query_hit, std::placeholders::_1, target_cache);
    check = std::bind(query_check, target, target_cache, std::placeholders::_1);
    uint64_t evict_pre = reporter.check_cache_evict(2);
    entry->read(target);
    do {
      uint64_t c = CM::normalize(get_random_uint64(1ull << 60));
      entry->read(c);
      bool h=hit(target);
      entry->read(target);
      if(!h) evset.insert(c);
    } while(reporter.check_cache_evict(2) - evict_pre < period);

    for(int i=0; i<stat_n; i++)
      record_mean_stat(stat_handlers[i], evset.size() >= evset_sizes[i] ? 1.0 : 0.0);
  }

  std::cout << period;
  for(auto stat:stat_handlers) {
    std::cout << "," << get_mean_mean(stat);
    close_mean_stat(stat);
  }
  std::cout << std::endl;

  cache_release();
  return 0;
}
