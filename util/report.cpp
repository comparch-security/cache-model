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
  uint64_t n_read;
  uint64_t n_write;
  uint64_t n_read_miss;
  uint64_t n_write_miss;
  uint64_t n_hit;
  uint64_t n_evict;
  uint64_t n_writeback;

  AccRecord() : n_access(0), n_read(0), n_write(0), n_read_miss(0), n_write_miss(0), n_hit(0), n_evict(0), n_writeback(0) {}
  virtual std::string to_string() const {
    auto fmt = boost::format("hit %1%, evict %2% and writeback %3% in %4% accesses") % n_hit % n_evict % n_writeback % n_access;
    return fmt.str();
  }
  virtual ~AccRecord() {}
};

class DBAccType : public CacheDB<AccRecord> {
  uint64_t m_access;
  uint64_t m_read;
  uint64_t m_write;
  uint64_t m_read_miss;
  uint64_t m_write_miss;
  uint64_t m_hit;
  uint64_t m_evict;
  uint64_t m_writeback;
  std::string fn;
  uint64_t period;
  uint64_t m_hit_pre;
public:
  bool detailed_to_addr;
  DBAccType() : m_access(0), m_read(0), m_write(0), m_read_miss(0), m_write_miss(0),
                m_hit(0), m_evict(0), m_writeback(0), period(0), m_hit_pre(0), detailed_to_addr(false) {}
  virtual void access(uint64_t id, bool bhit, uint8_t rw, uint64_t *wt) {
    if(detailed_to_addr) { get(id)->n_access++; if(rw == 1) get(id)->n_read++; if(rw == 2) get(id)->n_write++; }
    m_access++; if(rw == 1) m_read++; if(rw == 2) m_write++;
    if(bhit) { m_hit++; if(detailed_to_addr) get(id)->n_hit++; }
    else {
      if(rw == 1) { m_read_miss++;  if(detailed_to_addr) get(id)->n_read_miss++;  }
      if(rw == 2) { m_write_miss++; if(detailed_to_addr) get(id)->n_write_miss++; }
    }

    // report
    if(period != 0 && (m_access % period == 0)) {
      std::ofstream ofile(fn, std::ios_base::app);
      if(wt) ofile << *wt << ",";
      ofile << m_access << "," << m_hit << "," << m_access - m_hit << ","
            << m_evict << "," << m_writeback << ","
            << (double)m_hit / m_access << ","
            << (double)(m_hit - m_hit_pre) / period
            << std::endl;
      ofile.close();
      m_hit_pre = m_hit;
    }
  }
  virtual void evict(uint64_t id) {
    if(detailed_to_addr) get(id)->n_evict++;
    m_evict++;
  }
  virtual void writeback(uint64_t id) {
    if(detailed_to_addr) get(id)->n_writeback++;
    m_writeback++;
  }

  #define GET_FUNC(T)    virtual uint64_t get_##T() const            { return m_##T; }
  #define GET_FUNC_ID(T) virtual uint64_t get_##T(uint64_t id) const { return hit(id) ? get(id)->n_##T : 0; }
  GET_FUNC(access)
  GET_FUNC_ID(access)
  GET_FUNC(read)
  GET_FUNC_ID(read)
  GET_FUNC(write)
  GET_FUNC_ID(write)
  GET_FUNC(read_miss)
  GET_FUNC_ID(read_miss)
  GET_FUNC(write_miss)
  GET_FUNC_ID(write_miss)
  GET_FUNC(hit)
  GET_FUNC_ID(hit)
  GET_FUNC(evict)
  GET_FUNC_ID(evict)
  GET_FUNC(writeback)
  GET_FUNC_ID(writeback)
  #undef GET_FUNC
  #undef GET_FUNC_ID

  virtual uint64_t get_miss(uint64_t id) const   { return hit(id) ? get(id)->n_access - get(id)->n_hit : 0; }
  virtual uint64_t get_miss() const              { return m_access - m_hit; }

  virtual void clear() {
    CacheDB<AccRecord>::clear();
    m_access = 0;
    m_read = 0;
    m_write = 0;
    m_read_miss = 0;
    m_write_miss = 0;
    m_hit = 0;
    m_evict = 0;
    m_writeback = 0;
  }

  void set_reporter(const std::string &f, uint64_t p) {
    period = p;
    fn = f;
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

  virtual void access(uint64_t id, int32_t idx, uint32_t way) {
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
  int32_t idx;
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
  virtual void set_location(uint64_t id, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint32_t way) {
    auto record = get(id);
    record->level    = level;
    record->core_id  = core_id;
    record->cache_id = cache_id;
    record->idx      = idx;
    record->way      = way;
  }
  virtual bool get_location(uint64_t id, uint32_t *level, int32_t *core_id, int32_t *cache_id, int32_t *idx, uint32_t *way) const {
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
  virtual void report(std::string msg, uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way, uint32_t state) {
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

class DBSetDistMonitorType : public CacheDB<AccRecord> {
  uint32_t nset;
  uint64_t m_access;
  uint64_t period;
  std::list<set_dist_proc_t> processes;
public:
  DBSetDistMonitorType() : nset(0), m_access(0), period(0) {}

  void set_param(uint32_t nset, uint64_t period) {
    clear();
    this->nset = nset;
    this->period = period;
  }

  void add_process(set_dist_proc_t process) {
    processes.push_back(process);
  }

  virtual void access(int32_t idx) {
    m_access++;
    get(idx)->n_access++;
    if((m_access % period) == 0) {
      std::vector<uint64_t> access_vec(nset, 0);
      std::vector<uint64_t> evict_vec(nset, 0);
      for(uint32_t i=0; i<nset; i++) {
        auto r = get(i);
        access_vec[i] = r->n_access;
        evict_vec[i] = r->n_evict;
      }
      for(auto f:processes)
        f(m_access, access_vec, evict_vec);
      CacheDB<AccRecord>::clear();
    }
  }

  virtual void evict(int32_t idx) {
    get(idx)->n_evict++;
  }

  virtual void clear() {
    CacheDB<AccRecord>::clear();
    m_access = 0;
  }

  virtual ~DBSetDistMonitorType() {}
};

struct ReportDBs {
  std::unordered_map<uint64_t, DBAccType>   acc_dbs;     // record various access numbers
  std::unordered_map<uint64_t, DBAddrType>  addr_dbs;    // record the set/way pairs of an addr in a cache
  std::unordered_map<uint64_t, DBStateType> state_dbs;   // record the coherent status of something
  DBAddrTraceType                           addr_traces; // trace a group of specific address of interests
  DBSetTraceType                            set_traces;  // trace a group of specific sets of interests
  std::unordered_map<uint64_t, DBSetDistMonitorType> dist_dbs; // record distribution of sets
};

Reporter_t::Reporter_t()
  : dbs(new ReportDBs),
    wall_time(NULL),
    db_depth(4, false), db_type(5, false),
    paused(false) {}
Reporter_t::~Reporter_t() { delete dbs; }


// register recorders
void Reporter_t::register_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr, bool extra) {
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
    if(!dbs->acc_dbs.count(id)) {
      dbs->acc_dbs[id].detailed_to_addr = extra;
      db_depth[tracer_depth] = true;
      db_type[0] = true;
    }
    break;
  case 1: // address trace
    if(!dbs->addr_dbs.count(id)) {
      dbs->addr_dbs[id];
      db_depth[tracer_depth] = true;
      db_type[1] = true;
    }
    break;
  case 2: // state trace
    if(!dbs->state_dbs.count(id)) {
      dbs->state_dbs[id];
      db_depth[tracer_depth] = true;
      db_type[2] = true;
    }
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
  case 4: // set distribution
    if(!dbs->dist_dbs.count(id)) {
      dbs->dist_dbs[id];
      db_depth[tracer_depth] = true;
      db_type[4] = true;
    }
    break;
  default:
    return; // should not run to here
  }
}

void Reporter_t::add_reporter_generic(uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, const std::string &fn, uint64_t period) {
  uint64_t id;
  switch(tracer_depth) {
  case 0: id = hash(level); break;
  case 1: id = hash(level, core_id); break;
  case 2: id = hash(level, core_id, cache_id); break;
  default: id = 0; // should not run here
  }
  assert(dbs->acc_dbs.count(id));
  dbs->acc_dbs[id].set_reporter(fn, period);
}

// remove a certain recorder
void Reporter_t::remove_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr) {
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
  case 4: // set distribution
    assert(dbs->dist_dbs.count(id));
    dbs->dist_dbs.erase(id);
    break;
  default:
    return; // should not run to here
  }
}

// reset a certain recorder
void Reporter_t::reset_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr) {
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
  case 4: // set distribution
    assert(dbs->dist_dbs.count(id));
    dbs->dist_dbs[id].clear();
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
void Reporter_t::clear_distribution_traces() { dbs->dist_dbs.clear(); db_type[4] = false; }
void Reporter_t::clear() {
  clear_acc_dbs();
  clear_addr_dbs();
  clear_state_dbs();
  clear_addr_traces();
  clear_set_traces();
  clear_distribution_traces();
  db_depth = std::vector<bool>(4, false);
  db_type = std::vector<bool>(5, false);
}

  // event recorders
void Reporter_t::cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way, uint32_t state, bool hit, uint8_t rw) {
  if(paused) return;
  uint64_t record = addr_hash(addr);
  if(db_depth[0]) {
    uint64_t id = hash(level);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].access(record, hit, rw, wall_time);
    if(db_type[1] && dbs->addr_dbs.count(id))  dbs->addr_dbs[id].access(record, idx, way);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, state);
  }
  if(db_depth[1]) {
    uint64_t id = hash(level, core_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].access(record, hit, rw, wall_time);
    if(db_type[1] && dbs->addr_dbs.count(id))  dbs->addr_dbs[id].access(record, idx, way);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, state);
  }
  if(db_depth[2]) {
    uint64_t id = hash(level, core_id, cache_id);
    if(db_type[0] && dbs->acc_dbs.count(id))   dbs->acc_dbs[id].access(record, hit, rw, wall_time);
    if(db_type[1] && dbs->addr_dbs.count(id))  dbs->addr_dbs[id].access(record, idx, way);
    if(db_type[2] && dbs->state_dbs.count(id)) dbs->state_dbs[id].set_state(record, state);
    if(db_type[4] && dbs->dist_dbs.count(id))  dbs->dist_dbs[id].access(idx);
  }
  if(db_depth[3]) {
    uint64_t id = hash(level, core_id, cache_id, idx);
    if(db_type[0] && dbs->acc_dbs.count(id))  dbs->acc_dbs[id].access(record, hit, rw, wall_time);
    if(db_type[3] && dbs->set_traces.hit(id)) dbs->set_traces.access(id, addr);
  }
  if(db_type[3] && dbs->addr_traces.hit(addr)) {
    dbs->addr_traces.report("is accessed", level, core_id, cache_id, addr, idx, way, state);
  }
}

void Reporter_t::cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way) {
  if(paused) return;
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
    if(db_type[4] && dbs->dist_dbs.count(id)) dbs->dist_dbs[id].evict(idx);
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

void Reporter_t::cache_writeback(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way) {
  if(paused) return;
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

bool Reporter_t::add_set_dist_monitor(uint32_t level, int32_t core_id, int32_t cache_id,
                                      set_dist_proc_t process)
{
  uint64_t id = hash(level, core_id, cache_id);
  if(!dbs->dist_dbs.count(id)) return false;
  dbs->dist_dbs[id].add_process(process);
  return true;
}

bool Reporter_t::set_set_dist_tracer(uint32_t level, int32_t core_id, int32_t cache_id,
                                      uint32_t nset, uint64_t period)
{
  uint64_t id = hash(level, core_id, cache_id);
  if(!dbs->dist_dbs.count(id)) return false;
  dbs->dist_dbs[id].set_param(nset, period);
  return true;
}

// event checkers
bool Reporter_t::check_hit_generic(uint64_t id, uint64_t addr) const {
  if(dbs->state_dbs.count(id)) {
    return dbs->state_dbs.at(id).is_hit(addr_hash(addr));
  }
  return false;
}

bool Reporter_t::check_hit_generic(uint64_t id, uint64_t addr, uint32_t *level, int32_t *core_id, int32_t *cache_id, int32_t *idx, uint32_t *way) const {
  uint64_t record = addr_hash(addr);
  if(dbs->state_dbs.count(id)) {
    if(dbs->state_dbs.at(id).is_hit(record)) {
      dbs->state_dbs.at(id).get_location(record, level, core_id, cache_id, idx, way);
      return true;
    }
  }
  return false;
}

bool Reporter_t::check_hit(uint32_t *level, int32_t *core_id, int32_t *cache_id, uint64_t addr, int32_t *idx, uint32_t *way) const {
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

#define DEF_CHECK(T) \
  uint64_t Reporter_t::check_cache_##T##_generic(uint64_t id) const { \
    return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_##T() : 0; \
  } \
  uint64_t Reporter_t::check_addr_##T##_generic(uint64_t id, uint64_t addr) const { \
    return dbs->acc_dbs.count(id) ? dbs->acc_dbs.at(id).get_##T(addr_hash(addr)) : 0; \
  }

DEF_CHECK(access)
DEF_CHECK(read)
DEF_CHECK(write)
DEF_CHECK(read_miss)
DEF_CHECK(write_miss)
DEF_CHECK(hit)
DEF_CHECK(miss)
DEF_CHECK(evict)
DEF_CHECK(writeback)

#undef DEF_CHECK
