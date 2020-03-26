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
  IndexFuncBase(uint32_t nset, uint32_t delay) : DelaySim(delay), imask(nset-1) {}
  uint32_t virtual index(
    uint64_t *latency,         // latency estimation
    uint64_t addr,             // address of the cache line
    uint32_t skew_idx          // index of the skewed cache partition, default = 0
    ) = 0;
  uint32_t index(uint64_t *latency, uint64_t addr) {
    return index(latency, addr, 0);
  }
  virtual ~IndexFuncBase() {}
};

/////////////////////////////////
// Normal

class IndexNorm : public IndexFuncBase
{
public:
  IndexNorm(uint32_t nset, uint32_t delay) : IndexFuncBase(nset, delay) {}

  virtual uint32_t index(uint64_t *latency, uint64_t addr, uint32_t skew_idx) {
    latency_acc(latency);
    return (uint32_t)((addr >> 6) & imask);
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

#undef CLog2

#endif
