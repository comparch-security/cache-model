#ifndef CM_REPLACE_HPP_
#define CM_REPLACE_HPP_

#include <unordered_map>
#include <list>
#include <vector>
#include <unordered_set>
#include <functional>
#include <string>
#include "util/random.hpp"

///////////////////////////////////
// Base class

class ReplaceFuncBase : public DelaySim
{
protected:
  uint32_t nset, nway;
public:
  ReplaceFuncBase(uint32_t nset, uint32_t nway, uint32_t delay)
    : DelaySim(delay), nset(nset), nway(nway) {}
  virtual uint32_t replace(uint64_t *latency, uint32_t set) = 0;
  virtual void access(uint32_t set, uint32_t way) = 0;
  virtual void invalid(uint32_t set, uint32_t way) = 0;
  virtual std::string to_string(uint32_t set) const = 0;
  virtual std::string to_string() const = 0;
  virtual ~ReplaceFuncBase() {}
};

///////////////////////////////////
// Random replacement

class ReplaceRandom : public ReplaceFuncBase
{
  std::unordered_map<uint32_t, std::unordered_set<uint32_t> > free_map;
public:
  ReplaceRandom(uint32_t nset, uint32_t nway, uint32_t delay) : ReplaceFuncBase(nset, nway, delay) {}
  virtual uint32_t replace(uint64_t *latency, uint32_t set){
    latency_acc(latency);
    if(!free_map.count(set))
      for(uint32_t i=0; i<nway; i++) free_map[set].insert(i);

    if(free_map[set].size() > 0)
      return *(free_map[set].begin());
    else
      return (uint32_t)get_random_uint64(nway);
  }
  virtual void access(uint32_t set, uint32_t way) {
    if(free_map[set].count(way))
      free_map[set].erase(way);
  }
  virtual void invalid(uint32_t set, uint32_t way) {
    free_map[set].insert(way);
  }

  // there is not need to print for random replacement
  virtual std::string to_string(uint32_t set) const { return std::string(); }
  virtual std::string to_string() const { return std::string(); }

  virtual ~ReplaceRandom() {}

  static ReplaceFuncBase *factory(uint32_t nset, uint32_t nway, uint32_t delay) {
    return (ReplaceFuncBase *)(new ReplaceRandom(nset, nway, delay));
  }

  static replacer_creator_t gen(uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, _2, delay);
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

  ReplaceFIFO(uint32_t nset, uint32_t nway, uint32_t delay) : ReplaceFuncBase(nset, nway, delay) {}

  virtual uint32_t replace(uint64_t *latency, uint32_t set) {
    latency_acc(latency);
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

  virtual std::string to_string(uint32_t set) const;
  virtual std::string to_string() const;
  virtual ~ReplaceFIFO() {}

  static ReplaceFuncBase *factory(uint32_t nset, uint32_t nway, uint32_t delay) {
    return (ReplaceFuncBase *)(new ReplaceFIFO(nset, nway, delay));
  }

  static replacer_creator_t gen(uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, _2, delay);
  }
};


///////////////////////////////////
// LRU replacement

class ReplaceLRU : public ReplaceFIFO
{
public:

  ReplaceLRU(uint32_t nset, uint32_t nway, uint32_t delay) : ReplaceFIFO(nset, nway, delay) {}
 
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

  static ReplaceFuncBase *factory(uint32_t nset, uint32_t nway, uint32_t delay) {
    return (ReplaceFuncBase *)(new ReplaceLRU(nset, nway, delay));
  }

  static replacer_creator_t gen(uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, _2, delay);
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
  ReplaceRRIP(uint32_t nset, uint32_t nway, uint32_t width, uint32_t delay)
    : ReplaceFuncBase(nset, nway, delay), rrpv_max(1<<width) {}

  virtual uint32_t replace(uint64_t *latency, uint32_t set) {
    latency_acc(latency);
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

  virtual std::string to_string(uint32_t set) const;
  virtual std::string to_string() const;
  virtual ~ReplaceRRIP() {}

  static ReplaceFuncBase *factory(uint32_t nset, uint32_t nway, uint32_t width, uint32_t delay) {
    return (ReplaceFuncBase *)(new ReplaceRRIP(nset, nway, width, delay));
  }

  static replacer_creator_t gen(uint32_t width, uint32_t delay = 0) {
    using namespace std::placeholders;
    return std::bind(factory, _1, _2, width, delay);
  }
};

#endif
