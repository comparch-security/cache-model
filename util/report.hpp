#ifndef UTIL_REPORT_HPP_
#define UTIL_REPORT_HPP_

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <boost/format.hpp>

template<typename RD>
class CacheDB {
  std::unordered_map<uint64_t, RD> db;      // the actual database

public:
  bool hit (uint64_t id) const { return db.count(id); }
  RD *get(uint64_t id) { return &(db[id]); }
  const RD *get(uint64_t id) const { return &(db.at(id)); }
  void set(uint64_t id, const RD& r) { db[id] = r; }
  std::string to_string() const {
    std::string rv;
    for(auto it=db.begin(); it!=db.end(); it++) {
      auto fmt = boost::format("%1%: %2%\n") % it->first, to_string(it->first);
      rv += fmt.str();
    }
    return rv;
  }
  virtual void clear()             { db.clear(); }
  virtual void clear(uint64_t id)  { db.erase(id); }
  virtual std::string to_string(uint64_t id) const { return std::string(); }
  virtual ~CacheDB() {}
};

class AccRecord {
public:
  uint32_t n_access;
  uint32_t n_evict;

  AccRecord() : n_access(0), n_evict(0) {}
  virtual std::string to_string() const {
    auto fmt = boost::format("evict %2% in %1% accesses") % n_access % n_evict;
    return fmt.str();
  }
  virtual ~AccRecord() {}
};

class DBAccType : public CacheDB<AccRecord> {
  uint64_t m_access;
  uint64_t m_evict;
public:
  DBAccType() : m_access(0), m_evict(0) {}
  virtual void access(uint64_t id)           { get(id)->n_access++; m_access++; }
  virtual uint32_t access(uint64_t id) const { return hit(id) ? get(id)->n_access : 0; }
  virtual uint32_t access() const            { return m_access; }
  virtual void evict(uint64_t id)            { get(id)->n_evict++; m_evict++; }
  virtual uint32_t evict(uint64_t id) const  { return hit(id) ? get(id)->n_evict : 0; }
  virtual uint32_t evict() const             { return m_evict; }
  virtual ~DBAccType() {}

  virtual void replace(uint64_t id) {
    auto r = get(id);
    r->n_access++;
    r->n_evict++;
    m_access++;
    m_evict++;
  }

  virtual std::string to_string(uint64_t id) const {
    return hit(id) ? get(id)->to_string() : std::string();
  }
};

class AddrRecord {
public:
  std::unordered_map<uint32_t, std::unordered_set<uint32_t> > a_map;

  virtual std::string to_string() const {
    std::string rv;
    for(auto i = a_map.begin(); i != a_map.end(); i++) {
      std::string content;
      for(auto j = i->second.begin(); j != i->second.end(); j++) {
        auto fmt = j == i->second.begin() ? boost::format(": %1%") : boost::format(", %1%");
        fmt % (*j);
        content += fmt.str();
      }
      auto fmt = boost::format("(%2%:%3%)") % i->first % content;
      if(rv.empty()) rv = fmt.str();
      else {
        rv += "; ";
        rv += fmt.str();
      }
    }
    return rv;
  }

  virtual ~AddrRecord() {}
};

class DBAddrType : public CacheDB<AddrRecord> {
public:
  virtual ~DBAddrType() {}

  virtual void access(uint64_t id, uint32_t idx, uint32_t way) {
    get(id)->a_map[idx].insert(way);
  }

  virtual std::string to_string(uint64_t id) {
    return hit(id) ? get(id)->to_string() : std::string();
  }
};

class StateRecord {
public:
  uint32_t state;
  uint32_t level;
  int32_t core_id;
  int32_t cache_id;
  uint32_t idx;
  uint32_t way;
  StateRecord() : state(0) {}

  virtual std::string to_string() const {
    switch(state) {
    case 1:  return "S";
    case 2:  return "M";
    case 3:  return "O";
    case 4:  return "E";
    default: return "I";
    }
  }

  virtual ~StateRecord() {}
};

class DBStateType : public CacheDB<StateRecord> {
public:
  virtual ~DBStateType() {}
  virtual void set_state(uint64_t id, uint32_t s) { get(id)->state = s; }
  virtual bool is_state(uint64_t id, uint32_t s) const { return hit(id) && get(id)->state == s; }
  virtual bool is_hit(uint64_t id) const { return hit(id) && get(id)->state > 0; }
  virtual void set_location(uint64_t id, uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way) {
    auto record = get(id);
    record->level    = level;
    record->core_id  = core_id;
    record->cache_id = cache_id;
    record->idx      = idx;
    record->way      = way;
  }
  virtual bool get_location(uint64_t id, uint32_t *level, int32_t *core_id, int32_t *cache_id, uint32_t *idx, uint32_t *way) const {
    if(hit(id)) {
      auto record = get(id);
      *level    = record->level;
      *core_id  = record->core_id;
      *cache_id = record->cache_id;
      *idx      = record->idx;
      *way      = record->way;
      return true;
    } else return false;
  }
  virtual std::string to_string(uint64_t id) {
    return hit(id) ? get(id)->to_string() : std::string();
  }
};

class Reporter_t
{
  std::unordered_map<uint64_t, DBAccType>   acc_dbs;     // record various access numbers
  std::unordered_map<uint64_t, DBAddrType>  addr_dbs;    // record the set/way pairs of an addr in a cache
  std::unordered_map<uint64_t, DBStateType> state_dbs;   // record the coherent status of something

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
  Reporter_t() : db_depth(4, false), db_type(3, false) {}

  // register recorders
  void register_cache_access_tracer(uint32_t level) {
    uint64_t id = hash(level);
    assert(!acc_dbs.count(id));
    acc_dbs[id];
    db_depth[0] = true;
    db_type[0] = true;
  }

  void register_cache_access_tracer(uint32_t level, int32_t core_id) {
    uint64_t id = hash(level, core_id);
    assert(!acc_dbs.count(id));
    acc_dbs[id];
    db_depth[1] = true;
    db_type[0] = true;
  }

  void register_cache_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    uint64_t id = hash(level, core_id, cache_id);
    assert(!acc_dbs.count(id));
    acc_dbs[id];
    db_depth[2] = true;
    db_type[0] = true;
  }

  void register_set_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx) {
    uint64_t id = hash(level, core_id, cache_id, idx);
    assert(!acc_dbs.count(id));
    acc_dbs[id];
    db_depth[3] = true;
    db_type[0] = true;
  }

  void register_cache_addr_tracer(uint32_t level) {
    uint64_t id = hash(level);
    assert(!addr_dbs.count(id));
    addr_dbs[id];
    db_depth[0] = true;
    db_type[1] = true;
  }

  void register_cache_addr_tracer(uint32_t level, int32_t core_id) {
    uint64_t id = hash(level, core_id);
    assert(!addr_dbs.count(id));
    addr_dbs[id];
    db_depth[1] = true;
    db_type[1] = true;
  }

  void register_cache_addr_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    uint64_t id = hash(level, core_id, cache_id);
    assert(!addr_dbs.count(id));
    addr_dbs[id];
    db_depth[2] = true;
    db_type[1] = true;
  }

  void register_cache_state_tracer(uint32_t level) {
    uint64_t id = hash(level);
    assert(!state_dbs.count(id));
    state_dbs[id];
    db_depth[0] = true;
    db_type[2] = true;
  }

  void register_cache_state_tracer(uint32_t level, int32_t core_id) {
    uint64_t id = hash(level, core_id);
    assert(!state_dbs.count(id));
    state_dbs[id];
    db_depth[1] = true;
    db_type[2] = true;
  }

  void register_cache_state_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    uint64_t id = hash(level, core_id, cache_id);
    assert(!state_dbs.count(id));
    state_dbs[id];
    db_depth[2] = true;
    db_type[2] = true;
  }

  // clear recorders
  void clear_acc_dbs()    { acc_dbs.clear();   db_type[0] = false; }
  void clear_addr_dbs()   { addr_dbs.clear();  db_type[1] = false; }
  void clear_state_dbs()  { state_dbs.clear(); db_type[2] = false; }
  void clear() {
    clear_acc_dbs();
    clear_addr_dbs();
    clear_state_dbs();
    db_depth = std::vector<bool>(4, false);
  }

  // event recorders
  void cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way, uint32_t state) {
    uint64_t record = addr_hash(addr);
    if(db_depth[0]) {
      uint64_t id = hash(level);
      if(db_type[0]) acc_dbs[id].access(record);
      if(db_type[1]) addr_dbs[id].access(record, idx, way);
      if(db_type[2]) state_dbs[id].set_state(record, state);
    }
    if(db_depth[1]) {
      uint64_t id = hash(level, core_id);
      if(db_type[0]) acc_dbs[id].access(record);
      if(db_type[1]) addr_dbs[id].access(record, idx, way);
      if(db_type[2]) state_dbs[id].set_state(record, state);
    }
    if(db_depth[2]) {
      uint64_t id = hash(level, core_id, cache_id);
      if(db_type[0]) acc_dbs[id].access(record);
      if(db_type[1]) addr_dbs[id].access(record, idx, way);
      if(db_type[2]) state_dbs[id].set_state(record, state);
    }
    if(db_depth[3]) {
      uint64_t id = hash(level, core_id, cache_id, idx);
      if(db_type[0]) acc_dbs[id].access(record);
    }
  }

  void cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way) {
    uint64_t record = addr_hash(addr);
    if(db_depth[0]) {
      uint64_t id = hash(level);
      if(db_type[0]) acc_dbs[id].evict(record);
      if(db_type[2]) state_dbs[id].set_state(record, 0);
    }
    if(db_depth[1]) {
      uint64_t id = hash(level, core_id);
      if(db_type[0]) acc_dbs[id].evict(record);
      if(db_type[2]) state_dbs[id].set_state(record, 0);
    }
    if(db_depth[2]) {
      uint64_t id = hash(level, core_id, cache_id);
      if(db_type[0]) acc_dbs[id].evict(record);
      if(db_type[2]) state_dbs[id].set_state(record, 0);
    }
    if(db_depth[3]) {
      uint64_t id = hash(level, core_id, cache_id, idx);
      if(db_type[0]) acc_dbs[id].evict(record);
    }
  }

  // event checkers
  inline bool check_hit_generic(uint64_t id, uint64_t addr) const {
    if(state_dbs.count(id)) {
      return state_dbs.at(id).is_hit(addr_hash(addr));
    }
    return false;
  }
  inline bool check_hit_generic(uint64_t id, uint64_t addr, uint32_t *level, int32_t *core_id, int32_t *cache_id, uint32_t *idx, uint32_t *way) const {
    uint64_t record = addr_hash(addr);
    if(state_dbs.count(id)) {
      if(state_dbs.at(id).is_hit(record)) {
        state_dbs.at(id).get_location(record, level, core_id, cache_id, idx, way);
         return true;
      }
    }
    return false;
  }
  bool check_hit(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level, core_id, cache_id), addr, &level, &core_id, &cache_id, idx, way);
  }
  bool check_hit(uint32_t level, int32_t core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level, core_id), addr, &level, &core_id, cache_id, idx, way);
  }
  bool check_hit(uint32_t level, int32_t *core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level), addr, &level, core_id, cache_id, idx, way);
  }
  bool check_hit(uint32_t *level, int32_t *core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
    uint64_t record = addr_hash(addr);
    if(!state_dbs.empty()) {
      for(auto db: state_dbs)
        if(db.second.is_hit(record)) {
          db.second.get_location(record, level, core_id, cache_id, idx, way);
          return true;
        }
    }
    return false;
  }
  bool check_hit(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const {
    return check_hit_generic(hash(level, core_id, cache_id), addr);
  }
  bool check_hit(uint32_t level, int32_t core_id, uint64_t addr) const {
    return check_hit_generic(hash(level, core_id), addr);
  }
  bool check_hit(uint32_t level, uint64_t addr) const {
    return check_hit_generic(hash(level), addr);
  }
  bool check_hit(uint64_t addr) const {
    uint64_t record = addr_hash(addr);
    if(!state_dbs.empty()) {
      for(auto db: state_dbs)
        if(db.second.is_hit(record)) return true;
    }
    return false;
  }
  inline uint32_t check_cache_access_generic(uint64_t id) const {
    return acc_dbs.count(id) ? acc_dbs.at(id).access() : 0;
  }
  uint32_t check_cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way) const {
    return check_cache_access_generic(hash(level, core_id, cache_id, idx, way));
  }
  uint32_t check_cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx) const {
    return check_cache_access_generic(hash(level, core_id, cache_id, idx));
  }
  uint32_t check_cache_access(uint32_t level, int32_t core_id, int32_t cache_id) const {
    return check_cache_access_generic(hash(level, core_id, cache_id));
  }
  uint32_t check_cache_access(uint32_t level, int32_t core_id) const {
    return check_cache_access_generic(hash(level, core_id));
  }
  uint32_t check_cache_access(uint32_t level) const {
    return check_cache_access_generic(hash(level));
  }
  uint32_t check_addr_access_generic(uint64_t id, uint64_t addr) const {
    return acc_dbs.count(id) ? acc_dbs.at(id).access(addr_hash(addr)) : 0;
  }
  uint32_t check_addr_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id, cache_id, idx, way), addr);
  }
  uint32_t check_addr_access(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id, cache_id, idx), addr);
  }
  uint32_t check_addr_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id, cache_id), addr);
  }
  uint32_t check_addr_access(uint32_t level, int32_t core_id, uint64_t addr) const {
    return check_addr_access_generic(hash(level, core_id), addr);
  }
  uint32_t check_addr_access(uint32_t level, uint64_t addr) const {
    return check_addr_access_generic(hash(level), addr);
  }
  uint32_t check_cache_evict_generic(uint64_t id) const {
    return acc_dbs.count(id) ? acc_dbs.at(id).evict() : 0;
  }
  uint32_t check_cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way) const {
    return check_cache_evict_generic(hash(level, core_id, cache_id, idx, way));
  }
  uint32_t check_cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx) const {
    return check_cache_evict_generic(hash(level, core_id, cache_id, idx));
  }
  uint32_t check_cache_evict(uint32_t level, int32_t core_id, int32_t cache_id) const {
    return check_cache_evict_generic(hash(level, core_id, cache_id));
  }
  uint32_t check_cache_evict(uint32_t level, int32_t core_id) const {
    return check_cache_evict_generic(hash(level, core_id));
  }
  uint32_t check_cache_evict(uint32_t level) const {
    return check_cache_evict_generic(hash(level));
  }
  uint32_t check_addr_evict_generic(uint64_t id, uint64_t addr) const {
    return acc_dbs.count(id) ? acc_dbs.at(id).evict(addr_hash(addr)) : 0;
  }
  uint32_t check_addr_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint32_t way, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id, cache_id, idx, way), addr);
  }
  uint32_t check_addr_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id, cache_id, idx), addr);
  }
  uint32_t check_addr_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id, cache_id), addr);
  }
  uint32_t check_addr_evict(uint32_t level, int32_t core_id, uint64_t addr) const {
    return check_addr_evict_generic(hash(level, core_id), addr);
  }
  uint32_t check_addr_evict(uint32_t level, uint64_t addr) const {
    return check_addr_evict_generic(hash(level), addr);
  }
};

#endif
