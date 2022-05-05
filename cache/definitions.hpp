#ifndef CM_DEFINITIONS_HPP_
#define CM_DEFINITIONS_HPP_

#include <vector>
#include <cstdint>
#include <functional>

//declarations
class CacheBase;
class CoherentCache;

class IndexFuncBase;
class TagFuncBase;
class ReplaceFuncBase;
class LLCHashBase;

typedef std::function<IndexFuncBase *(uint32_t)> indexer_creator_t;
typedef std::function<TagFuncBase *(uint32_t)> tagger_creator_t;
typedef std::function<ReplaceFuncBase *(uint32_t, uint32_t)> replacer_creator_t;
typedef std::function<LLCHashBase *(uint32_t)> llc_hash_creator_t;
typedef std::function<CacheBase *(uint32_t, int32_t, uint32_t)> cache_creator_t;

// global data structure
extern std::vector<CoherentCache *> l1_caches;     // list of L1 caches
extern std::vector<CoherentCache *> llc_caches;    // list of LLCs

// event tracer
class Reporter_t;
extern Reporter_t reporter;

/////////////////////////////////
// cache model functions

struct CM
{
  static uint64_t normalize(uint64_t addr) { return addr >> 6 << 6; }
  static bool is_invalid(uint64_t m)       { return (m&0x3) == 0; }
  static bool is_shared(uint64_t m)        { return (m&0x3) == 1; }
  static bool is_modified(uint64_t m)      { return (m&0x3) == 2; }
  static bool is_dirty(uint64_t m)         { return (m&0x4) == 0x4; }
  static uint64_t to_invalid(uint64_t m)   { return 0; }
  static uint64_t to_shared(uint64_t m)    { return (m >> 2 << 2) | 1; }
  static uint64_t to_modified(uint64_t m)  { return (m >> 2 << 2) | 2; }
  static uint64_t to_dirty(uint64_t m)     { return m | 0x4; }
  static uint64_t to_clean(uint64_t m)     { return m & ~(uint64_t)(0x4); }
};

// query
class CBInfo;
class SetInfo;
class LocInfo;

#endif
