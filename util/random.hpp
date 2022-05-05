#ifndef CM_UTIL_RANDOM_HPP_
#define CM_UTIL_RANDOM_HPP_

#include <cstdint>
#include <unordered_set>
#include <list>

// a 64-bit hash function using the 64-bit random number generator
extern uint64_t hash(uint64_t);

extern uint64_t get_random_uint64(uint64_t max);

extern void get_random_set64(std::unordered_set<uint64_t> &random_set, uint32_t num, uint64_t max);
extern void get_random_set32(std::unordered_set<uint32_t> &random_set, uint32_t num, uint32_t max);

extern void get_random_list(std::list<uint64_t> &random_list, uint32_t num, uint64_t max);

extern void shuffle_list(std::list<uint64_t> &random_list);

#endif
