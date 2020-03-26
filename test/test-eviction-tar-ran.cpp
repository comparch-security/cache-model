#include "test/common.hpp"

int main(int argc, char* argv[]) {
  if(argc != 7) {
    for(int i=0; i<argc; i++) std::cout << argv[i] << " ";
    std::cout << std::endl;
    std::cout << "test_eviction  <cache-config> <traverse-cfg> target-cache-level candidate-size split total-tests" << std::endl;
    return 0;
  }

  int cache_level = std::stoi(std::string(argv[3]));
  int candidate_size = std::stoi(std::string(argv[4]));
  int splitN = std::stoi(std::string(argv[5]));
  uint32_t testN = std::stoi(std::string(argv[6]));

  if(!cache_config_parser("config/cache.json", argv[1], &ccfg)) return 1;
  traverse_func_t traverse_func = traverse_config_parser("config/traverse.json", argv[2], &tcfg);

  cache_init();

  uint32_t stat_mean_evict = init_mean_stat();
  uint32_t stat_mean_full = init_mean_stat();

  std::list<uint64_t> candidate;
  L1CacheBase *entry = (L1CacheBase *)l1_caches[0];

  for(uint32_t t=0; t<testN; t++) {
    uint64_t target = get_random_uint64(1ull << 60);
    candidate.clear();
    reporter.clear();
    if(cache_level == 1) reporter.register_cache_access_tracer(1, 0, 0);
    else                 reporter.register_cache_access_tracer(2);

    set_hit_check_func(target, entry, cache_level, traverse_func);

    if(!obtain_targeted_evict_set(candidate_size, candidate, entry, target, traverse, 1000))
      continue;
    
    double creation_access = (cache_level == 1) ?
      (double)(reporter.check_cache_access(1, 0, 0)) :
      (double)(reporter.check_cache_access(2)) ;

    if(!targeted_trim_divide_random(entry, target, candidate, traverse, check, splitN))
      continue;

    double evict_access = (cache_level == 1) ?
      (double)(reporter.check_cache_access(1, 0, 0)) :
      (double)(reporter.check_cache_access(2)) ;

    record_mean_stat(stat_mean_evict, evict_access - creation_access);
    record_mean_stat(stat_mean_full,  evict_access);
  }

  std::cout << candidate_size << "\t"
            << testN << "\t"
            << get_mean_count(stat_mean_full) << "\t"
            << get_mean_mean(stat_mean_full) << "\t"
            << get_mean_error(stat_mean_full) << "\t"
            << get_mean_mean(stat_mean_evict) << "\t"
            << get_mean_error(stat_mean_evict) << std::endl;

  close_mean_stat(stat_mean_full);
  close_mean_stat(stat_mean_evict);
  cache_release();
  return 0;
}
