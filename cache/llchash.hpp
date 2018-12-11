#ifndef CM_LLCHASH_HPP_
#define CM_LLCHASH_HPP_

#include <cstdint>

/////////////////////////////////
// base class

class LLCHashBase
{
protected:
  uint32_t nllc;
public:
  LLCHashBase(uint32_t nllc) : nllc(nllc) {}
  uint32_t virtual hash(uint64_t addr) = 0;
};

/////////////////////////////////
// normal (no hash)

class LLCHashNorm : public LLCHashBase
{
public:
  LLCHashNorm(uint32_t nllc) : LLCHashBase(nllc) {}
  uint32_t virtual hash(uint64_t addr) { return (addr >> 6) % nllc; }
  static LLCHashBase *gen(uint32_t nllc) {
    return (LLCHashBase *)(new LLCHashNorm(nllc));
  }
};

/////////////////////////////////
// hash
class LLCHashHash : public LLCHashBase
{
  uint32_t addr_xor(uint64_t mask, uint64_t addr) {
    uint64_t m = mask & addr;
    uint32_t rv = 0;
    for(auto i=0; i<64; i++) {
      rv ^= (m & 0x1);
      m >>= 1;
    }
    return rv;
  }

  std::map<uint64_t, uint32_t> hash_cache;
  
public:
  LLCHashHash(uint32_t nllc) : LLCHashBase(nllc) {}
  uint32_t virtual hash(uint64_t addr) {
    uint32_t rv = 0;
    if(hash_cache.count(addr)) return hash_cache[addr];

    switch(nllc) {
    case 2:
      rv = addr_xor(0x15f575440, addr);
      break;
    case 4:
      rv = (addr_xor(0x6b5faa880, addr) << 1) | addr_xor(0x35f575440, addr);
      break;
    case 8:
      rv = (addr_xor(0x3cccc93100, addr) << 2) |
           (addr_xor(0x2eb5faa880, addr) << 1) |
            addr_xor(0x1b5f575400, addr);
      break;
    default:
      std::cout << "LLCHash: unsupport number of LLCs!";
      return 0xffff;
    }
    hash_cache[addr] = rv;
    return rv;
  }
    
};


#endif
