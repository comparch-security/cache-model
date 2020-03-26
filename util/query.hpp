#ifndef UTIL_QUERY_HPP_
#define UTIL_QUERY_HPP_

#include "cache/definitions.hpp"
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <string>
#include <list>

// the status information related to a cache block
class CBInfo {
  uint64_t meta;
public:
  CBInfo() : meta(0) {}
  CBInfo(uint64_t m) : meta(m) {}
  uint64_t addr() const { return CM::normalize(meta);   }
  bool invalid()  const { return CM::is_invalid(meta);  }
  bool shared()   const { return CM::is_shared(meta);   }
  bool modified() const { return CM::is_modified(meta); }
  bool dirty()    const { return CM::is_dirty(meta);    }
  std::string to_string() const;
};

// the status information related to a cache set
class SetInfo {
public:
  std::vector<CBInfo> ways;
  std::string to_string() const;
};

class LocRange {
  std::pair<uint32_t, uint32_t> range;
public:
  LocRange() : range(0,0) {}
  LocRange(uint32_t l, uint32_t h) : range(l,h) {}
  std::string to_string() const;
};

// the possible location of an address in a cache
class LocInfo {
public:
  uint32_t level;
  int32_t core_id;
  uint32_t cache_id;
  CacheBase *cache;
  std::unordered_map<uint32_t, LocRange> locs;

  LocInfo(uint32_t level, int32_t core_id, uint32_t cache_id, CacheBase *cache)
    : level(level), core_id(core_id), cache_id(cache_id), cache(cache) {}
  LocInfo()
    : level(0), core_id(0), cache_id(0) {}
  void insert(uint32_t idx, LocRange r) { locs[idx] = r; }
  std::string to_string() const;
};

extern bool query_hit(uint64_t addr, CacheBase *cache);
extern uint32_t query_coloc(uint64_t addr, CacheBase *cache, std::list<uint64_t> evset);
extern bool query_check(uint64_t addr, CacheBase *cache, std::list<uint64_t> evset);

extern void print_locs(const std::list<LocInfo> &locs, uint32_t indent = 0);

#endif
