#ifndef CM_INDEX_HPP_
#define CM_INDEX_HPP_

#include <cstdint>
#include <cmath>
#include "datagen/include/random_generator.h"

#define CLog2(x) (uint32_t)(log2((float)(x)))

/////////////////////////////////
// Base class

class IndexFuncBase
{
protected:
  const uint32_t imask;
public:
  IndexFuncBase(uint32_t nset) : imask(nset-1) {}
  uint32_t virtual index(uint64_t addr) = 0;
};

/////////////////////////////////
// Normal

class IndexNorm : public IndexFuncBase
{
public:
  IndexNorm(uint32_t nset) : IndexFuncBase(nset) {}

  uint32_t virtual index(uint64_t addr) {
    return (uint32_t)((addr >> 6) & imask);
  }

  static IndexFuncBase *gen(uint32_t nset) {
    return (IndexFuncBase *)(new IndexNorm(nset));
  }

};

#undef CLog2

#endif
