#include "test/common.hpp"

int main(int argc, char* argv[]) {
  if(argc != 2) {
    std::cerr << "Usage: cache-test <cache-config>" << std::endl;
    return 1;
  }

  if(!cache_config_parser("config/cache.json", argv[1], &ccfg)) return 1;
  cache_init();
  L1CacheBase *entry = (L1CacheBase *)l1_caches[0];

  for(int i=0; i<1000; i++) {
    uint64_t addr = i*64;
    reporter.register_address_tracer(addr);
    entry->read(addr);
  }
  
  for(int i=1000; i>0; i--) {
    uint64_t addr = 64*i;
    entry->read(addr);
  }

  cache_release();
  return 0;
}
