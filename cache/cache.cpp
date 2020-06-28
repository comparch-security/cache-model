#include "cache/cache.hpp"
#include "util/query.hpp"
#include "util/report.hpp"
#include <boost/format.hpp>

const uint32_t mem_delay = 200;  // currently this is a hack

std::string CacheBase::cache_name() const {
  if(core_id < 0) {
    auto fmt = boost::format("L%1%(%2%)") % level % cache_id;
    return fmt.str();
  } else if(level == 1) {
    auto fmt = boost::format("Core(%1%)-L1(%2%)") % core_id % cache_id;
    return fmt.str();
  } else if(level == 2 && cache_id == 0) {
    auto fmt = boost::format("Core(%1%)-L2") % core_id;
    return fmt.str();
  } else {
    auto fmt = boost::format("Level(%1%) Core(%2%) ID(%3%)") % level % core_id % cache_id;
    throw std::runtime_error("unsupported cache identifier: " + fmt.str());
    return std::string();
  }
}

void CacheBase::query_block(uint32_t idx, uint32_t way, CBInfo *info) const {
  *info = CBInfo(get_meta(NULL, idx, way));
}

void CacheBase::query_set(uint32_t idx, SetInfo *info) const {
  info->ways.resize(nway);
  for(int i=0; i<nway; i++) info->ways[i] = CBInfo(get_meta(NULL, idx, i));
}

LocInfo CacheBase::query_loc(uint64_t addr) {
  LocInfo rv(level, core_id, cache_id, this);
  rv.insert(get_index(NULL, addr), LocRange(0, nway-1));
  return rv;
}

bool CacheBase::query_coloc(uint64_t addrA, uint64_t addrB) {
  return get_index(NULL, addrA) == get_index(NULL, addrB);
}

void CoherentCache::replace(uint64_t *latency, uint64_t addr, uint32_t *idx, uint32_t *way) {
  *idx = cache->get_index(latency, addr);
  *way = cache->replace(latency, *idx);
  evict(latency, *idx, *way);
}

void CoherentCache::flush(uint64_t *latency, uint64_t addr, int32_t levels, uint32_t inner_id) {
  uint32_t idx, way;
  addr = CM::normalize(addr);
  cache->latency_acc(latency);
  if(cache->hit(latency, addr, &idx, &way))
    evict(latency, idx, way);
  if(levels != 0 && outer_caches)
    outer_flush(latency, id, addr, levels-1);
}

void CoherentCache::flush_cache(uint64_t *latency, int32_t levels, uint32_t inner_id) {
  for(uint32_t idx=0; idx<cache->nset; idx++)
    for(uint32_t way=0; way<cache->nway; way++)
      evict(latency, idx, way);
  if(levels != 0 && outer_caches)
    outer_flush_cache(latency, id, levels-1);
}


void CoherentCache::read(uint64_t *latency, uint64_t addr, uint32_t inner_id) {
  addr = CM::normalize(addr);
  uint32_t idx, way;
  bool h = true;
  cache->latency_acc(latency);
  if(cache->hit(latency, addr, &idx, &way)) { // hit
    if(inner_caches && CM::is_modified(cache->get_meta(NULL, idx, way))) {
      inner_probe(latency, inner_id, addr, id, false, false);
      cache->set_meta(latency, idx, way, CM::to_shared(cache->get_meta(NULL, idx, way)));
    }
  } else {  //miss
    h = false;
    replace(latency, addr, &idx, &way);
    if(outer_caches) outer_read(latency, id, addr);
    else if(latency) *latency += mem_delay;
    cache->set_meta(latency, idx, way, CM::to_shared(addr));
  }
  cache->access(idx, way);
  reporter.cache_access(cache->level, cache->core_id, cache->cache_id,
                        addr, idx, way, 1, h);
}

void CoherentCache::write(uint64_t *latency, uint64_t addr, uint32_t inner_id, bool to_dirty) {
  addr = CM::normalize(addr);
  uint32_t idx, way;
  bool h = true;
  cache->latency_acc(latency);
  uint64_t meta;
  if(cache->hit(latency, addr, &idx, &way)) { // hit
    if(inner_caches && CM::is_shared(cache->get_meta(NULL, idx, way))) {
      inner_probe(latency, inner_id, addr, id, true, false);
    }
    meta = cache->get_meta(NULL, idx, way);
    if(!CM::is_modified(meta)) {
      if(outer_caches) outer_write(latency, id, addr);
      meta = CM::to_modified(meta);
    }
  } else {  //miss
    h = false;
    replace(latency, addr, &idx, &way);
    if(outer_caches) outer_write(latency, id, addr);
    else if(latency) *latency += mem_delay;
    meta = CM::to_modified(addr);
  }
  if(to_dirty) meta = CM::to_dirty(meta);
  cache->set_meta(latency, idx, way, meta);
  cache->access(idx, way);
  reporter.cache_access(cache->level, cache->core_id, cache->cache_id,
                        addr, idx, way, 2, h);
}

void CoherentCache::probe(uint64_t *latency, uint64_t addr, bool invalid) {
  uint32_t idx, way;
  cache->latency_acc(latency);
  if(cache->hit(latency, addr, &idx, &way)) {
    if(inner_caches && (CM::is_modified(cache->get_meta(NULL, idx, way)) || invalid))
      inner_probe(latency, -1, addr, id, invalid, true);
    uint64_t meta = cache->get_meta(NULL, idx, way);
    if(CM::is_dirty(meta)) {
      outer_release(latency, id, addr);
      meta = CM::to_clean(meta);
      reporter.cache_writeback(cache->level, cache->core_id, cache->cache_id,
                            addr, idx, way);
    }
    if(invalid) {
      cache->set_meta(latency, idx, way, CM::to_invalid(meta));
      cache->invalid(idx, way);
      reporter.cache_evict(cache->level, cache->core_id, cache->cache_id,
                           addr, idx, way);
    } else {
      cache->set_meta(latency, idx, way, CM::to_shared(meta));
      cache->access(idx, way);
    }
    reporter.cache_access(cache->level, cache->core_id, cache->cache_id,
                          addr, idx, way, 1, true);
  }
}

void CoherentCache::query_loc(uint64_t addr, std::list<LocInfo> *locs) {
  addr = CM::normalize(addr);
  if(outer_caches) outer_query_loc(addr, locs);
  locs->push_front(cache->query_loc(addr));
  locs->front().wrapper = this;  // add a pointer for the CoherentCache wrapper
}

void CoherentCache::evict(uint64_t *latency, uint32_t idx, uint32_t way) {
  uint64_t meta = cache->get_meta(NULL, idx, way);
  uint64_t addr = CM::normalize(meta);   // we know meta has the full address except for the lowest 6 bits
  if(!CM::is_invalid(meta)) {
    if(inner_caches)
    inner_probe(latency, -1, addr, id, true, true);
    meta = cache->get_meta(NULL, idx, way); // always get a new meta after probes
    if(CM::is_dirty(meta)) {
      if(outer_caches) outer_release(latency, id, addr);
      else if(latency) *latency += mem_delay;
      reporter.cache_writeback(cache->level, cache->core_id, cache->cache_id,
                               addr, idx, way);
    }
    cache->set_meta(latency, idx, way, CM::to_invalid(meta));
    cache->invalid(idx, way);
    reporter.cache_evict(cache->level, cache->core_id, cache->cache_id,
                         addr, idx, way);
  }
}

void CoherentCache::release(uint64_t *latency, uint64_t addr, uint32_t inner_id) {
  uint32_t idx, way;
  cache->latency_acc(latency);
  if(cache->hit(latency, addr, &idx, &way)) { // hit
    uint64_t meta = cache->get_meta(NULL, idx, way);
    meta = CM::to_shared(meta);
    meta = CM::to_dirty(meta);
    cache->set_meta(latency, idx, way, meta);
  } else { // miss
    // should never went here
    throw(addr);
  }
  cache->access(idx, way);
  reporter.cache_access(cache->level, cache->core_id, cache->cache_id,
                        addr, idx, way, 1, true);
}
