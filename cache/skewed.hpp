#ifndef CM_SKEWED_HPP_
#define CM_SKEWED_HPP_

#include "cache/cache.hpp"
#include "util/query.hpp"
#include "util/random.hpp"

/////////////////////////////////
// skewed cache

class SkewedCache : public CacheBase
{
  const uint32_t partition;
public:
  // treat N partition skewed cache as a s*N set w/N way cache
  SkewedCache(uint32_t nset, uint32_t nway,
              indexer_creator_t ic,
              tagger_creator_t tc,
              replacer_creator_t rc,
              uint32_t level, int32_t core_id, int32_t cache_id,
              uint32_t partition,
              uint32_t delay)
    : CacheBase(nset*partition, nway/partition, ic, tc, rc, level, core_id, cache_id, delay),
      partition(partition) {}

  virtual ~SkewedCache() {}

  virtual int32_t get_index(uint64_t *latency, uint64_t addr) {
    int32_t skew_idx = (uint32_t)get_random_uint64(partition);
    return indexer->index(latency, addr, skew_idx);
  }

  virtual bool hit(uint64_t *latency, uint64_t addr, int32_t *idx, uint32_t *way) {
    for(int si=0; si<partition; si++) {
      *idx = indexer->index(latency, addr, si);
      for(int i=0; i<nway; i++) {
        uint64_t meta = get_meta(NULL, *idx, i);
        if(tagger->match(meta, addr) && !CM::is_invalid(meta)) {
          *way = i;
          return true;
        }
      }
    }
    return false;
  }

  virtual LocInfo query_loc(uint64_t addr) {
    LocInfo rv(level, core_id, cache_id, this);
    for(int i=0; i<partition; i++)
      rv.insert(indexer->index(NULL, addr, i), LocRange(0, nway-1));
    return rv;
  }

  virtual bool query_coloc(uint64_t addrA, uint64_t addrB){
    std::unordered_set<uint32_t> idxA, idxB;
    for(int i=0; i<partition; i++) {
      idxA.insert(indexer->index(NULL, addrA, i));
      idxB.insert(indexer->index(NULL, addrB, i));
    }
    for(auto idx: idxA) {
      if(idxB.count(idx)) return true;
    }
    return false;
  }

  static CacheBase *factory(uint32_t nset, uint32_t nway,
                            indexer_creator_t ic,
                            tagger_creator_t tc,
                            replacer_creator_t rc,
                            uint32_t level,
                            int32_t core_id,
                            int32_t cache_id,
                            uint32_t partition,
                            uint32_t delay) {
    return (CacheBase *)(new SkewedCache(nset, nway, ic, tc, rc, level, core_id, cache_id, partition, delay));
  }

  static cache_creator_t gen(uint32_t nset, uint32_t nway,
                             indexer_creator_t ic,
                             tagger_creator_t tc,
                             replacer_creator_t rc,
                             uint32_t partition,
                             uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, nset, nway, ic, tc, rc, _1, _2, _3, partition, delay);
  }
};

#endif
