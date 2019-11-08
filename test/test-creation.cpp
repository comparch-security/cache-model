#include "cache/cache.hpp"
#include "attack/algorithm.hpp"
#include "attack/create.hpp"
#include "util/report.hpp"
#include "util/query.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/error_of.hpp>
#include <boost/accumulators/statistics/error_of_mean.hpp>
using namespace boost::accumulators;

Reporter_t reporter;

std::vector<CoherentCache *>  l1_caches;
std::vector<CoherentCache *> llc_caches;

hit_func_t hit;
check_func_t check;

uint32_t NL1;
uint32_t NL1Set;
uint32_t NL1Way;
uint32_t NL1WRTI;

uint32_t NLLC;
uint32_t NLLCSet;
uint32_t NLLCWay;
uint32_t NLLCWRTI;

uint32_t ProbeCycle;
uint32_t ProbeWrite;
uint32_t ProbeThreshold;
uint32_t SearchMaxIteration = 2000;

void set_hit_check_func(uint64_t addr, L1CacheBase *cache, uint32_t level) {
  std::list<LocInfo> locs;
  cache->query_loc(addr, &locs);
  auto it = locs.begin();
  for(int i=1; i<level; i++) it++;
  CacheBase *c = it->cache;
  hit = std::bind(query_hit, std::placeholders::_1, c);
  check = std::bind(query_check, addr, c, std::placeholders::_1);
}

int main(int argc, char* argv[]) {
  if(argc < 13) {
    for(int i=0; i<argc; i++) std::cout << argv[i] << " ";
    std::cout << std::endl;
    std::cout << "test_creation nl1 nl1s nl1w nllc nllcs nllcw level size cycle write threshold N [report]" << std::endl;
    return 0;
  }

  NL1 = atoi(argv[1]);
  NL1Set = atoi(argv[2]);
  NL1Way = atoi(argv[3]);
  NLLC = atoi(argv[4]);
  NLLCSet = atoi(argv[5]);
  NLLCWay = atoi(argv[6]);
  int cache_level = atoi(argv[7]);
  int candidate_size = atoi(argv[8]);
  ProbeCycle = atoi(argv[9]);
  ProbeWrite = atoi(argv[10]);
  ProbeThreshold = atoi(argv[11]);
  uint32_t testN = atoi(argv[12]);
  std::string fn = "";
  if(argc == 14) fn = argv[13];

  l1_caches.resize(NL1);
  llc_caches.resize(NLLC);

  for(int i=0; i<NL1; i++)
    l1_caches[i] =
      new L1CacheBase(i, i, 0, CacheBase::gen, NL1Set, NL1Way,
                      IndexNorm::gen, TagNorm::gen, ReplaceLRU::gen,
                      &llc_caches, LLCHashNorm::gen, NLLC);

  for(int i=0; i<NLLC; i++)
    llc_caches[i] =
      new LLCCacheBase(i, CacheBase::gen, NLLCSet, NLLCWay,
                       IndexNorm::gen, TagNorm::gen, ReplaceLRU::gen,
                       &l1_caches);

  random_seed_gen64();

  typedef accumulator_set<double, stats<tag::mean, tag::error_of<tag::mean> > > stat_mean;

  stat_mean  cache_state;

  std::unordered_set<uint64_t> candidate;

  for(uint32_t t=0; t<testN; t++) {
    uint64_t target = random_uint_uniform(60, 0, (1ll<<60)-1);
    candidate.clear();
    reporter.clear();
    if(cache_level == 1) reporter.register_cache_access_tracer(1, 0, 0);
    else                 reporter.register_cache_access_tracer(2);

    set_hit_check_func(target, (L1CacheBase *)l1_caches[0], cache_level);
    if(!obtain_targeted_evict_set(candidate_size, candidate, (L1CacheBase *)l1_caches[0], target, ProbeCycle, ProbeWrite, ProbeThreshold, hit, 1000))
      continue;

    double accesses = (cache_level == 1) ?
      (double)(reporter.check_cache_access(1, 0, 0)) :
      (double)(reporter.check_cache_access(2)) ;
    cache_state(accesses);
  }

  if(fn != "") {
    std::ofstream outfile(fn, std::ofstream::app);
    outfile << candidate_size << "\t";
    outfile << testN << "\t";
    outfile << mean(cache_state) << "\t";
    outfile << error_of<tag::mean>(cache_state) << std::endl;
  }
  return 0;
}
