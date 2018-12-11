#ifndef UTIL_REPORT_HPP_
#define UTIL_REPORT_HPP_

#include <cstdint>

class REPORT
{
public:

  // report that a line is accessed (read/write) in a cache
  static void access_l1(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id);
  static void access_llc(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id);

  // report that a line is evicted from a cache
  static void evict_l1(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id);
  static void evict_llc(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id);

  // whether a line is cached in a cache
  static int count_l1(uint64_t addr);
  static bool count_llc(uint64_t addr);

  // clear various part of the report database
  static void clear();
  static void clear_addr();
  static void clear_evicted();
  static void clear_l1(int id = -1);
  static void clear_l1_evicted(int id = -1);
  static void clear_llc(int id = -1);
  static void clear_llc_evicted(int id = -1);

  // report
  static uint32_t get_l1_accessed(uint32_t id);
  static uint32_t get_l1_set_accessed(uint32_t idx, uint32_t id);
  static uint32_t get_l1_addr_accessed(uint64_t addr);
  static uint32_t get_llc_accessed();
  static uint32_t get_llc_accessed(uint32_t id);
  static uint32_t get_llc_set_accessed(uint32_t idx, uint32_t id);
  static uint32_t get_llc_addr_accessed(uint64_t addr);
  static uint32_t get_l1_evicted(uint32_t id);
  static uint32_t get_l1_set_evicted(uint32_t idx, uint32_t id);
  static uint32_t get_l1_addr_evicted(uint64_t addr);
  static uint32_t get_llc_evicted();
  static uint32_t get_llc_evicted(uint32_t id);
  static uint32_t get_llc_set_evicted(uint32_t idx, uint32_t id);
  static uint32_t get_llc_addr_evicted(uint64_t addr);

  static uint32_t get_l1_evicted_max(uint32_t id);
  static uint32_t get_llc_evicted_max();
};

#endif
