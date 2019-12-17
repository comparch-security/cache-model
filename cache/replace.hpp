#ifndef CM_REPLACE_HPP_
#define CM_REPLACE_HPP_

#include <cstdint>
#include <unordered_map>
#include <list>
#include <vector>
#include <unordered_set>
#include <functional>
#include "datagen/include/random_generator.h"

///////////////////////////////////
// Base class

class ReplaceFuncBase
{
protected:
  uint32_t nset, nway;
public:
  ReplaceFuncBase(uint32_t nset, uint32_t nway) : nset(nset), nway(nway) {}
  virtual uint32_t replace(uint32_t set) = 0;
  virtual void access(uint32_t set, uint32_t way) = 0;
  virtual void invalid(uint32_t set, uint32_t way) = 0;
  virtual ~ReplaceFuncBase() {}
};

///////////////////////////////////
// Random replacement

class ReplaceRandom : public ReplaceFuncBase
{
public:
  ReplaceRandom(uint32_t nset, uint32_t nway) : ReplaceFuncBase(nset, nway) {}
  virtual uint32_t replace(uint32_t set){
    return (uint32_t)random_uint_uniform(31, 0, nway-1);
  }
  virtual void access(uint32_t set, uint32_t way) {}
  virtual void invalid(uint32_t set, uint32_t way) {}

  virtual ~ReplaceRandom() {}

  static ReplaceFuncBase *gen(uint32_t nset, uint32_t nway) {
    return (ReplaceFuncBase *)(new ReplaceRandom(nset, nway));
  }
};

///////////////////////////////////
// FIFO replacement

class ReplaceFIFO : public ReplaceFuncBase
{
protected:
  std::unordered_map<uint32_t, std::list<uint32_t> > used_map;
  std::unordered_map<uint32_t, std::unordered_set<uint32_t> > free_map;

public:

  ReplaceFIFO(uint32_t nset, uint32_t nway) : ReplaceFuncBase(nset, nway) {}

  virtual uint32_t replace(uint32_t set) {
    if(!free_map.count(set))
      for(uint32_t i=0; i<nway; i++) free_map[set].insert(i);

    if(free_map[set].size() > 0)
      return *(free_map[set].begin());
    else
      return used_map[set].front();
  }

  virtual void access(uint32_t set, uint32_t way) {
    if(free_map[set].count(way)) {
      free_map[set].erase(way);
      used_map[set].push_back(way);
    }
  }

  virtual void invalid(uint32_t set, uint32_t way) {
    used_map[set].remove(way);
    free_map[set].insert(way);
  }

  virtual ~ReplaceFIFO() {}

  static ReplaceFuncBase *gen(uint32_t nset, uint32_t nway) {
    return (ReplaceFuncBase *)(new ReplaceFIFO(nset, nway));
  }

};


///////////////////////////////////
// LRU replacement

class ReplaceLRU : public ReplaceFIFO
{
public:

  ReplaceLRU(uint32_t nset, uint32_t nway) : ReplaceFIFO(nset, nway) {}
 
  virtual void access(uint32_t set, uint32_t way) {
    if(free_map[set].count(way)) {
      free_map[set].erase(way);
      used_map[set].push_back(way);
    } else {
      used_map[set].remove(way);
      used_map[set].push_back(way);
    }
  }

  virtual ~ReplaceLRU() {}

  static ReplaceFuncBase *gen(uint32_t nset, uint32_t nway) {
    return (ReplaceFuncBase *)(new ReplaceLRU(nset, nway));
  }

};

///////////////////////////////////
// SRRIP replacement
//
// https://dblp.org/rec/bib/conf/isca/JaleelTSE10
//

class ReplaceRRIP : public ReplaceFuncBase
{
protected:
  std::unordered_map<uint32_t, std::vector<uint32_t> > rrpv_map;
  uint32_t rrpv_max;

public:
  ReplaceRRIP(uint32_t nset, uint32_t nway, uint32_t width)
  : ReplaceFuncBase(nset, nway), rrpv_max(1<<width) {}

  virtual uint32_t replace(uint32_t set) {
    if(!rrpv_map.count(set))
      rrpv_map[set] = std::vector<uint32_t>(nway, rrpv_max);

    uint32_t pos = 0, pmax = 0;
    for(uint32_t i=0; i<nway; i++)
      if(rrpv_map[set][i] > pmax) { pos = i; pmax = rrpv_map[set][i]; }

    if(pmax < rrpv_max - 1) {
      uint32_t diff = rrpv_max - 1 - pmax;
      for(uint32_t i=0; i<nway; i++) rrpv_map[set][i] += diff;
    }

    return pos;
  }

  virtual void access(uint32_t set, uint32_t way) {
    if(rrpv_map[set][way] == rrpv_max)
      rrpv_map[set][way] = rrpv_max - 2;
    else
      rrpv_map[set][way] = 0;
  }

  virtual void invalid(uint32_t set, uint32_t way) {
    if(rrpv_map.count(set))
      rrpv_map[set][way] = rrpv_max;
  }

  virtual ~ReplaceRRIP() {}

  static ReplaceFuncBase *factory(uint32_t nset, uint32_t nway, uint32_t width) {
    return (ReplaceFuncBase *)(new ReplaceRRIP(nset, nway, width));
  }

  static replacer_creator_t gen(uint32_t width) {
    return std::bind(factory, std::placeholders::_1, std::placeholders::_2, width);
  }
};

#endif
