#ifndef UTIL_REPORT_HPP_
#define UTIL_REPORT_HPP_

#include <cstdint>
#include <vector>

class ReportDBs;

class Reporter_t
{
  ReportDBs *dbs;

  inline uint64_t hash(uint32_t level) const {
    return (uint64_t)(level)<<56;
  }

  inline uint64_t hash(uint32_t level, int32_t core_id) const {
    return ((uint64_t)(level)<<56)|((uint64_t)(core_id)<<44);
  }

  inline uint64_t hash(uint32_t level, int32_t core_id, int32_t cache_id) const {
    return ((uint64_t)(level)<<56)|((uint64_t)(core_id)<<44)|((uint64_t)(cache_id)<<32);
  }

  inline uint64_t hash(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx) const {
    return ((uint64_t)(level)<<56)|((uint64_t)(core_id)<<44)|((uint64_t)(cache_id)<<32)|((uint64_t)(idx)<<16);
  }

  inline uint64_t hash(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way) const {
    return ((uint64_t)(level)<<56)|((uint64_t)(core_id)<<44)|((uint64_t)(cache_id)<<32)|((uint64_t)(idx)<<16)|way;
  }

  inline uint64_t addr_hash(uint64_t addr) const {
    return addr;
  }

  std::vector<bool> db_depth;
  std::vector<bool> db_type;

public:
  Reporter_t();
  ~Reporter_t();

  // register recorders
  void register_cache_access_tracer(uint32_t level);
  void register_cache_access_tracer(uint32_t level, int32_t core_id);
  void register_cache_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id);
  void register_set_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx);
  void register_cache_addr_tracer(uint32_t level);
  void register_cache_addr_tracer(uint32_t level, int32_t core_id);
  void register_cache_addr_tracer(uint32_t level, int32_t core_id, int32_t cache_id);
  void register_cache_state_tracer(uint32_t level);
  void register_cache_state_tracer(uint32_t level, int32_t core_id);
  void register_cache_state_tracer(uint32_t level, int32_t core_id, int32_t cache_id);
  void register_address_tracer(uint64_t addr);
  void register_set_tracer(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx);

  // clear recorders
  void clear_acc_dbs();
  void clear_addr_dbs();
  void clear_state_dbs();
  void clear_addr_traces();
  void clear_set_traces();
  void clear();

  // event recorders
  void cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way, uint32_t state);
  void cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way);

  // event checkers
  bool check_hit_generic(uint64_t id, uint64_t addr) const;
  bool check_hit_generic(uint64_t id, uint64_t addr, uint32_t *level, int32_t *core_id, int32_t *cache_id, uint32_t *idx, uint32_t *way) const;
  inline bool check_hit(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level, core_id, cache_id), addr, &level, &core_id, &cache_id, idx, way);
  }
  inline bool check_hit(uint32_t level, int32_t core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level, core_id), addr, &level, &core_id, cache_id, idx, way);
  }
  inline bool check_hit(uint32_t level, int32_t *core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level), addr, &level, core_id, cache_id, idx, way);
  }
  bool check_hit(uint32_t *level, int32_t *core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const;
  inline bool check_hit(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const {
    return check_hit_generic(hash(level, core_id, cache_id), addr);
  }
  inline bool check_hit(uint32_t level, int32_t core_id, uint64_t addr) const {
    return check_hit_generic(hash(level, core_id), addr);
  }
  inline bool check_hit(uint32_t level, uint64_t addr) const {
    return check_hit_generic(hash(level), addr);
  }
  bool check_hit(uint64_t addr) const;
  uint32_t check_cache_access_generic(uint64_t id) const;
  inline uint32_t check_cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way) const {
    return check_cache_access_generic(hash(level, core_id, cache_id, idx, way));
  }
  inline uint32_t check_cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx) const {
    return check_cache_access_generic(hash(level, core_id, cache_id, idx));
  }
  inline uint32_t check_cache_access(uint32_t level, int32_t core_id, int32_t cache_id) const {
    return check_cache_access_generic(hash(level, core_id, cache_id));
  }
  inline uint32_t check_cache_access(uint32_t level, int32_t core_id) const {
    return check_cache_access_generic(hash(level, core_id));
  }
  inline uint32_t check_cache_access(uint32_t level) const {
    return check_cache_access_generic(hash(level));
  }
  uint32_t check_addr_access_generic(uint64_t id, uint64_t addr) const;
  inline uint32_t check_addr_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id, cache_id, idx, way), addr);
  }
  inline uint32_t check_addr_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id, cache_id, idx), addr);
  }
  inline uint32_t check_addr_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id, cache_id), addr);
  }
  inline uint32_t check_addr_access(uint32_t level, int32_t core_id, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id), addr);
  }
  inline uint32_t check_addr_access(uint32_t level, uint64_t addr) const {
    return check_addr_access_generic(hash(level), addr);
  }
  uint32_t check_cache_evict_generic(uint64_t id) const;
  inline uint32_t check_cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way) const {
    return check_cache_evict_generic(hash(level, core_id, cache_id, idx, way));
  }
  inline uint32_t check_cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx) const {
    return check_cache_evict_generic(hash(level, core_id, cache_id, idx));
  }
  inline uint32_t check_cache_evict(uint32_t level, int32_t core_id, int32_t cache_id) const {
    return check_cache_evict_generic(hash(level, core_id, cache_id));
  }
  inline uint32_t check_cache_evict(uint32_t level, int32_t core_id) const {
    return check_cache_evict_generic(hash(level, core_id));
  }
  inline uint32_t check_cache_evict(uint32_t level) const {
    return check_cache_evict_generic(hash(level));
  }
  uint32_t check_addr_evict_generic(uint64_t id, uint64_t addr) const;
  inline uint32_t check_addr_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id, cache_id, idx, way), addr);
  }
  inline uint32_t check_addr_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id, cache_id, idx), addr);
  }
  inline uint32_t check_addr_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id, cache_id), addr);
  }
  inline uint32_t check_addr_evict(uint32_t level, int32_t core_id, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id), addr);
  }
  inline uint32_t check_addr_evict(uint32_t level, uint64_t addr) const {
    return check_addr_evict_generic(hash(level), addr);
  }
};

#endif
