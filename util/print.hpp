#ifndef UTIL_PRINT_HPP_
#define UTIL_PRINT_HPP_

#include <set>
#include <iostream>
#include <cstdint>
#include "cache/cache.hpp"

template<typename T>
void inline show_set(const std::set<T>& s) {
  std::cout << "{";
  for(auto it = s.begin(); it != s.end(); ) {
    std::cout << "0x" << std::hex << *it;
    if(++it != s.end()) std::cout << ", ";
  }
  std::cout << "}";
}

extern void print_line(uint64_t addr, L1CacheBase *l1c);

#endif
