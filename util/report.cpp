#include "util/report.hpp"
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <boost/format.hpp>
#include <iostream>
#include <fstream>

template<typename RD>
class CacheDB {
  std::unordered_map<uint64_t, RD> db;      // the actual database

public:
  bool hit (uint64_t id) const { return db.count(id); }
  RD *get(uint64_t id) { return &(db[id]); }
  const RD *get(uint64_t id) const { return &(db.at(id)); }
  void set(uint64_t id, const RD& r) { db[id] = r; }
  typename std::unordered_map<uint64_t, RD>::const_iterator db_begin() const { return db.begin(); }
  typename std::unordered_map<uint64_t, RD>::const_iterator db_end() const { return db.end(); }
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
  uint64_t n_access;
  uint64_t n_hit;
  uint64_t n_evict;
  uint64_t n_writeback;

  AccRecord() : n_access(0), n_hit(0), n_evict(0), n_writeback(0) {}
  virtual std::string to_string() const {
    auto fmt = boost::format("hit %1%, evict %2% and writeback %3% in %4% accesses") % n_hit % n_evict % n_writeback % n_access;
    return fmt.str();
  }
  virtual ~AccRecord() {}
};

class DBAccType : public CacheDB<AccRecord> {
  uint64_t m_access;
  uint64_t m_hit;
  uint64_t m_evict;
  uint64_t m_writeback;
public:
  bool detailed_to_addr;
  DBAccType() : m_access(0), m_hit(0), m_evict(0), m_writeback(0), detailed_to_addr(false) {}
  virtual void access(uint64_t id, bool bhit, uint64_t *wt) {
    if(detailed_to_addr) get(id)->n_access++;
    m_access++;
    if(bhit && detailed_to_addr) get(id)->n_hit++;
    if(bhit) m_hit++;

  }
  virtual void evict(uint64_t id) {
    if(detailed_to_addr) get(id)->n_evict++;
    m_evict++;
  }
  virtual void writeback(uint64_t id) {
    if(detailed_to_addr) get(id)->n_writeback++;
    m_writeback++;
  }
  virtual uint64_t get_access(uint64_t id) const { return hit(id) ? get(id)->n_access : 0; }
  virtual uint64_t get_access() const            { return m_access; }
  virtual uint64_t get_hit(uint64_t id) const    { return hit(id) ? get(id)->n_hit : 0; }
  virtual uint64_t get_hit() const               { return m_hit; }
  virtual uint64_t get_miss(uint64_t id) const   { return hit(id) ? get(id)->n_access - get(id)->n_hit : 0; }
  virtual uint64_t get_miss() const              { return m_access - m_hit; }
  virtual uint64_t get_evict(uint64_t id) const  { return hit(id) ? get(id)->n_evict : 0; }
  virtual uint64_t get_evict() const             { return m_evict; }
  virtual uint64_t get_writeback(uint64_t id) const  { return hit(id) ? get(id)->n_writeback : 0; }
  virtual uint64_t get_writeback() const             { return m_writeback; }
  virtual void clear() {
    CacheDB<AccRecord>::clear();
    m_access = 0;
    m_hit = 0;
    m_evict = 0;
    m_writeback = 0;
  }

  virtual ~DBAccType() {}

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

class DBAddrTraceType : public CacheDB<bool> {
public:
  virtual ~DBAddrTraceType() {}
  virtual void report(std::string msg, uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way, uint32_t state) {
    auto fmt = boost::format("0x%016x ") % addr;
    std::cout << fmt.str();
    fmt = boost::format(msg + " at level %1% core %2% cache %3% set %4% way %5% [%6%]") % level % core_id % cache_id % idx % way % state;
    std::cout << fmt.str() << std::endl;
  }
};

class DBSetTraceType : public CacheDB<std::unordered_set<uint64_t> > {
public:
  virtual ~DBSetTraceType() {}
  virtual void access(uint64_t id, uint64_t addr) {
    auto blocks = get(id);
    if(!blocks->count(addr)) {
      blocks->insert(addr);
      auto fmt = boost::format("0x%016x: ") % addr;
      std::cout << "set " << id << " add " << fmt.str() << to_string(id) << std::endl;
    }
  }

  virtual void evict(uint64_t id, uint64_t addr) {
    auto blocks = get(id);
    if(blocks->count(addr)) blocks->erase(addr);
    auto fmt = boost::format("0x%016x: ") % addr;
    std::cout << "set " << id << " evict " << fmt.str() << to_string(id) << std::endl;
  }

  virtual std::string to_string(uint64_t id) {
    auto blocks = get(id);
    std::string rv;
    auto it = blocks->begin();
    while(it != blocks->end()) {
      auto fmt = boost::format("0x%016x") % *it;
      rv += fmt.str();
      it++;
      if(it != blocks->end()) rv += " ";
    }
    return rv;
  }
};

struct ReportDBs {
  std::unordered_map<uint64_t, DBAccType>   acc_dbs;     // record various access numbers
  std::unordered_map<uint64_t, DBAddrType>  addr_dbs;    // record the set/way pairs of an addr in a cache
  std::unordered_map<uint64_t, DBStateType> state_dbs;   // record the coherent status of something
  DBAddrTraceType                           addr_traces; // trace a group of specific address of interests
  DBSetTraceType                            set_traces;  // trace a group of specific sets of interests
};

Reporter_t::Reporter_t() : dbs(new ReportDBs), db_depth(4, false), db_type(4, false) {}
Reporter_t::~Reporter_t() { delete dbs; }


// register recorders
void Reporter_t::register_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr, bool extra) {
  uint64_t id;
  switch(tracer_depth) {
  case 0: id = hash(level); break;
  case 1: id = hash(level, core_id); break;
  case 2: id = hash(level, core_id, cache_id); break;
  case 3: id = hash(level, core_id, cache_id, idx); break;
  default: id = 0; // should not run here
}
  switch(tracer_type) {
  case 0: // access trace
  assert(!dbs->acc_dbs.count(id));
    dbs->acc_dbs[id].detailed_to_addr = extra;
    db_depth[tracer_depth] = true;
  db_type[0] = true;
    break;
  case 1: // address trace
  assert(!dbs->addr_dbs.count(id));
  dbs->addr_dbs[id];
    db_depth[tracer_depth] = true;
  db_type[1] = true;
    break;
  case 2: // state trace
  assert(!dbs->state_dbs.count(id));
  dbs->state_dbs[id];
    db_depth[tracer_depth] = true;
  db_type[2] = true;
    break;
  case 3: // address/set monitor
    if(tracer_depth == 3) {
      dbs->set_traces.set(id, std::unordered_set<uint64_t>());
      db_depth[tracer_depth] = true;
    } else {
      dbs->addr_traces.set(addr, true);
    }
    db_type[3] = true;
    break;
  default:
    return; // should not run to here
  }
}

// remove a certain recorder
void Reporter_t::remove_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr) {
  uint64_t id;
  switch(tracer_depth) {
  case 0: id = hash(level); break;
  case 1: id = hash(level, core_id); break;
  case 2: id = hash(level, core_id, cache_id); break;
  case 3: id = hash(level, core_id, cache_id, idx); break;
  default: id = 0; // should not run here
  }
  switch(tracer_type) {
  case 0: // access trace
    assert(dbs->acc_dbs.count(id));
    dbs->acc_dbs.erase(id);
    break;
  case 1: // address trace
    assert(dbs->addr_dbs.count(id));
    dbs->addr_dbs.erase(id);
    break;
  case 2: // state trace
    assert(dbs->state_dbs.count(id));
    dbs->state_dbs.erase(id);
    break;
  case 3: // address/set monitor
    if(tracer_depth == 3) {
      dbs->set_traces.clear(id);
    } else {
      dbs->addr_traces.clear(addr);
    }
    break;
  default:
    return; // should not run to here
  }
}

// reset a certain recorder
void Reporter_t::reset_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, uint32_t idx, uint64_t addr) {
  uint64_t id;
  switch(tracer_depth) {
  case 0: id = hash(level); break;
  case 1: id = hash(level, core_id); break;
  case 2: id = hash(level, core_id, cache_id); break;
  case 3: id = hash(level, core_id, cache_id, idx); break;
  default: id = 0; // should not run here
  }
  switch(tracer_type) {
  case 0: // access trace
    assert(dbs->acc_dbs.count(id));
    dbs->acc_dbs[id].clear();
    break;
  case 1: // address trace
    assert(dbs->addr_dbs.count(id));
    dbs->addr_dbs[id].clear();
    break;
  case 2: // state trace
    assert(dbs->state_dbs.count(id));
    dbs->state_dbs[id].clear();
    break;
  default:
    return; // should not run to here
  }
}

// clear recorders
void Reporter_t::clear_acc_dbs()     { dbs->acc_dbs.clear();   db_type[0] = false; }
void Reporter_t::clear_addr_dbs()    { dbs->addr_dbs.clear();  db_type[1] = false; }
void Reporter_t::clear_state_dbs()   { dbs->state_dbs.clear(); db_type[2] = false; }
void Reporter_t::clear_addr_traces() { dbs->addr_traces.clear();                   }
void Reporter_t::clear_set_traces()  { dbs->set_traces.clear();                    }
void Reporter_t::clear() {
  clear_acc_dbs();
  clear_addr_dbs();
  clear_state_dbs();
  clear_addr_traces();
  clear_set_traces();
  db_depth = std::vector<bool>(4, false);
  db_type = std::vector<bool>(4, false);
}

  // event recorders
void Reporter_t::cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way, uint32_t state, bool hit) {
  uint64_t record = addr_hash(addr);
  if(db_depth[0]) {
    uint64_t id = hash(level);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].access(record, hit, wall_time);
    if(db_type[1] && dbs->addr_dbs.count(id))  dbs->addr_dbs[id].access(record, idx, way);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, state);
  }
  if(db_depth[1]) {
    uint64_t id = hash(level, core_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].access(record, hit, wall_time);
    if(db_type[1] && dbs->addr_dbs.count(id))  dbs->addr_dbs[id].access(record, idx, way);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, state);
  }
  if(db_depth[2]) {
    uint64_t id = hash(level, core_id, cache_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].access(record, hit, wall_time);
    if(db_type[1] && dbs->addr_dbs.count(id))  dbs->addr_dbs[id].access(record, idx, way);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, state);
  }
  if(db_depth[3]) {
    uint64_t id = hash(level, core_id, cache_id, idx);
    if(db_type[0] && dbs->acc_dbs.count(id))  dbs->acc_dbs[id].access(record, hit, wall_time);
    if(db_type[3] && dbs->set_traces.hit(id)) dbs->set_traces.access(id, addr);
  }
  if(db_type[3] && dbs->addr_traces.hit(addr)) {
    dbs->addr_traces.report("is accessed", level, core_id, cache_id, addr, idx, way, state);
  }
}

void Reporter_t::cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way) {
  uint64_t record = addr_hash(addr);
  if(db_depth[0]) {
    uint64_t id = hash(level);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].evict(record);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, 0);
  }
  if(db_depth[1]) {
    uint64_t id = hash(level, core_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].evict(record);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, 0);
  }
  if(db_depth[2]) {
    uint64_t id = hash(level, core_id, cache_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].evict(record);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, 0);
  }
  if(db_depth[3]) {
    uint64_t id = hash(level, core_id, cache_id, idx);
    if(db_type[0] && dbs->acc_dbs.count(id)) dbs->acc_dbs[id].evict(record);
    if(db_type[3] && dbs->set_traces.hit(id)) dbs->set_traces.evict(id, addr);
  }
  if(db_type[3] && dbs->addr_traces.hit(addr)) {
    dbs->addr_traces.report("is evicted", level, core_id, cache_id, addr, idx, way, 0);
  }
}

void Reporter_t::cache_writeback(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, uint32_t idx, uint32_t way) {
  uint64_t record = addr_hash(addr);
  if(db_depth[0]) {
    uint64_t id = hash(level);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].writeback(record);
  }
  if(db_depth[1]) {
    uint64_t id = hash(level, core_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].writeback(record);
  }
  if(db_depth[2]) {
    uint64_t id = hash(level, core_id, cache_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].writeback(record);
  }
  if(db_depth[3]) {
    uint64_t id = hash(level, core_id, cache_id, idx);
    if(db_type[0] && dbs->acc_dbs.count(id)) dbs->acc_dbs[id].writeback(record);
  }
}

// event checkers
bool Reporter_t::check_hit_generic(uint64_t id, uint64_t addr) const {
  if(dbs->state_dbs.count(id)) {
    return dbs->state_dbs.at(id).is_hit(addr_hash(addr));
  }
  return false;
}

bool Reporter_t::check_hit_generic(uint64_t id, uint64_t addr, uint32_t *level, int32_t *core_id, int32_t *cache_id, uint32_t *idx, uint32_t *way) const {
  uint64_t record = addr_hash(addr);
  if(dbs->state_dbs.count(id)) {
    if(dbs->state_dbs.at(id).is_hit(record)) {
      dbs->state_dbs.at(id).get_location(record, level, core_id, cache_id, idx, way);
      return true;
    }
  }
  return false;
}

bool Reporter_t::check_hit(uint32_t *level, int32_t *core_id, int32_t *cache_id, uint64_t addr, uint32_t *idx, uint32_t *way) const {
  uint64_t record = addr_hash(addr);
  if(!dbs->state_dbs.empty()) {
    for(auto db: dbs->state_dbs)
      if(db.second.is_hit(record)) {
        db.second.get_location(record, level, core_id, cache_id, idx, way);
        return true;
      }
  }
  return false;
}

bool Reporter_t::check_hit(uint64_t addr) const {
  uint64_t record = addr_hash(addr);
  if(!dbs->state_dbs.empty()) {
    for(auto db: dbs->state_dbs)
      if(db.second.is_hit(record)) return true;
  }
  return false;
}

uint64_t Reporter_t::check_cache_access_generic(uint64_t id) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_access() : 0;
}

uint64_t Reporter_t::check_addr_access_generic(uint64_t id, uint64_t addr) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_access(addr_hash(addr)) : 0;
}

uint64_t Reporter_t::check_cache_hit_generic(uint64_t id) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_hit() : 0;
}

uint64_t Reporter_t::check_addr_hit_generic(uint64_t id, uint64_t addr) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_hit(addr_hash(addr)) : 0;
}

uint64_t Reporter_t::check_cache_miss_generic(uint64_t id) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_miss() : 0;
}

uint64_t Reporter_t::check_addr_miss_generic(uint64_t id, uint64_t addr) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_miss(addr_hash(addr)) : 0;
}

uint64_t Reporter_t::check_cache_evict_generic(uint64_t id) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_evict() : 0;
}

uint64_t Reporter_t::check_addr_evict_generic(uint64_t id, uint64_t addr) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_evict(addr_hash(addr)) : 0;
}

uint64_t Reporter_t::check_cache_writeback_generic(uint64_t id) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_writeback() : 0;
}

uint64_t Reporter_t::check_addr_writeback_generic(uint64_t id, uint64_t addr) const {
  return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_writeback(addr_hash(addr)) : 0;
}
