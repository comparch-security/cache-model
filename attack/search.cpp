#include "attack/search.hpp"
#include "attack/create.hpp"
#include "util/random.hpp"

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
  return targeted_trim_original(cache, target, candidate, traverse, check);
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
  return targeted_trim_original(cache, target, candidate, traverse, check);
}
