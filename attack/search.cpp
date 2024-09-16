#include "attack/search.hpp"
#include "attack/create.hpp"
#include "util/random.hpp"
#include "cache/cache.hpp"
//#include <iostream>

void split_random_set(
                      std::list<uint64_t> &candidate,
                      std::list<uint64_t> &picked_set,
                      uint32_t pick
                      )
{
  std::unordered_set<uint32_t> picked_index;
  get_random_set32(picked_index, pick, candidate.size()-1);
  uint32_t i = 0;
  auto it = candidate.begin();
  for(uint32_t index = 0; index < candidate.size(); index++) {
    if(picked_index.count(index)) {
      picked_set.push_back(*it);
      it = candidate.erase(it);
    } else
      it++;
  }
}

bool targeted_evict_random_pick(
                                L1CacheBase * cache,
                                uint64_t target,
                                std::list<uint64_t> &candidate,
                                std::list<uint64_t> &picked_set,
                                std::list<uint64_t> &evict_set,
                                traverse_test_t traverse,
                                uint32_t pick
                                )
{
  split_random_set(candidate, picked_set, pick);
  std::list<uint64_t> traverse_set(candidate.cbegin(), candidate.cend());
  traverse_set.insert(traverse_set.end(), evict_set.cbegin(), evict_set.cend());
  return traverse(cache, traverse_set, target);
}

static uint32_t loop_size, loop_count, loop_count_max;

bool static loop_guard(uint32_t new_size) {
  if(loop_size == new_size) {
    loop_count++;
    if(loop_count > loop_count_max)
      return false;
  } else {
    loop_count = 0;
    loop_size = new_size;
    loop_count_max = new_size * 4;
  }
  return true;
}

bool targeted_trim_original(
                            L1CacheBase * cache,
                            uint64_t target,
                            std::list<uint64_t> &candidate,
                            traverse_test_t traverse,
                            check_func_t check
                            )
{
  std::list<uint64_t> picked_set, evict_set;
  loop_count = 0;
  while(candidate.size() > 0 && loop_guard(candidate.size())) {
    if(!targeted_evict_random_pick(cache, target, candidate, picked_set, evict_set, traverse, 1))
      candidate.insert(candidate.end(), picked_set.begin(), picked_set.end());
    picked_set.clear();
  }
  return check(candidate);
}

bool targeted_trim_divide(
                          L1CacheBase * cache,
                          uint64_t target,
                          std::list<uint64_t> &candidate,
                          traverse_test_t traverse,
                          check_func_t check,
                          uint32_t split
                          )
{
  std::list<uint64_t> picked_set, evict_set;
  loop_count = 0;
  while(candidate.size() > 2*split && loop_guard(candidate.size())) {
    uint32_t step = (candidate.size() + split - 1) / split;
    for(uint32_t i=0; i<split; i++) {
      picked_set.clear();
      if(targeted_evict_random_pick(cache, target, candidate, picked_set, evict_set, traverse, step))
        break;
      evict_set.insert(evict_set.end(), picked_set.begin(), picked_set.end());
    }
    candidate.insert(candidate.end(), evict_set.begin(), evict_set.end());
    evict_set.clear();
  }
  if(candidate.size() > 2*split) return false;
  else return targeted_trim_original(cache, target, candidate, traverse, check);
}

bool targeted_trim_divide_random(
                          L1CacheBase * cache,
                          uint64_t target,
                          std::list<uint64_t> &candidate,
                          traverse_test_t traverse,
                          check_func_t check,
                          uint32_t split
                          )
{
  std::list<uint64_t> picked_set, evict_set;
  loop_count = 0;
  while(candidate.size() > 2*split && loop_guard(candidate.size())) {
    uint32_t step = (candidate.size() + split - 1) / split;
    picked_set.clear();
    if(!targeted_evict_random_pick(cache, target, candidate, picked_set, evict_set, traverse, step))
      candidate.insert(candidate.end(), picked_set.begin(), picked_set.end());
  }
  if(candidate.size() > 2*split) return false;
  else return targeted_trim_original(cache, target, candidate, traverse, check);
}

bool search_conflict_blocks(
                            L1CacheBase * cache,
                            uint64_t target,
                            std::list<uint64_t> &candidate,
                            std::list<uint64_t> &evset,
                            uint64_t delay_th
                            )
{
  size_t csize;
  uint64_t delay = 0;
  do {
    csize = candidate.size();
    for(auto it=candidate.rbegin(); it != candidate.rend(); it++)
      cache->read(*it);

    for(auto it=candidate.begin(); it != candidate.end();) {
      delay = 0;
      cache->read(&delay, *it);
      if(delay > delay_th) it = candidate.erase(it);
      else it++;
    }
  } while(csize != candidate.size());

  delay = 0;
  cache->read(&delay, target);
  if(delay <= delay_th) return false; // failed to find any new blocks

  for(auto a:candidate) {
    delay = 0;
    cache->read(&delay, a);
    if(delay > delay_th) {
      evset.push_back(a);
      return true;
    }
  }

  return false; // should not reach here
}

bool find_conflict_set_by_prime(
                       L1CacheBase * cache,
                       uint32_t ctype,
                       hit_func_t hit,
                       check_func_t check,
                       uint64_t target,
                       std::list<uint64_t> &evset,
                       uint64_t pssize,
                       uint64_t evsize
                       )
{
  std::unordered_set<uint64_t> evset_set;
  while(evset_set.size() < evsize && loop_guard(100)) {
    std::list<uint64_t> prime_set;
    obtain_cache_prime_set(pssize, prime_set, cache, hit);

    bool h=hit(target);
    cache->read(target);
    if(h) continue;

    bool found_one = true;
    switch(ctype) {
    case 0: // FIFO/LRU
      for(int i=0; evset_set.size() < evsize && found_one && i<2; i++) {
        found_one = false;
        for(auto a:prime_set) {
          h = hit(a);
          cache->read(a);
          if(!h) {
            evset_set.insert(a);
            loop_guard(10);
            found_one = true;
            if(evset_set.size() >= evsize) break;
          }
        }
      }
      break;
    case 1: // RRIP
      cache->write(target);
      for(int i=0; found_one && i<2; i++) {
        found_one = false;
        for(auto a:prime_set) {
          h = hit(a);
          cache->read(a);
          if(!h) {
            evset_set.insert(a);
            loop_guard(10);
            cache->write(a);
            found_one = true;
          }
        }
      }
      break;
    case 2: // Random
      for(int i=0; found_one && i<4; i++) {
        found_one = false;
        for(auto e:evset_set) cache->read(e);
        for(auto a:prime_set) {
          h = hit(a);
          cache->read(a);
          if(!h) {
            evset_set.insert(a);
            loop_guard(10);
            found_one = true;
          }
        }
      }
      break;
    case 3: // Skewed
      for(int i=0; i<evsize; i++) {
        cache->read(target);
        cache->flush(target);
      }
      for(auto a:prime_set) {
        h = hit(a);
        cache->read(a);
        if(!h) {
          evset_set.insert(a);
          loop_guard(10);
        }
      }
      break;
    default:
      return false;
    }
    //std::cout << "evset size: " << evset_set.size() << std::endl;
  }
  evset.insert(evset.begin(), evset_set.begin(), evset_set.end());
  return evset.size() >= evsize && check(evset);
}

bool find_conflict_set_by_repeat(
                       L1CacheBase * cache,
                       hit_func_t hit,
                       check_func_t check,
                       uint64_t target,
                       std::list<uint64_t> &evset,
                       uint64_t evsize
                       )
{
  std::unordered_set<uint64_t> evset_set;
  cache->read(target);

  while(evset_set.size() < evsize && loop_guard(100000)) {
    uint64_t c = CM::normalize(get_random_uint64(1ull << 60));
    cache->read(c);

    bool h=hit(target);
    cache->read(target);
    if(!h) {
      evset_set.insert(c);
      loop_guard(10);
    }
  }
  evset.insert(evset.begin(), evset_set.begin(), evset_set.end());
  return evset_set.size() == evsize && check(evset);
}

uint32_t guess_cache_type(
                          L1CacheBase * cache,
                          hit_func_t hit,
                          uint64_t target,
                          uint64_t pssize,
                          uint32_t way
                          )
{
  std::list<uint64_t> prime_set;
  std::unordered_set<uint64_t> blocks;
  do {
    uint32_t trial = obtain_cache_prime_set(pssize, prime_set, cache, hit);
    bool h = hit(target);
    cache->read(target);
    if(h) continue;

    if(trial <= 3) {
      for(uint32_t i=0; i<4*way; i++) {
        cache->flush(target);
        for(auto a:prime_set) {
          h = hit(a);
          cache->read(a);
          if(!h) {
            blocks.insert(a);
            break;
          }
        }
        cache->read(target);
      }
    } else {
      for(uint32_t i=0; i<8*way; i++) {
        cache->read(target);
        cache->flush(target);
      }
      for(auto a:prime_set) {
        h = hit(a);
        cache->read(a);
        if(!h)
          blocks.insert(a);
      }
    }

    if(blocks.size() == 0) continue;
    if(trial <= 3 && blocks.size() >= way/2) return 0; // FIFO/LRU
    if(trial <= 3 && blocks.size() <  way/2) return 1; // RRIP
    if(trial > 3  && blocks.size() ==  1   ) return 2; // Random
    if(trial > 3  && blocks.size() >   1   ) return 3; // skewed
  } while(true);
}
