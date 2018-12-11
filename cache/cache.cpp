#include "cache/cache.hpp"

#ifdef DPRINT

std::set<uint64_t> l1_targets;
std::set<uint64_t> l2_targets;

#endif

void CacheBase::print_set(uint64_t addr) const {
  std::cout << cname << " set " << get_index(addr) << ":";
  for(unsigned int i=0; i<nway; i++) {
    uint64_t meta = get_meta(addr, i);
    if(CM::is_invalid(meta)) continue;
    std::cout << " " << CM::normalize(meta) <<
      (CM::is_shared(meta)? "(S" : "(_") <<
      (CM::is_modified(meta)? "M" : "_") <<
      (CM::is_dirty(meta)? "D)" : "_)");
  }
  std::cout << std::endl;
}

void CacheBase::print_line(uint64_t addr) const {
  std::cout << cname << " set " << get_index(addr);
  std::cout << (hit(addr) ? " h" : " m");
}

void L1CacheBase::probe(uint64_t addr, bool invalid) {
  uint32_t way = hit(addr);
  if(way) {
    way >>= 1;
    uint64_t meta = get_meta(addr, way);

    if(CM::is_modified(meta)) {
      uint32_t sel = llc_hasher->hash(addr);
      llc_caches[sel]->release(addr, id);
    }

    if(invalid) {
      set_meta(addr, way, CM::to_invalid(meta));
      replacer->invalid(get_index(addr), way);
      REPORT::evict_l1(addr, get_index(addr), way, id);
    } else {
      set_meta(addr, way, CM::to_shared(meta));
      replacer->access(get_index(addr), way);
      REPORT::access_l1(addr, get_index(addr), way, id);
    }


#ifdef DPRINT
    if(l1_targets.count(addr)) {
      std::cout << std::hex << "L1(" << id << ") probe to " <<
        (invalid ? "invalidate " : "share ") << addr << std::endl;
      print_set(addr);
    }
#endif
  }
}

uint32_t L1CacheBase::fetch_read(uint64_t addr) {
  uint32_t way = replace(addr);
  uint32_t sel = llc_hasher->hash(addr);
  llc_caches[sel]->fetch_read(addr, id);
  set_meta(addr, way, CM::to_shared(addr));
  return way;
}

uint32_t L1CacheBase::fetch_write(uint64_t addr, bool perm_only = false) {
  uint32_t way = perm_only ? hit(addr) >> 1 : replace(addr);
  uint32_t sel = llc_hasher->hash(addr);
  llc_caches[sel]->fetch_write(addr, id);
  set_meta(addr, way, CM::to_modified(addr));
  return way;
}

uint32_t L1CacheBase::replace(uint64_t addr) {
  uint32_t idx = get_index(addr);
  uint32_t way = replacer->replace(idx);
  uint64_t meta = get_meta(addr, way);
  uint64_t rep_addr = CM::normalize(meta);   // we know meta has the full address except for the lowest 6 bits
  if(CM::is_modified(meta)) {
    uint32_t sel = llc_hasher->hash(rep_addr);
    llc_caches[sel]->release(rep_addr, id);
  }
  if(!CM::is_invalid(meta)) {
    replacer->invalid(get_index(rep_addr), way);
    REPORT::evict_l1(rep_addr, get_index(rep_addr), way, id);
  }
  return way;
}

uint32_t LLCCacheBase::replace(uint64_t addr) {
  uint32_t way = replacer->replace(get_index(addr));
  uint64_t meta = get_meta(addr, way);
  uint64_t rep_addr = CM::normalize(meta); // we know meta has the full address except for the lowest 6 bits
  if(!CM::is_invalid(meta)) {
    probe(rep_addr, -1, true);
    if(CM::is_dirty(meta)) extern_mem->write(rep_addr, id);
    replacer->invalid(get_index(rep_addr), way);
    REPORT::evict_llc(rep_addr, get_index(rep_addr), way, id);
#ifdef DPRINT
      if(l2_targets.count(rep_addr)) {
        std::cout << std::hex << "L2(" << id << ") replace " << rep_addr << std::endl;
        print_set(addr);
      }
#endif
  }
  return way;
}

void LLCCacheBase::probe(uint64_t addr, uint32_t id, bool invalidate) {
  for (uint32_t i=0; i<NL1; i++)
    if(id != i || !invalidate) l1_caches[i]->probe(addr, invalidate);
}

