#ifndef UTIL_ARGUMENT_PARSER_HPP_
#define UTIL_ARGUMENT_PARSER_HPP_

#include "cache/definitions.hpp"

const int MAX_CACHE_LEVEL = 2;

struct CacheCFG {
  bool enable[MAX_CACHE_LEVEL];
  uint32_t number[MAX_CACHE_LEVEL];
  cache_creator_t cache_gen[MAX_CACHE_LEVEL];
  llc_hash_creator_t hash_gen[MAX_CACHE_LEVEL];
  // extra information needed for certain applications
  uint32_t nset[MAX_CACHE_LEVEL];
  uint32_t nway[MAX_CACHE_LEVEL];
};

extern bool cache_config_parser(const std::string& fn, const std::string& cfg, CacheCFG *ccfg);

#endif

