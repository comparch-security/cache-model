#ifndef CM_INDEX_HPP_
#define CM_INDEX_HPP_

#include "util/random.hpp"
#include <cmath>

#define CLog2(x) (uint32_t)(log2((float)(x)))

/////////////////////////////////
// Base class

class IndexFuncBase : public DelaySim
{
protected:
  const uint32_t imask;
public:
  // typing the indexer
  enum indexer_t {BASE, NORM, RANDOM, SKEW};
  indexer_t get_type() const { return m_type; }

  IndexFuncBase(uint32_t nset, uint32_t delay, indexer_t t = BASE) : DelaySim(delay), imask(nset-1), m_type(t) {}
  int32_t virtual index(
    uint64_t *latency,         // latency estimation
    uint64_t addr,             // address of the cache line
    int32_t skew_idx          // index of the skewed cache partition, default = 0
    ) = 0;
  int32_t index(uint64_t *latency, uint64_t addr) {
    return index(latency, addr, 0);
  }

  virtual ~IndexFuncBase() {}
protected:
  const indexer_t m_type;
};

/////////////////////////////////
// Normal

class IndexNorm : public IndexFuncBase
{
public:
  IndexNorm(uint32_t nset, uint32_t delay) : IndexFuncBase(nset, delay, NORM) {}

  virtual int32_t index(uint64_t *latency, uint64_t addr, int32_t skew_idx) {
    latency_acc(latency);
    return (int32_t)((addr >> 6) & imask);
  }

  virtual ~IndexNorm() {}

  static IndexFuncBase *factory(uint32_t nset, uint32_t delay) {
    return (IndexFuncBase *)(new IndexNorm(nset, delay));
  }

  static indexer_creator_t gen(uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, delay);
  }
};

/////////////////////////////////
// Random

class IndexRandom : public IndexFuncBase
{
  const uint32_t iwidth;
  const uint32_t tmask;
  uint64_t seed;
public:
  IndexRandom(uint32_t nset, uint32_t wtag, uint32_t delay)
    : IndexFuncBase(nset, delay, RANDOM), iwidth(CLog2(nset)), tmask((1 << wtag)-1),
      seed(get_random_uint64(1ull << 60))
  {}

  virtual int32_t index(uint64_t *latency, uint64_t addr, int32_t skew_idx) {
    latency_acc(latency);
    uint32_t oidx = (uint32_t)((addr >> 6) & imask);
    uint64_t otag = (uint64_t)((addr >> (6+iwidth)) & tmask);
    uint32_t hkey = (uint32_t)(hash(seed ^ otag)) & imask;
    return (int32_t)(hkey ^ oidx);
  }

  void reseed() { seed = hash(seed); }

  virtual ~IndexRandom() {}

  static IndexFuncBase *factory(uint32_t nset, uint32_t wtag, uint32_t delay) {
    return (IndexFuncBase *)(new IndexRandom(nset, wtag, delay));
  }

  static indexer_creator_t gen(uint32_t wtag, uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, wtag, delay);
  }
};

/////////////////////////////////
// Skewed Cache Indexer
class IndexSkewed : public IndexFuncBase
{
  const uint32_t iwidth;
  const uint32_t partition;
  uint64_t seed;
public:
  IndexSkewed(uint32_t nset, uint32_t partition, uint32_t delay)
    : IndexFuncBase(nset/partition, delay, SKEW), iwidth(CLog2(nset/partition)),
      partition(partition), seed(get_random_uint64(1ull << 60))
  {}

  virtual int32_t index(uint64_t *latency, uint64_t addr, int32_t skew_idx) {
    latency_acc(latency);
    uint64_t tidx = addr >> 6;
    uint32_t mask = (1 << iwidth)-1;
    uint32_t ridx = (uint32_t)(hash(seed ^ tidx ^ skew_idx)) & mask;
    return (int32_t)((skew_idx << iwidth) + ridx);
  }

  void reseed() { seed = hash(seed); }

  virtual ~IndexSkewed() {}

  static IndexFuncBase *factory(uint32_t nset, uint32_t partition, uint32_t delay) {
    return (IndexFuncBase *)(new IndexSkewed(nset, partition, delay));
  }

  static indexer_creator_t gen(uint32_t partition, uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, partition, delay);
  }
};

#undef CLog2

#endif
