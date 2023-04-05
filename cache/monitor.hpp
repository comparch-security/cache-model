#ifndef CM_MONITOR_HPP_
#define CM_MONITOR_HPP_

#include "cache/definitions.hpp"
#include <list>
#include <vector>
#include <cmath>

class CacheMonitorGlobalQueue
{
  static std::list<CoherentCache *> objects; // the cache objects to operate
public:
  static void add(CoherentCache *obj) {
    objects.push_back(obj);
  }
  static void trigger();
};

class CacheMonitorBase
{
protected:
  CoherentCache *cache;
  uint64_t *wall_time;
public:
  CacheMonitorBase(CoherentCache *p, uint64_t *wt) : cache(p), wall_time(wt) {}
  virtual ~CacheMonitorBase() {}
  virtual void access_event(uint64_t addr, int32_t idx, uint32_t way) = 0;
  virtual void evict_event(uint64_t addr, int32_t idx, uint32_t way) = 0;
  virtual void clean_history() = 0;
};

class CacheMonitorList
{
protected:
  std::list<CacheMonitorBase*> monitors;
public:
  CacheMonitorList() {}
  virtual void access_event(uint64_t addr, int32_t idx, uint32_t way) {
    for(auto m:monitors) m->access_event(addr, idx, way);
  }
  virtual void evict_event(uint64_t addr, int32_t idx, uint32_t way) {
    for(auto m:monitors) m->evict_event(addr, idx, way);
  }
  virtual void clean_monitor_history() {
    for(auto m:monitors) m->clean_history();
  }

  void add_monitor(CacheMonitorBase *m) {
    monitors.push_back(m);
  }

  virtual ~CacheMonitorList() {
    for(auto m:monitors) delete(m);
  }
};

class SetCSAMonitor : public CacheMonitorBase
{
protected:
  const double factor;
  const uint32_t nset;
  const double threshold;
  const uint64_t access_period, evict_period;
  std::vector<uint64_t> evicts;
  std::vector<double> set_evict_history;
  uint64_t accesses;
  uint64_t total_evicts;
  bool remap_enable;

  bool set_evict_detect();

public:
  SetCSAMonitor(CoherentCache *p, const double factor, uint32_t nset, uint32_t nway, uint64_t access_period, uint64_t evict_period, double th, bool re = false, uint64_t *wt = NULL)
    : CacheMonitorBase(p, wt), factor(factor), nset(nset), threshold(th),
      access_period(access_period), evict_period(evict_period), evicts(std::vector<uint64_t>(nset, 0)),
      set_evict_history(std::vector<double>(nset, 0.0)),
      accesses(0), total_evicts(0), remap_enable(re) {}

  virtual void access_event(uint64_t addr, int32_t idx, uint32_t way);
  virtual void evict_event(uint64_t addr, int32_t idx, uint32_t way);
  virtual ~SetCSAMonitor() {}
  virtual void clean_history();
};

#endif
