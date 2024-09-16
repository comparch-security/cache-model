#include "attack/create.hpp"
#include "cache/cache.hpp"
#include "util/query.hpp"
#include "util/random.hpp"

bool obtain_targeted_evict_set(
                               uint32_t num,
                               std::list<uint64_t>& candidate,
                               L1CacheBase * cache,
                               uint64_t target,
                               traverse_test_t traverse,
                               uint32_t trial
                               )
{
  for(int i=0; trial==0 || i<trial; i++) {
    candidate.clear();
    get_random_list(candidate, num, 1ull << 60);
    if(traverse(cache, candidate, target))
      return true;
  }
  return false;
}

bool
produce_targeted_evict_set(
                           uint32_t candidate_size,
                           std::list<uint64_t>& evset_rv,
                           uint32_t evset_size,
                           L1CacheBase * cache,
                           uint32_t level,
                           uint64_t target
                           )
{
  std::list<LocInfo> locs;
  cache->query_loc(target, &locs);
  auto it = locs.begin();
  for(int i=1; i<level; i++) it++;
  CacheBase *c = it->cache;

  std::unordered_set<uint64_t> candidate, evset;
  get_random_set64(candidate, candidate_size, 1ull << 60);
  for(auto addr : candidate) {
    if(c->query_coloc(CM::normalize(target), CM::normalize(addr)))
      evset.insert(addr);
    if(evset.size() == evset_size) break;
  }
  if(evset.size() == evset_size) {
    evset_rv.insert(evset_rv.end(), evset.begin(), evset.end());
    return true;
  } else
    return false;
}

uint32_t
obtain_cache_prime_set(
                       uint32_t num,
                       std::list<uint64_t>& prime_set,
                       L1CacheBase * cache,
                       hit_func_t hit
                       )
{
  get_random_list(prime_set, num, 1ull << 60);

  size_t ssize;
  uint64_t pass = 0;
  do {
    ssize = prime_set.size();
    uint32_t miss = 0;
    for(auto it=prime_set.rbegin(); it != prime_set.rend(); it++) {
      if(!hit(*it)) miss++;
      cache->read(*it);
    }
 
    miss = 0;
    for(auto it=prime_set.begin(); it != prime_set.end();) {
      bool h = hit(*it);
      cache->read(*it);
      if(!h) {
        miss++;
        it = prime_set.erase(it);
      } else it++;
    }
    pass++;
  } while(ssize != prime_set.size() && pass < 20);

  if(ssize == prime_set.size()) {
    //std::cout << "prime size: " << ssize << std::endl;
    return pass;
  }
  else
    return 0;
}
