#include "util/print.hpp"

void print_line(uint64_t addr, L1CacheBase *l1c) {
  std::cout << "address: " << std::hex << addr << " | ";
  std::cout << "L1 ";
  l1c->print_line(addr);
  std::cout << " | llc " << llc_hasher->hash(addr) << " ";
  llc_caches[llc_hasher->hash(addr)]->print_line(addr);
  std::cout << std::endl;
}
