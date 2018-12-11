#include "attack/search.hpp"
#include "attack/create.hpp"
#include "cache/cache.hpp"

LLCCacheBase *obtain_llc(uint64_t addr) {
  return llc_caches[llc_hasher->hash(addr)];
}

bool l1_hit(L1CacheBase * cache, uint64_t addr) {
  return cache->hit(addr) != 0;
}

bool llc_hit(uint64_t addr) {
  return obtain_llc(addr)->hit(addr) != 0;
}

bool l1_evict(uint32_t id) {
  return REPORT::get_l1_evicted(id) >= NL1Way;
}

bool llc_evict() {
  return REPORT::get_llc_evicted() >= NLLCWay;
}

const uint32_t NTrial = 16;

bool targeted_evict_probe(
                          L1CacheBase * cache,
                          uint64_t target,
                          std::set<uint64_t> &candidate,
                          std::set<uint64_t> &evict_set,
                          hit_func_t hit
                          )
{
  uint32_t count = 0;
  for(uint32_t i=0; i<ProbeCycle; i++) {
    cache->read(target);
    for(uint32_t j=0; j<ProbeWrite; j++) {
      for(auto line: candidate) cache->read(line);
      for(auto line: evict_set) cache->read(line);
    }
    if(!hit(target)) {
      count++;
    }
  }
  return count > ProbeThreshold;
}

bool targeted_evict_random_pick(
                                L1CacheBase * cache,
                                uint64_t target,
                                std::set<uint64_t> &candidate,
                                std::set<uint64_t> &picked_set,
                                std::set<uint64_t> &evict_set,
                                hit_func_t hit,
                                uint32_t pick
                                )
{
  assert(candidate.size() >= pick);
  std::set<uint32_t> picked_index;
  get_random_set(picked_index, pick, candidate.size());
  uint32_t i = 0;
  for(auto it=candidate.begin(); it != candidate.end(); ++i, ++it)
    if(picked_index.count(i)) picked_set.insert(*it);
  for(auto it:picked_set) candidate.erase(it);
  return targeted_evict_probe(cache, target, candidate, evict_set, hit);
}

bool l1_evict_check(L1CacheBase *cache, const std::set<uint64_t> &evict_set) {
  assert(evict_set.size() >= NL1Way + 1);
  auto it = evict_set.begin();
  auto target = *it++;
  std::set<uint64_t> m_evict_set(it, evict_set.end());
  return l1_targeted_evict_check(target, cache, m_evict_set);
}

bool l1_targeted_evict_check(uint64_t target, L1CacheBase *cache, const std::set<uint64_t> &evict_set) {
  auto idx = cache->get_index(target);
  for(auto l:evict_set) if(cache->get_index(l) != idx) return false;
  return true;
}

bool llc_evict_check(const std::set<uint64_t> &evict_set) {
  assert(evict_set.size() >= NL1Way + 1);
  auto it = evict_set.begin();
  auto target = *it++;
  std::set<uint64_t> m_evict_set(it, evict_set.end());
  return llc_targeted_evict_check(target, m_evict_set);
}

bool llc_targeted_evict_check(uint64_t target, const std::set<uint64_t> &evict_set) {
  auto cache = obtain_llc(target);
  auto idx = cache->get_index(target);
  for(auto l:evict_set) if(obtain_llc(l) != cache || cache->get_index(l) != idx) return false;
  return true;
}
