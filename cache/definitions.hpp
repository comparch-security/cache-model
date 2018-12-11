#ifndef CM_DEFINITIONS_HPP_
#define CM_DEFINITIONS_HPP_

#include <vector>
#include <cstdint>
#include <functional>

//declarations
class Memory;
class L1CacheBase;
class LLCCacheBase;

class IndexFuncBase;
class TagFuncBase;
class ReplaceFuncBase;
class LLCHashBase;

typedef std::function<IndexFuncBase *(uint32_t)> indexer_creator_t;
typedef std::function<TagFuncBase *(uint32_t)> tagger_creator_t;
typedef std::function<ReplaceFuncBase *(uint32_t, uint32_t)> replacer_creator_t;
typedef std::function<LLCHashBase *(uint32_t)> llc_hash_creator_t;

// global cache configuration
#define NL1 4
#define NL1Set 64
#define NL1Way 8
#define NL1WRTI 6

#define NLLC 1
#define NLLCSet 1024
#define NLLCWay 16
#define NLLCWRTI 10

// probe parameters
extern uint32_t ProbeCycle;
extern uint32_t ProbeWrite;
extern uint32_t ProbeThreshold;
extern uint32_t SearchMaxIteration;

// global data structure
extern Memory *extern_mem;                       // memory backend
extern std::vector<L1CacheBase *> l1_caches;     // list of L1 caches
extern std::vector<LLCCacheBase *> llc_caches;   // list of LLCs

extern LLCHashBase *llc_hasher;                  // the hash used in LLC

extern indexer_creator_t  l1_indexer_creator;   // factory function to create the L1 indexer
extern tagger_creator_t   l1_tagger_creator;    // factory function to create the L1 tagger
extern replacer_creator_t l1_replacer_creator;  // factory function to create the L1 replacer
extern indexer_creator_t  llc_indexer_creator;  // factory function to create the LLC indexer
extern tagger_creator_t   llc_tagger_creator;   // factory function to create the LLC tagger
extern replacer_creator_t llc_replacer_creator; // factory function to create the LLC replacer

#endif
