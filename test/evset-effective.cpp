#include "test/common.hpp"
#include <cmath>

int main(int argc, char* argv[]) {
  if(argc > 8 || argc < 7) {
    //                            0            1                 2          3           4              5       6      7
    std::cerr << "Usage: evset-effective <cache-config> <traverse-cfg> cache-level evset-size candidate-size tests [mode]" << std::endl;
    return 1;
  }

  if(!cache_config_parser("config/cache.json", argv[1], &ccfg)) return 1;
  traverse_func_t traverse_func = traverse_config_parser("config/traverse.json", argv[2], &tcfg);

  uint32_t cache_level    = std::stoi(std::string(argv[3]));
  uint32_t ccfg_level     = cache_level - 1;
  uint32_t evset_size     = std::stoi(std::string(argv[4]));
  uint32_t candidate_size = std::stoi(std::string(argv[5]));
  uint32_t max_tests      = std::stoi(std::string(argv[6]));
  int      mode           = 0;
  if(argc == 8) {
    mode = std::stoi(std::string(argv[7]));
    if(mode < 0 || mode > 2) {
      std::cerr << "Available mode: 0, report; 1, debug; 2, output." << std::endl;
      return 1;
    }
  }

  cache_init();

  L1CacheBase *entry = (L1CacheBase *)l1_caches[0];

  uint32_t evict_mean = init_mean_stat();

  for(int i=0; i<max_tests; i++) {
    uint32_t trials = 1;
    std::list<uint64_t> evset;
    uint64_t target = get_random_uint64(1ull << 60);
    set_hit_check_func(target, entry, cache_level, traverse_func);
    while(!produce_targeted_evict_set(candidate_size, evset, evset_size, entry, cache_level, target)) {}
    entry->read(target);
    traverse_func(entry, evset);
    bool succ = hit(CM::normalize(target));
    record_mean_stat(evict_mean, succ ? 0 : 1);
    if(mode == 1)
      std::cout << boost::format("%1% %2% %3%") % i % succ % evset.size() << std::endl;
  }

  double rate = get_mean_mean(evict_mean);
  
  if(mode == 0 || mode == 1) {
    std::cout << evset_size << "\t"
              << boost::format("%.3f   ") % rate
              << boost::format("%.4f   ") % get_mean_variance(evict_mean)
              << max_tests << std::endl;
  } else if(mode == 2) {
    std::cout << rate;
  }

  close_mean_stat(evict_mean);
  cache_release();
  return 0;
}
