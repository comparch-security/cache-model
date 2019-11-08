#include "util/query.hpp"
#include "cache/cache.hpp"
#include <boost/format.hpp>
#include <iostream>

std::string CBInfo::to_string() const {
  std::string state;
  if(invalid()) state = "I";
  if(shared()) state = "S";
  if(modified()) state = "M";
  if(dirty()) state += "(D)";
  auto fmt = boost::format("0x%016x %2%") % addr() % state;
  return fmt.str();
}

std::string SetInfo::to_string() const {
  std::string rv;
  auto it = ways.begin();
  while(true) {
    rv += it->to_string();
    it++;
    if(it != ways.end()) rv += " ";
    else break;
  }
  return rv;
}

std::string LocRange::to_string() const {
  if(range.first == range.second) {
    auto fmt = boost::format("[%1%]") % range.first;
    return fmt.str();
  } else {
    auto fmt = boost::format("[%1%:%2%]") % range.first % range.second;
    return fmt.str();
  }
}

std::string LocInfo::to_string() const {
  std::string rv = cache->cache_name() + ": ";
  auto it = locs.begin();
  while(true) {
    rv += std::to_string(it->first) + it->second.to_string();
    it++;
    if(it != locs.end()) rv += ", ";
    else { rv += "."; break; }
  }
  return rv;
}

bool query_hit(uint64_t addr, CacheBase *cache) {
  return cache->hit(addr);
}

uint32_t query_coloc(uint64_t addr, CacheBase *cache, std::unordered_set<uint64_t> evset) {
  uint32_t rv = 0;
  for(auto a:evset) if(cache->query_coloc(addr, a)) rv++;
  return rv;
}

bool query_check(uint64_t addr, CacheBase *cache, std::unordered_set<uint64_t> evset) {
  for(auto a:evset) if(!cache->query_coloc(addr, a)) return false;
  return true;
}

void print_locs(const std::list<LocInfo> &locs, uint32_t indent) {
  for(auto c: locs)
    std::cout << std::string(indent, ' ') << c.to_string() << std::endl;
}
