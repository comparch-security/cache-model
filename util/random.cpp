#include "util/random.hpp"
#include <boost/random/uniform_int_distribution.hpp>
#include "datagen/include/random_generator.h"
#include <map>

static boost::random::mt19937_64 hash_gen;
static boost::random::uniform_int_distribution<uint64_t> ranGen64;

uint64_t hash(uint64_t seed) {
  hash_gen.seed(seed);
  return ranGen64(hash_gen);
}

uint64_t get_random_uint64(uint64_t max) {
  return random_uint_uniform(60, 0, max-1);
}

void get_random_set64(
                    std::unordered_set<uint64_t> &random_set, // the set containing the random numbers
                    uint32_t num,            // number of random number to be generated
                    uint64_t max             // the maximam number to be generated
                    )
{
  if(max <= num) {
    for(uint32_t i=0; i<max; i++)
      random_set.insert(random_set.begin(), i);
  } else {
    for(uint32_t i=0; random_set.size()<num; i++)
      random_set.insert(random_set.begin(), get_random_uint64(max));
  }
}

void get_random_set32(
                    std::unordered_set<uint32_t> &random_set, // the set containing the random numbers
                    uint32_t num,            // number of random number to be generated
                    uint32_t max             // the maximam number to be generated
                    )
{
  if(max <= num) {
    for(uint32_t i=0; i<max; i++)
      random_set.insert(random_set.begin(), i);
  } else {
    for(uint32_t i=0; random_set.size()<num; i++)
      random_set.insert(random_set.begin(), (uint32_t)get_random_uint64(max));
  }
}

void get_random_list(
                     std::list<uint64_t> &random_list, // the set containing the random numbers
                     uint32_t num,              // number of random number to be generated
                     uint64_t max               // the maximam number to be generated
                     )
{
  std::unordered_set<uint64_t> random_set;
  get_random_set64(random_set, num, max);
  random_list = std::list<uint64_t>(random_set.begin(), random_set.end());
}

void shuffle_list(std::list<uint64_t> &random_list) {
  std::map<uint64_t, uint64_t> buf;
  while(!random_list.empty()) {
    uint64_t rid;
    do {
      rid = get_random_uint64(1ull << 60);
    } while(buf.count(rid));
    buf[rid] = random_list.back();
    random_list.pop_back();
  }
  for(auto r : buf) random_list.push_back(r.second);
}
