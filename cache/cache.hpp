#ifndef CM_CACHE_HPP_
#define CM_CACHE_HPP_

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include "cache/definitions.hpp"
#include "cache/cache.hpp"
#include "cache/memory.hpp"
#include "cache/replace.hpp"
#include "cache/index.hpp"
#include "cache/tag.hpp"
#include "cache/llchash.hpp"
#include "util/report.hpp"

// debug related
//#define DPRINT

#ifdef DPRINT

#include <set>
#include <iostream>

extern std::set<uint64_t> l1_targets;
extern std::set<uint64_t> l2_targets;

#endif


/////////////////////////////////
// cache model functions

struct CM
{
  static uint64_t normalize(uint64_t addr) { return addr >> 6 << 6; }
  static bool is_invalid(uint64_t m)       { return (m&0x3) == 0; }
  static bool is_shared(uint64_t m)        { return (m&0x3) == 1; }
  static bool is_modified(uint64_t m)      { return (m&0x3) == 2; }
  static bool is_dirty(uint64_t m)         { return (m&0x4) == 0x4; }
  static uint64_t to_invalid(uint64_t m)   { return 0; }
  static uint64_t to_shared(uint64_t m)    { return (m >> 2 << 2) | 1; }
  static uint64_t to_modified(uint64_t m)  { return (m >> 2 << 2) | 2; }
  static uint64_t to_dirty(uint64_t m)     { return m | 0x4; }
};

/////////////////////////////////
// Base class for all caches

class CacheBase
{
protected:
  IndexFuncBase   *indexer;     // generic index function
  TagFuncBase     *tagger;      // generic tag function
  ReplaceFuncBase *replacer;    // generic replace function
  uint64_t *meta;      // metadata array
  uint32_t nset, nway; // number of sets and ways
  std::string cname;   // name of the cache

public:
  CacheBase(uint32_t nset, uint32_t nway,
            indexer_creator_t ic,
            tagger_creator_t tc,
            replacer_creator_t rc,
            std::string nm)
    : indexer(ic(nset)), tagger(tc(nset)), replacer(rc(nset, nway)),
      nset(nset), nway(nway), cname(nm)
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

  uint32_t get_index(uint64_t addr) const {
    return indexer->index(addr);
  }
  
  uint64_t get_meta(uint64_t addr, uint32_t way) const {
    return meta[nway * get_index(addr) + way];
  }

  void set_meta(uint64_t addr, uint32_t way, uint64_t m_meta) {
    meta[nway * get_index(addr) + way] = m_meta;
  }

  uint32_t hit(uint64_t addr) const {
    for(int i=0; i<nway; i++) {
      uint64_t meta = get_meta(addr, i);
      if(tagger->match(meta, addr) && !CM::is_invalid(meta))
        return (i<<1) | 1;
    }
    return 0;
  }

  // print the set related to an address
  void print_set(uint64_t addr) const;
  void print_line(uint64_t addr) const;

};

class L1CacheBase;
class LLCCacheBase;

/////////////////////////////////
// Coherent L1 caches

class L1CacheBase : public CacheBase
{
  uint32_t id;
public:
  L1CacheBase(uint32_t id)
    : CacheBase(NL1Set, NL1Way,
                l1_indexer_creator,
                l1_tagger_creator,
                l1_replacer_creator,
                "L1#"+std::to_string(id)),
      id(id) {}

  void read(uint64_t addr) {
    addr = CM::normalize(addr);
    uint32_t way = hit(addr);
    if(!way) way = fetch_read(addr);
    else way >>= 1;
    replacer->access(get_index(addr), way);
    REPORT::access_l1(addr, get_index(addr), way, id);
#ifdef DPRINT
    if(l1_targets.count(addr)) {
      std::cout << std::hex << "L1(" << id << ") read " << addr << std::endl;
      print_set(addr);
    }
#endif
  }

  void write(uint64_t addr) {
    addr = CM::normalize(addr);
    uint32_t way = hit(addr);
    if(!way)
      way = fetch_write(addr, false);
    else if(!CM::is_modified(get_meta(addr, way >> 1)))
      way = fetch_write(addr, true);
    else
      way >>= 1;
    replacer->access(indexer->index(addr), way);
    REPORT::access_l1(addr, get_index(addr), way, id);
#ifdef DPRINT
    if(l1_targets.count(addr)) {
      std::cout << std::hex << "L1(" << id << ") write " << addr << std::endl;
      print_set(addr);
    }
#endif
  }

  void probe(uint64_t addr, bool invalid);

private:
  uint32_t fetch_read(uint64_t addr);
  uint32_t fetch_write(uint64_t addr, bool perm_only);
  uint32_t replace(uint64_t addr);

};

/////////////////////////////////
// Coherent LLC(L2) caches

class LLCCacheBase : public CacheBase
{
  uint64_t *data;   // data array
  uint32_t id;
public:
  LLCCacheBase(uint32_t id)
    : CacheBase(NLLCSet, NLLCWay,
                llc_indexer_creator,
                llc_tagger_creator,
                llc_replacer_creator,
                "LLC#"+std::to_string(id)),
      id(id)
  {
    size_t s = sizeof(uint64_t)*nset*nway;
    data = (uint64_t *)malloc(s); memset(data, 0, s);
  }

  virtual ~LLCCacheBase() {
    free(data);
  }

  void fetch_read(uint64_t addr, uint32_t l1_id) {
    uint32_t way = hit(addr);
    if(way) { // hit
      way >>= 1;
      if(CM::is_modified(get_meta(addr, way))) {
        probe(addr, l1_id, false);
        set_meta(addr, way, CM::to_dirty(get_meta(addr, way)));
      }
    } else {  //miss
      extern_mem->read(addr, id);
      way = replace(addr);
      set_meta(addr, way, CM::to_shared(addr));
    }
    replacer->access(get_index(addr), way);
    REPORT::access_llc(addr, get_index(addr), way, id);

#ifdef DPRINT
      if(l2_targets.count(addr)) {
        std::cout << std::hex << "L2(" << id << ") fetch to read " << addr << std::endl;
        print_set(addr);
      }
#endif
  }

  void fetch_write(uint64_t addr, uint32_t l1_id) {
    uint32_t way = hit(addr);
    if(way) { // hit
      way >>= 1;
      if(CM::is_shared(get_meta(addr, way))) {
        probe(addr, l1_id, true);
        set_meta(addr, way, CM::to_modified(get_meta(addr, way)));
      }
    } else {  //miss
      extern_mem->read(addr, id);
      way = replace(addr);
      set_meta(addr, way, CM::to_modified(addr));
    }
    replacer->access(get_index(addr), way);
    REPORT::access_llc(addr, get_index(addr), way, id);

#ifdef DPRINT
      if(l2_targets.count(addr)) {
        std::cout << std::hex << "L2(" << id << ") fetch to write " << addr << std::endl;
        print_set(addr);
      }
#endif
  }

  void release(uint64_t addr, uint32_t l1_id) {
    uint32_t way = hit(addr);
    if(way) { //hit
      way >>= 1;
      uint64_t meta = get_meta(addr, way);
      meta = CM::to_shared(meta);
      meta = CM::to_dirty(meta);
      set_meta(addr, way, meta);
    } else { // miss
      // should never went here
#ifdef DPRINT
      std::cout << std::hex << "L2(" << id << ") release " << addr << " but MISSED!" << std::endl;
      print_set(addr);
#endif
      throw(addr);
    }
    replacer->access(get_index(addr), way);
    REPORT::access_llc(addr, get_index(addr), way, id);

#ifdef DPRINT
      if(l2_targets.count(addr)) {
        std::cout << std::hex << "L2(" << id << ") release " << addr << std::endl;
        print_set(addr);
      }
#endif
  }

private:

  uint32_t replace(uint64_t addr);
  void probe(uint64_t addr, uint32_t l1_id, bool invalid);

};

#endif
