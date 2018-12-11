#ifndef CM_TAG_HPP_
#define CM_TAG_HPP_

#include <cstdint>
#include <cmath>

#define CLog2(x) (uint32_t)(log2((float)(x)))

/////////////////////////////////
// base class

class TagFuncBase
{
protected:
  uint32_t iwidth;
public:
  uint32_t toff;
  TagFuncBase(uint32_t nset) : iwidth(CLog2(nset)), toff(CLog2(nset)+6) {}
  uint64_t virtual tag(uint64_t addr) = 0;
  bool match(uint64_t meta, uint64_t addr) { return tag(meta) == tag(addr); }
};


/////////////////////////////////
// normal

class TagNorm : public TagFuncBase
{
public:
  TagNorm(uint32_t nset) : TagFuncBase(nset) {}
  uint64_t virtual tag(uint64_t addr) { return addr >> toff; }

  static TagFuncBase *gen(uint32_t nset) {
    return (TagFuncBase *)(new TagNorm(nset));
  }

};

#undef CLog2
#endif
