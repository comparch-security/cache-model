#ifndef CM_TAG_HPP_
#define CM_TAG_HPP_

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
  virtual uint64_t tag(uint64_t addr) = 0;
  bool match(uint64_t meta, uint64_t addr) { return tag(meta) == tag(addr); }
  virtual ~TagFuncBase() {}
};


/////////////////////////////////
// normal

class TagNorm : public TagFuncBase
{
public:
  TagNorm(uint32_t nset) : TagFuncBase(nset) {}
  virtual uint64_t tag(uint64_t addr) { return addr >> toff; }

  virtual ~TagNorm() {}

  static TagFuncBase *factory(uint32_t nset) {
    return (TagFuncBase *)(new TagNorm(nset));
  }

  static tagger_creator_t gen() {
    using namespace std::placeholders;
    return std::bind(factory, _1);
  }
};

#undef CLog2
#endif
