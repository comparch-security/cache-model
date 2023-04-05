#include "cache/monitor.hpp"
#include "cache/cache.hpp"
//#include <iostream>
#include <numeric>

std::list<CoherentCache *> CacheMonitorGlobalQueue::objects;

void CacheMonitorGlobalQueue::trigger() {
  while(!objects.empty()) {
    objects.front()->remap();
    objects.front()->clean_monitor_history();
    objects.pop_front();
  }
}

bool SetCSAMonitor::set_evict_detect() {
  double qrm  = sqrt(std::accumulate(evicts.begin(), evicts.end(), 0.0,
                                     [](double q, const uint64_t& d){return q + d * d;})
                     / (nset-1.0)
                     );
  double mu   = std::accumulate(evicts.begin(), evicts.end(), 0.0) / (double)(nset);
  for(size_t i=0; i<nset; i++) {
    double delta = qrm == 0.0 ? 0.0 : (evicts[i] - mu) * (evicts[i] - mu) / qrm;
    set_evict_history[i] = factor * set_evict_history[i] +
      (evicts[i] > mu ? delta : -delta);
  }

  for(size_t i=0; i<nset; i++)
    if(set_evict_history[i] >= threshold) {
      //std::cerr << "found set " << i << std::endl;
      return true;
    }

  return false;
}

void SetCSAMonitor::access_event(uint64_t addr, int32_t idx, uint32_t way) {
  accesses++;
  if(remap_enable && access_period != 0 && (accesses % access_period) == 0) {
    if(set_evict_detect()) {
      //if(wall_time)
      //  std::cerr << *wall_time << ":";
      //std::cerr << " remapped by detect @" << accesses  << std::endl;
      CacheMonitorGlobalQueue::add(cache);
    }
    evicts = std::vector<uint64_t>(nset, 0);
  }
}

void SetCSAMonitor::evict_event(uint64_t addr, int32_t idx, uint32_t way) {
  total_evicts++;
  evicts[idx]++;
  if(remap_enable && evict_period != 0 && (total_evicts % evict_period) == 0) {
    //if(wall_time)
    //    std::cerr << *wall_time << ":";
    //  std::cerr << " remapped by eviction limit @" << total_evicts  << std::endl;
    CacheMonitorGlobalQueue::add(cache);
  }
}

void SetCSAMonitor::clean_history() {
  evicts = std::vector<uint64_t>(nset, 0);
  set_evict_history = std::vector<double>(nset, 0.0);
  if(access_period != 0) accesses -= (accesses % access_period);
  if(evict_period != 0) total_evicts -= (total_evicts % evict_period);
}
