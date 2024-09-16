#ifndef UTIL_REPORT_HPP_
#define UTIL_REPORT_HPP_

#include <cstdint>
#include <vector>
#include <list>
#include <functional>
#include "util/detector.hpp"

class ReportDBs;

class Reporter_t
{
  ReportDBs *dbs;
  uint64_t *wall_time;

  inline uint64_t hash(uint32_t level) const {
    return
      (uint64_t)(0xff ^ level)<<56;
  }

  inline uint64_t hash(uint32_t level, int32_t core_id) const {
    return
      (((uint64_t)(0xff  ^ level  ))<<56) |
      (((uint64_t)(0xfff ^ core_id))<<44);
  }

  inline uint64_t hash(uint32_t level, int32_t core_id, int32_t cache_id) const {
    return
      (((uint64_t)(0xff  ^ level   ))<<56) |
      (((uint64_t)(0xfff ^ core_id ))<<44) |
      (((uint64_t)(0xfff ^ cache_id))<<32);
  }

  inline uint64_t hash(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx) const {
    return
      (((uint64_t)(0xff   ^ level   ))<<56) |
      (((uint64_t)(0xfff  ^ core_id ))<<44) |
      (((uint64_t)(0xfff  ^ cache_id))<<32) |
      (((uint64_t)(0xffff ^ idx     ))<<16);
  }

  inline uint64_t hash(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint32_t way) const {
    return
      (((uint64_t)(0xff   ^ level   ))<<56) |
      (((uint64_t)(0xfff  ^ core_id ))<<44) |
      (((uint64_t)(0xfff  ^ cache_id))<<32) |
      (((uint64_t)(0xffff ^ idx     ))<<16) |
                  (0xffff ^ way     );
  }

  inline uint64_t addr_hash(uint64_t addr) const {
    return addr;
  }

  void register_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr, bool extra);
  void add_reporter_generic(uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, const std::string &fn, uint64_t period);
  void remove_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr);
  void reset_tracer_generic(uint32_t tracer_type, uint32_t tracer_depth, uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr);
  bool check_hit_generic(uint64_t id, uint64_t addr) const;
  bool check_hit_generic(uint64_t id, uint64_t addr, uint32_t *level, int32_t *core_id, int32_t *cache_id, int32_t *idx, uint32_t *way) const;

  #define DEF_CHECK(T) \
    uint64_t check_cache_##T##_generic(uint64_t id) const; \
    uint64_t check_addr_##T##_generic(uint64_t id, uint64_t addr) const;

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

  std::vector<bool> db_depth;
  std::vector<bool> db_type;

  bool paused;

public:
  Reporter_t();
  ~Reporter_t();

  void set_wall_time(uint64_t *t) { wall_time = t; }

  // register recorders
  inline void register_cache_access_tracer(uint32_t level, bool detailed_to_addr = false) {
    register_tracer_generic(0, 0, level, 0, 0, 0, 0, detailed_to_addr);
  }
  inline void register_cache_access_tracer(uint32_t level, int32_t core_id, bool detailed_to_addr = false) {
    register_tracer_generic(0, 1, level, core_id, 0, 0, 0, detailed_to_addr);
  }
  inline void register_cache_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id, bool detailed_to_addr = false) {
    register_tracer_generic(0, 2, level, core_id, cache_id, 0, 0, detailed_to_addr);
  }
  inline void add_cache_access_reporter(uint32_t level, const std::string &fn, uint64_t period) {
    add_reporter_generic(0, level, 0, 0, fn, period);
  }
  inline void add_cache_access_reporter(uint32_t level, int32_t core_id, const std::string &fn, uint64_t period) {
    add_reporter_generic(1, level, core_id, 0, fn, period);
  }
  inline void add_cache_access_reporter(uint32_t level, int32_t core_id, int32_t cache_id, const std::string &fn, uint64_t period) {
    add_reporter_generic(2, level, core_id, cache_id, fn, period);
  }
  inline void register_set_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, bool detailed_to_addr = false) {
    register_tracer_generic(0, 3, level, core_id, cache_id, idx, 0, detailed_to_addr);
  }
  inline void register_set_dist_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    register_tracer_generic(4, 2, level, core_id, cache_id, 0, 0, false);
  }
  bool set_set_dist_tracer(uint32_t level, int32_t core_id, int32_t cache_id,
                           uint32_t nset, uint64_t period);
  bool add_set_dist_monitor(uint32_t level, int32_t core_id, int32_t cache_id,
                            set_dist_proc_t process);
  inline void register_cache_addr_tracer(uint32_t level) {
    register_tracer_generic(1, 0, level, 0, 0, 0, 0, false);
  }
  inline void register_cache_addr_tracer(uint32_t level, int32_t core_id) {
    register_tracer_generic(1, 1, level, core_id, 0, 0, 0, false);
  }
  inline void register_cache_addr_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    register_tracer_generic(1, 2, level, core_id, cache_id, 0, 0, false);
  }
  inline void register_cache_state_tracer(uint32_t level) {
    register_tracer_generic(2, 0, level, 0, 0, 0, 0, false);
  }
  inline void register_cache_state_tracer(uint32_t level, int32_t core_id) {
    register_tracer_generic(2, 1, level, core_id, 0, 0, 0, false);
  }
  inline void register_cache_state_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    register_tracer_generic(2, 2, level, core_id, cache_id, 0, 0, false);
  }
  inline void register_address_tracer(uint64_t addr) {
    register_tracer_generic(3, 0, 0, 0, 0, 0, addr, false);
  }
  inline void register_set_tracer(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx) {
    register_tracer_generic(3, 3, level, core_id, cache_id, idx, 0, false);
  }

  // remove recorders
  inline void remove_cache_access_tracer(uint32_t level) {
    remove_tracer_generic(0, 0, level, 0, 0, 0, 0);
  }
  inline void remove_cache_access_tracer(uint32_t level, int32_t core_id) {
    remove_tracer_generic(0, 1, level, core_id, 0, 0, 0);
  }
  inline void remove_cache_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    remove_tracer_generic(0, 2, level, core_id, cache_id, 0, 0);
  }
  inline void remove_set_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx) {
    remove_tracer_generic(0, 3, level, core_id, cache_id, idx, 0);
  }
  inline void remove_set_dist_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    remove_tracer_generic(4, 2, level, core_id, cache_id, 0, 0);
  }
  inline void remove_cache_addr_tracer(uint32_t level) {
    remove_tracer_generic(1, 0, level, 0, 0, 0, 0);
  }
  inline void remove_cache_addr_tracer(uint32_t level, int32_t core_id) {
    remove_tracer_generic(1, 1, level, core_id, 0, 0, 0);
  }
  inline void remove_cache_addr_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    remove_tracer_generic(1, 2, level, core_id, cache_id, 0, 0);
  }
  inline void remove_cache_state_tracer(uint32_t level) {
    remove_tracer_generic(2, 0, level, 0, 0, 0, 0);
  }
  inline void remove_cache_state_tracer(uint32_t level, int32_t core_id) {
    remove_tracer_generic(2, 1, level, core_id, 0, 0, 0);
  }
  inline void remove_cache_state_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    remove_tracer_generic(2, 2, level, core_id, cache_id, 0, 0);
  }
  inline void remove_address_tracer(uint64_t addr) {
    remove_tracer_generic(3, 0, 0, 0, 0, 0, addr);
  }
  inline void remove_set_tracer(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx) {
    remove_tracer_generic(3, 3, level, core_id, cache_id, idx, 0);
  }

  // reset recorders
  inline void reset_cache_access_tracer(uint32_t level) {
    reset_tracer_generic(0, 0, level, 0, 0, 0, 0);
  }
  inline void reset_cache_access_tracer(uint32_t level, int32_t core_id) {
    reset_tracer_generic(0, 1, level, core_id, 0, 0, 0);
  }
  inline void reset_cache_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    reset_tracer_generic(0, 2, level, core_id, cache_id, 0, 0);
  }
  inline void reset_set_access_tracer(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx) {
    reset_tracer_generic(0, 3, level, core_id, cache_id, idx, 0);
  }
  inline void reset_set_dist_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    reset_tracer_generic(4, 2, level, core_id, cache_id, 0, 0);
  }
  inline void reset_cache_addr_tracer(uint32_t level) {
    reset_tracer_generic(1, 0, level, 0, 0, 0, 0);
  }
  inline void reset_cache_addr_tracer(uint32_t level, int32_t core_id) {
    reset_tracer_generic(1, 1, level, core_id, 0, 0, 0);
  }
  inline void reset_cache_addr_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    reset_tracer_generic(1, 2, level, core_id, cache_id, 0, 0);
  }
  inline void reset_cache_state_tracer(uint32_t level) {
    reset_tracer_generic(2, 0, level, 0, 0, 0, 0);
  }
  inline void reset_cache_state_tracer(uint32_t level, int32_t core_id) {
    reset_tracer_generic(2, 1, level, core_id, 0, 0, 0);
  }
  inline void reset_cache_state_tracer(uint32_t level, int32_t core_id, int32_t cache_id) {
    reset_tracer_generic(2, 2, level, core_id, cache_id, 0, 0);
  }

  // run-control
  void pause()  { paused = true; }
  void resume() { paused = false; }

  // clear recorders
  void clear_acc_dbs();
  void clear_addr_dbs();
  void clear_state_dbs();
  void clear_addr_traces();
  void clear_set_traces();
  void clear_distribution_traces();
  void clear();

  // event recorders
  void cache_access(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way, uint32_t state, bool hit, uint8_t rw);
  void cache_evict(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way);
  void cache_writeback(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t idx, uint32_t way);

  // event checkers
  inline bool check_hit(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr, int32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level, core_id, cache_id), addr, &level, &core_id, &cache_id, idx, way);
  }
  inline bool check_hit(uint32_t level, int32_t core_id, int32_t *cache_id, uint64_t addr, int32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level, core_id), addr, &level, &core_id, cache_id, idx, way);
  }
  inline bool check_hit(uint32_t level, int32_t *core_id, int32_t *cache_id, uint64_t addr, int32_t *idx, uint32_t *way) const {
    return check_hit_generic(hash(level), addr, &level, core_id, cache_id, idx, way);
  }
  bool check_hit(uint32_t *level, int32_t *core_id, int32_t *cache_id, uint64_t addr, int32_t *idx, uint32_t *way) const;
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

  // access related checks
  #define CHECK_C_L5(T) \
    inline uint64_t check_cache_##T(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint32_t way) const { \
      return check_cache_##T##_generic(hash(level, core_id, cache_id, idx, way)); \
    }
  #define CHECK_C_L4(T) \
    inline uint64_t check_cache_##T(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx) const { \
      return check_cache_##T##_generic(hash(level, core_id, cache_id, idx)); \
    }
  #define CHECK_C_L3(T) \
    inline uint64_t check_cache_##T(uint32_t level, int32_t core_id, int32_t cache_id) const { \
      return check_cache_##T##_generic(hash(level, core_id, cache_id)); \
    }
  #define CHECK_C_L2(T) \
    inline uint64_t check_cache_##T(uint32_t level, int32_t core_id) const { \
      return check_cache_##T##_generic(hash(level, core_id)); \
    }
  #define CHECK_C_L1(T) \
    inline uint64_t check_cache_##T(uint32_t level) const { \
      return check_cache_##T##_generic(hash(level)); \
    }
  #define CHECK_A_L5(T) \
    inline uint64_t check_addr_##T(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint32_t way, uint64_t addr) const { \
      return check_addr_##T##_generic(hash(level, core_id, cache_id, idx, way), addr); \
    }
  #define CHECK_A_L4(T) \
    inline uint64_t check_addr_##T(uint32_t level, int32_t core_id, int32_t cache_id, int32_t idx, uint64_t addr) const { \
      return check_addr_##T##_generic(hash(level, core_id, cache_id, idx), addr); \
    }
  #define CHECK_A_L3(T) \
    inline uint64_t check_addr_##T(uint32_t level, int32_t core_id, int32_t cache_id, uint64_t addr) const { \
      return check_addr_##T##_generic(hash(level, core_id, cache_id), addr); \
    }
  #define CHECK_A_L2(T) \
    inline uint64_t check_addr_##T(uint32_t level, int32_t core_id, uint64_t addr) const { \
      return check_addr_##T##_generic(hash(level, core_id), addr); \
    }
  #define CHECK_A_L1(T) \
    inline uint64_t check_addr_##T(uint32_t level, uint64_t addr) const { \
      return check_addr_##T##_generic(hash(level), addr); \
    }
  #define CHECK_C(T) CHECK_C_L1(T) CHECK_C_L2(T) CHECK_C_L3(T) CHECK_C_L4(T) CHECK_C_L5(T)
  #define CHECK_A(T) CHECK_A_L1(T) CHECK_A_L2(T) CHECK_A_L3(T) CHECK_A_L4(T) CHECK_A_L5(T)
  #define CHECK_FUNC(T) CHECK_C(T) CHECK_A(T)

  CHECK_FUNC(access)
  CHECK_FUNC(read)
  CHECK_FUNC(write)
  CHECK_FUNC(read_miss)
  CHECK_FUNC(write_miss)
  CHECK_FUNC(hit)
  CHECK_FUNC(miss)
  CHECK_FUNC(evict)
  CHECK_FUNC(writeback)

  #undef CHECK_C_L5
  #undef CHECK_C_L4
  #undef CHECK_C_L3
  #undef CHECK_C_L2
  #undef CHECK_C_L1
  #undef CHECK_A_L5
  #undef CHECK_A_L4
  #undef CHECK_A_L3
  #undef CHECK_A_L2
  #undef CHECK_A_L1
  #undef CHECK_C
  #undef CHECK_A
  #undef CHECK_FUNC

};

#endif
