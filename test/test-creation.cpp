#include "cache/definitions.hpp"
#include "attack/algorithm.hpp"
#include "attack/create.hpp"
#include "util/print.hpp"

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

uint32_t ProbeCycle;
uint32_t ProbeWrite;
uint32_t ProbeThreshold;
uint32_t SearchMaxIteration;

std::vector<L1CacheBase *>  l1_caches(NL1);
std::vector<LLCCacheBase *> llc_caches(NLLC);

Memory *extern_mem                      = new Memory();
LLCHashBase *llc_hasher                 = LLCHashNorm::gen(NLLC);
indexer_creator_t  l1_indexer_creator   = IndexNorm::gen;
tagger_creator_t   l1_tagger_creator    = TagNorm::gen;
replacer_creator_t l1_replacer_creator  = ReplaceLRU::gen;
indexer_creator_t  llc_indexer_creator  = IndexNorm::gen;
tagger_creator_t   llc_tagger_creator   = TagNorm::gen;
replacer_creator_t llc_replacer_creator = ReplaceLRU::gen;

hit_func_t hit;
check_func_t check;

int main(int argc, char* argv[]) {
  if(argc < 7) {
    for(int i=0; i<argc; i++) std::cout << argv[i] << " ";
    std::cout << std::endl;
    std::cout << "test_creation level size cycle write threshold N [file]" << std::endl;
    return 0;
  }

  int cache_level = atoi(argv[1]);
  int candidate_size = atoi(argv[2]);
  ProbeCycle = atoi(argv[3]);
  ProbeWrite = atoi(argv[4]);
  ProbeThreshold = atoi(argv[5]);
  uint32_t testN = atoi(argv[6]);
  std::string fn = "";
  if(argc == 8) fn = argv[7];

  for(int i=0; i<NL1; i++) l1_caches[i] = new L1CacheBase(i);
  for(int i=0; i<NLLC; i++) llc_caches[i] = new LLCCacheBase(i);

  random_seed_gen64();

  typedef accumulator_set<double, stats<tag::mean, tag::error_of<tag::mean> > > stat_mean;

  stat_mean  l1_stat;
  stat_mean  llc_stat;

  std::set<uint64_t> candidate;

  for(uint32_t t=0; t<testN; t++) {
    uint64_t target = random_uint_uniform(60, 0, (1ll<<60)-1);
    candidate.clear();
    REPORT::clear();
    extern_mem->clear();

    if(cache_level == 1) {
      hit = std::bind(l1_hit, l1_caches[0], std::placeholders::_1);
      check = std::bind(l1_targeted_evict_check, target, l1_caches[0],
                        std::placeholders::_1);
    } else {
      hit = llc_hit;
      check = std::bind(llc_targeted_evict_check, target, std::placeholders::_1);
    }
    if(obtain_targeted_evict_set(candidate_size, candidate, l1_caches[0], target, hit, 50)) {
      l1_stat((double)REPORT::get_l1_accessed(0));
      llc_stat((double)REPORT::get_llc_accessed());
    }
  }

  std::cout << "sucess " << count(l1_stat) << " time in " << testN << " trials." << std::endl;
  std::cout << "L1 cache access: " << mean(l1_stat) << "+/-" << error_of<tag::mean>(l1_stat)<< std::endl;
  std::cout << "LLC cache access: " << mean(llc_stat) << "+/-" << error_of<tag::mean>(llc_stat)<< std::endl;

  if(fn != "") {
    std::ofstream outfile(fn, std::ofstream::app);
    outfile << candidate_size << "\t";
    outfile << testN << "\t";
    outfile << count(l1_stat) << "\t";
    outfile << (cache_level==1 ?  mean(l1_stat) : mean(llc_stat)) << "\t";
    outfile << (cache_level==1 ?  error_of<tag::mean>(l1_stat) : error_of<tag::mean>(llc_stat)) << std::endl;
  }
  return 0;
}
