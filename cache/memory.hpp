#ifndef CM_MEMORY_HPP_
#define CM_MEMORY_HPP_

#include <cstdint>
#include <map>

class Memory
{
  std::map<uint64_t, uint32_t> mem;

public:
  void read(uint64_t addr, uint32_t id) {
    mem[addr] = id << 1;
  }

  void write(uint64_t addr, uint32_t id) {
    mem[addr] = (id << 1) | 0x1;
  }

  void clear() {
    mem.clear();
  }
};

#endif
