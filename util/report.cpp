#include <map>
#include <set>
#include <vector>
#include "cache/definitions.hpp"
#include "cache/cache.hpp"
#include "util/report.hpp"

struct cache_address_record_t {
  int llc;
  std::set<uint32_t> l1;
  uint32_t l1_accessed, l1_evicted;
  uint32_t llc_accessed, llc_evicted;

  cache_address_record_t() :
    llc(-1), l1_accessed(0), l1_evicted(0), llc_accessed(0), llc_evicted(0) {}

  void clear() {
    l1.clear();
    llc = -1;
    l1_accessed = 0;
    l1_evicted = 0;
    llc_accessed = 0;
    llc_evicted = 0;
  }
};

struct cache_line_record_t {
  uint32_t accessed;
  uint32_t evicted;

  cache_line_record_t() : accessed(0), evicted(0) {}

  void clear() {
    accessed = 0;
    evicted = 0;
  }

  void clear_evicted() {
    evicted = 0;
  }
};

template<uint32_t nway>
struct cache_set_record_t {
  cache_line_record_t ways[nway];
  uint32_t accessed, evicted;

  cache_set_record_t() : accessed(0), evicted(0) {}

  void clear() {
    for(int i=0; i<nway; i++) ways[i].clear();
    accessed = 0;
    evicted = 0;
  }

  void clear_evicted() {
    for(int i=0; i<nway; i++) ways[i].clear_evicted();
    accessed = 0;
  }
};

template<uint32_t nset, uint32_t nway>
struct cache_record_t {
  cache_set_record_t<nway> sets[nset];
  uint32_t accessed, evicted;
  int32_t max_accessed_set;
  int32_t max_evicted_set;

  cache_record_t() : accessed(0), evicted(0), max_accessed_set(-1), max_evicted_set(-1) {}

  void clear() {
    for(int i=0; i<nset; i++) sets[i].clear();
    accessed = 0;
    evicted = 0;
    max_accessed_set = -1;
    max_evicted_set = -1;
  }

  void clear_evicted() {
    for(int i=0; i<nset; i++) sets[i].clear_evicted();
    evicted = 0;
    max_evicted_set = -1;
  }
};

std::map<uint64_t, cache_address_record_t> addrDB;
cache_record_t<NL1Set, NL1Way> l1DB[NL1];
cache_record_t<NLLCSet, NLLCWay> llcDB[NLLC];

void REPORT::access_l1(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id) {
  addr = CM::normalize(addr);
  l1DB[id].accessed++;
  l1DB[id].sets[idx].accessed++;
  if(l1DB[id].max_accessed_set == -1 ||
     l1DB[id].sets[l1DB[id].max_accessed_set].accessed < l1DB[id].sets[idx].accessed)
    l1DB[id].max_accessed_set = idx;
  l1DB[id].sets[idx].ways[way].accessed++;
  addrDB[addr].l1.insert(id);
  addrDB[addr].l1_accessed++;
}

void REPORT::access_llc(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id) {
  addr = CM::normalize(addr);
  llcDB[id].accessed++;
  llcDB[id].sets[idx].accessed++;
  if(llcDB[id].max_accessed_set == -1 ||
     llcDB[id].sets[llcDB[id].max_accessed_set].accessed < llcDB[id].sets[idx].accessed)
    llcDB[id].max_accessed_set = idx;
  llcDB[id].sets[idx].ways[way].accessed++;
  addrDB[addr].llc = id;
  addrDB[addr].llc_accessed++;
}

void REPORT::evict_l1(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id) {
  addr = CM::normalize(addr);
  l1DB[id].evicted++;
  l1DB[id].sets[idx].evicted++;
  if(l1DB[id].max_evicted_set == -1 ||
     l1DB[id].sets[l1DB[id].max_evicted_set].evicted < l1DB[id].sets[idx].evicted)
    l1DB[id].max_evicted_set = idx;
  l1DB[id].sets[idx].ways[way].evicted++;
  addrDB[addr].l1.erase(id);
  addrDB[addr].l1_evicted++;
}

void REPORT::evict_llc(uint64_t addr, uint32_t idx, uint32_t way, uint32_t id) {
  addr = CM::normalize(addr);
  llcDB[id].evicted++;
  llcDB[id].sets[idx].evicted++;
  if(llcDB[id].max_evicted_set == -1 ||
     llcDB[id].sets[llcDB[id].max_evicted_set].evicted < llcDB[id].sets[idx].evicted)
    llcDB[id].max_evicted_set = idx;
  llcDB[id].sets[idx].ways[way].evicted++;
  addrDB[addr].llc = -1;
  addrDB[addr].llc_evicted++;
}

int REPORT::count_l1(uint64_t addr) {
  addr = CM::normalize(addr);
  if(addrDB.count(addr)) {
    if(addrDB[addr].l1.empty()) return -1;
    else return *(addrDB[addr].l1.begin());
  } else return -1;
}

bool REPORT::count_llc(uint64_t addr) {
  addr = CM::normalize(addr);
  if(addrDB.count(addr)) {
    if(addrDB[addr].llc == -1) return false;
    else return true;
  } else return false;
}

void REPORT::clear() {
  clear_addr();
  clear_l1(-1);
  clear_llc(-1);
}

void REPORT::clear_addr() {
  addrDB.clear();
}

void REPORT::clear_evicted() {
  clear_l1_evicted(-1);
  clear_llc_evicted(-1);
}

void REPORT::clear_l1(int id) {
  if(id != -1) l1DB[id].clear();
  else
    for(int i=0; i<NL1; i++) l1DB[i].clear();
}

void REPORT::clear_l1_evicted(int id) {
  if(id != -1) l1DB[id].clear_evicted();
  else
    for(int i=0; i<NL1; i++) l1DB[i].clear_evicted();
}

void REPORT::clear_llc(int id) {
  if(id != -1) llcDB[id].clear();
  else
    for(int i=0; i<NLLC; i++) llcDB[i].clear();
}

void REPORT::clear_llc_evicted(int id) {
  if(id != -1) llcDB[id].clear_evicted();
  else
    for(int i=0; i<NLLC; i++) llcDB[i].clear_evicted();
}

uint32_t REPORT::get_l1_accessed(uint32_t id) {
  return l1DB[id].accessed;
}

uint32_t REPORT::get_l1_set_accessed(uint32_t idx, uint32_t id) {
  return l1DB[id].sets[idx].accessed;
}

uint32_t REPORT::get_l1_addr_accessed(uint64_t addr) {
  addr = CM::normalize(addr);
  if(addrDB.count(addr))
    return addrDB[addr].l1_accessed;
  else
    return 0;
}

uint32_t REPORT::get_llc_accessed() {
  uint32_t rv = 0;
  for(int i=0; i<NLLC; i++) rv += llcDB[i].accessed;
  return rv;
}

uint32_t REPORT::get_llc_accessed(uint32_t id) {
  return llcDB[id].accessed;
}

uint32_t REPORT::get_llc_set_accessed(uint32_t idx, uint32_t id) {
  return llcDB[id].sets[idx].accessed;
}

uint32_t REPORT::get_llc_addr_accessed(uint64_t addr) {
  addr = CM::normalize(addr);
  if(addrDB.count(addr))
    return addrDB[addr].llc_accessed;
  else
    return 0;
}

uint32_t REPORT::get_l1_evicted(uint32_t id) {
  return l1DB[id].evicted;
}

uint32_t REPORT::get_l1_set_evicted(uint32_t idx, uint32_t id) {
  return l1DB[id].sets[idx].evicted;
}

uint32_t REPORT::get_l1_addr_evicted(uint64_t addr) {
  addr = CM::normalize(addr);
  if(addrDB.count(addr))
    return addrDB[addr].l1_evicted;
  else
    return 0;
}

uint32_t REPORT::get_llc_evicted() {
  uint32_t rv = 0;
  for(int i=0; i<NLLC; i++) rv += llcDB[i].evicted;
  return rv;
}

uint32_t REPORT::get_llc_evicted(uint32_t id) {
  return llcDB[id].evicted;
}

uint32_t REPORT::get_llc_set_evicted(uint32_t idx, uint32_t id) {
  return llcDB[id].sets[idx].evicted;
}

uint32_t REPORT::get_llc_addr_evicted(uint64_t addr) {
  addr = CM::normalize(addr);
  if(addrDB.count(addr))
    return addrDB[addr].llc_evicted;
  else
    return 0;
}

uint32_t REPORT::get_l1_evicted_max(uint32_t id) {
  if(l1DB[id].max_evicted_set == -1) return 0;
  else return l1DB[id].sets[l1DB[id].max_evicted_set].evicted;
}

uint32_t REPORT::get_llc_evicted_max() {
  uint32_t rv = 0;
  for(int i=0; i<NLLC; i++) {
    if(llcDB[i].max_evicted_set != -1 && llcDB[i].sets[llcDB[i].max_evicted_set].evicted > rv)
      rv = llcDB[i].sets[llcDB[i].max_evicted_set].evicted;
  }
  return rv;
}
