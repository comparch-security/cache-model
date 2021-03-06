#ifndef CM_CACHE_HPP_
#define CM_CACHE_HPP_

#include <cstdlib>
#include <cstring>
#include <string>
#include "util/delay.hpp"
#include "cache/definitions.hpp"
#include "cache/replace.hpp"
#include "cache/index.hpp"
#include "cache/tag.hpp"
#include "cache/llchash.hpp"

/////////////////////////////////
// Base class for all caches

class CacheBase : public DelaySim
{
protected:
  IndexFuncBase   *indexer;     // generic index function
  TagFuncBase     *tagger;      // generic tag function
  ReplaceFuncBase *replacer;    // generic replace function
  uint64_t *meta;      // metadata array
  friend CoherentCache;

public:
  uint32_t level;      // cache level (L1, L2, L3)
  int32_t core_id;     // when private, record the core id, -1 means unified (LLC)
  uint32_t cache_id;   // record the cache id in a core or just cache id when unified (LLC)
  uint32_t nset, nway; // number of sets and ways

  CacheBase(uint32_t nset, uint32_t nway,
            indexer_creator_t ic,
            tagger_creator_t tc,
            replacer_creator_t rc,
            uint32_t level, int32_t core_id, uint32_t cache_id,
            uint32_t delay)
    : DelaySim(delay),
      indexer(ic(nset)), tagger(tc(nset)), replacer(rc(nset, nway)),
      level(level), core_id(core_id), cache_id(cache_id),
      nset(nset), nway(nway)
  {
    size_t s = sizeof(uint64_t)*nset*nway;
    meta = (uint64_t *)malloc(s); memset(meta, 0, s);
  }

  virtual ~CacheBase() {
    delete indexer;
    delete tagger;
    delete replacer;
    free(meta);
  }

  virtual uint32_t get_index(uint64_t *latency, uint64_t addr) {
    return indexer->index(latency, addr);
  }

  uint64_t get_meta(uint64_t *latency, uint32_t idx, uint32_t way) const {
    latency_acc(latency);
    return meta[nway * idx + way];
  }

  void set_meta(uint64_t *latency, uint32_t idx, uint32_t way, uint64_t m_meta) {
    latency_acc(latency);
    meta[nway * idx + way] = m_meta;
  }

  virtual bool hit(uint64_t addr) {
    uint32_t idx, way;
    return hit(NULL, addr, &idx, &way);
  }

  virtual bool hit(uint64_t *latency, uint64_t addr, uint32_t *idx, uint32_t *way) {
    *idx = get_index(latency, addr);
    for(unsigned int i=0; i<nway; i++) {
      uint64_t meta = get_meta(NULL, *idx, i);
      if(tagger->match(meta, addr) && !CM::is_invalid(meta)) {
        *way = i;
        return true;
      }
    }
    return false;
  }

  virtual uint32_t replace(uint64_t *latency, uint32_t idx) { return replacer->replace(latency, idx); }
  virtual void access(uint32_t idx, uint32_t way)  { replacer->access(idx, way);    }
  virtual void invalid(uint32_t idx, uint32_t way) { replacer->invalid(idx, way);   }

  std::string cache_name() const;
  virtual void query_block(uint32_t idx, uint32_t way, CBInfo *info) const;
  virtual void query_set(uint32_t idx, SetInfo *info) const;
  virtual bool query_coloc(uint64_t addrA, uint64_t addrB);
  virtual LocInfo query_loc(uint64_t addr);

  static CacheBase *factory(uint32_t nset, uint32_t nway,
                            indexer_creator_t ic,
                            tagger_creator_t tc,
                            replacer_creator_t rc,
                            uint32_t level,
                            int32_t core_id,
                            uint32_t cache_id,
                            uint32_t delay
                            ) {
    return (CacheBase *)(new CacheBase(nset, nway, ic, tc, rc, level, core_id, cache_id, delay));
  }

  static cache_creator_t gen(uint32_t nset, uint32_t nway,
                             indexer_creator_t ic,
                             tagger_creator_t tc,
                             replacer_creator_t rc,
                             uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, nset, nway, ic, tc, rc, _1, _2, _3, delay);
  }
};

/////////////////////////////////
// Coherent cache base
class CoherentCache
{
protected:
  uint32_t id;
  CacheBase *cache;
  std::vector<CoherentCache *> *inner_caches;
  std::vector<CoherentCache *> *outer_caches;
  LLCHashBase *hasher;
public:
  CoherentCache(uint32_t id,
                uint32_t level,
                int32_t core_id,
                uint32_t cache_id,
                cache_creator_t cc,
                std::vector<CoherentCache *> *ic = NULL,
                std::vector<CoherentCache *> *oc = NULL,
                llc_hash_creator_t hc = LLCHashNorm::gen()
                )
    : id(id), cache(cc(level, core_id, cache_id)),
      inner_caches(ic), outer_caches(oc), hasher(hc(oc == NULL ? 0 : oc->size()))
  {}

  virtual ~CoherentCache() {
    delete cache;
    delete hasher;
  }

  std::string cache_name() const { return cache->cache_name(); }
  bool hit(uint64_t addr) { return cache->hit(addr); }
  uint32_t get_index(uint64_t addr) { return cache->get_index(NULL, addr); }

  virtual void read(uint64_t *latency, uint64_t addr, uint32_t inner_id);
  virtual void write(uint64_t *latency, uint64_t addr, uint32_t inner_id, bool to_dirty = false);
  virtual void release(uint64_t *latency, uint64_t addr, uint32_t inner_id);
  virtual void flush(uint64_t *latency, uint64_t addr, int32_t levels, uint32_t inner_id);
  virtual void flush_cache(uint64_t *latency, int32_t levels, uint32_t inner_id);
  virtual void probe(uint64_t *latency, uint64_t addr, bool invalid);
  virtual void query_block(uint32_t idx, uint32_t way, CBInfo *info) const {
    cache->query_block(idx, way, info);
  }
  virtual void query_set(uint32_t idx, SetInfo *info) const {
    cache->query_set(idx, info);
  }
  virtual bool query_hit(uint64_t addr) { return cache->hit(CM::normalize(addr)); }
  virtual void query_loc(uint64_t addr, std::list<LocInfo>* locs);

  virtual void inner_probe(uint64_t *latency, uint32_t inner_id, uint64_t addr, uint32_t outer_id, bool invalidate, bool all) {
    for (uint32_t i=0; i<inner_caches->size(); i++)
      if(inner_id != i || all) (*inner_caches)[i]->probe(latency, addr, invalidate);
  }

  virtual void outer_read(uint64_t *latency, uint32_t id, uint64_t addr) {
    (*outer_caches)[hasher->hash(addr)]->read(latency, addr, id);
  }
  virtual void outer_write(uint64_t *latency, uint32_t id, uint64_t addr) {
    (*outer_caches)[hasher->hash(addr)]->write(latency, addr, id);
  }
  virtual void outer_release(uint64_t *latency, uint32_t id, uint64_t addr) {
    (*outer_caches)[hasher->hash(addr)]->release(latency, addr, id);
  }
  virtual void outer_flush(uint64_t *latency, uint32_t id, uint64_t addr, int32_t levels) {
    (*outer_caches)[hasher->hash(addr)]->flush(latency, addr, id, levels);
  }
  virtual void outer_flush_cache(uint64_t *latency, uint32_t id, int32_t levels) {
    for(auto oc: (*outer_caches))
      oc->flush_cache(latency, id, levels);
  }

  virtual void outer_query_loc(uint64_t addr, std::list<LocInfo> *locs) {
    (*outer_caches)[hasher->hash(addr)]->query_loc(addr, locs);
  }

protected:
  virtual void replace(uint64_t *latency, uint64_t addr, uint32_t *idx, uint32_t *way);
  virtual void evict(uint64_t *latency, uint32_t idx, uint32_t way);
};

/////////////////////////////////
// Coherent L1 caches

class L1CacheBase : public CoherentCache
{
public:
  L1CacheBase(uint32_t id,
              int32_t core_id,
              uint32_t cache_id,
              cache_creator_t cc,
              std::vector<CoherentCache *> *oc = NULL,
              llc_hash_creator_t hc = LLCHashNorm::gen()
              )
    : CoherentCache(id, 1, core_id, cache_id, cc, NULL, oc, hc)
  {}

  virtual ~L1CacheBase() { }

  // otherwise the virtual methods got hidden
  using CoherentCache::read;
  using CoherentCache::write;
  using CoherentCache::flush;
  using CoherentCache::flush_cache;

  void read(uint64_t addr)  { read(NULL, addr, 0);  }
  void write(uint64_t addr) { write(NULL, addr, 0, true); }
  void flush(uint64_t addr) { flush(NULL, addr, -1, 0); }
  void flush_cache()        { flush_cache(NULL, 0, 0); } // flush L1 by default
  void read(uint64_t *latency, uint64_t addr)  { read(latency, addr, 0);  }
  void write(uint64_t *latency, uint64_t addr) { write(latency, addr, 0, true); }
  void flush(uint64_t *latency, uint64_t addr) { flush(latency, addr, -1, 0); }
  void flush_cache(uint64_t *latency)          { flush_cache(latency, 0, 0); } // flush L1 by default
};

/////////////////////////////////
// Coherent LLC caches

class LLCCacheBase : public CoherentCache
{
public:
  LLCCacheBase(uint32_t id,
               uint32_t level,
               cache_creator_t cc,
               std::vector<CoherentCache *> *ic)
    : CoherentCache(id, level, -1, id, cc, ic)
  {}

  virtual ~LLCCacheBase() { }
};

#endif
