#ifndef CM_REPLACE_HPP_
#define CM_REPLACE_HPP_

#include <cstdint>
#include <unordered_map>
#include <list>
#include <unordered_set>
#include "datagen/include/random_generator.h"

///////////////////////////////////
// Base class

class ReplaceFuncBase
{
protected:
  uint32_t nset, nway;
public:
  ReplaceFuncBase(uint32_t nset, uint32_t nway) : nset(nset), nway(nway) {}
  uint32_t virtual replace(uint32_t set) = 0;
  void virtual access(uint32_t set, uint32_t way) = 0;
  void virtual invalid(uint32_t set, uint32_t way) = 0;
  virtual ~ReplaceFuncBase() {}
};

///////////////////////////////////
// Random replacement

class ReplaceRandom : public ReplaceFuncBase
{
public:
  ReplaceRandom(uint32_t nset, uint32_t nway) : ReplaceFuncBase(nset, nway) {}
  uint32_t virtual replace(uint32_t set){
    return (uint32_t)random_uint_uniform(31, 0, nway-1);
  }
  void virtual access(uint32_t set, uint32_t way) {}
  void virtual invalid(uint32_t set, uint32_t way) {}

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

  uint32_t virtual replace(uint32_t set) {
    if(!free_map.count(set))
      return 0;
    else if(free_map[set].size() > 0)
      return *(free_map[set].begin());
    else
      return used_map[set].front();
  }

  void virtual access(uint32_t set, uint32_t way) {
    if(!free_map.count(set)) {
      for(uint32_t i=0; i<nway; i++) free_map[set].insert(i);
    }

    if(free_map[set].count(way)) {
      free_map[set].erase(way);
      used_map[set].push_back(way);
    }
  }

  void virtual invalid(uint32_t set, uint32_t way) {
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
 
  void virtual access(uint32_t set, uint32_t way) {
    if(!free_map.count(set)) {
      for(uint32_t i=0; i<nway; i++) free_map[set].insert(i);
    }

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

#endif
