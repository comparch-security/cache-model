#ifndef UTIL_TRAVERSE_PARSER_HPP_
#define UTIL_TRAVERSE_PARSER_HPP_

#include "attack/traverse.hpp"
#include <string>
struct TraverseTestCFG {
  std::string traverse_type;
  uint32_t ntests;
  uint32_t ntraverse;
  uint32_t threshold;
  uint32_t window;
  uint32_t repeat;
  uint32_t step;
};

extern traverse_func_t traverse_config_parser(const std::string& fn, const std::string& cfg, TraverseTestCFG *tcfg);

#endif
