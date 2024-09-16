#ifndef UTIL_DETECTOR_HPP_
#define UTIL_DETECTOR_HPP_

#include <cstdint>
#include <vector>
#include <list>
#include <functional>
#include <cmath>
#include <unordered_set>
#include <string>

typedef std::function<void(uint64_t, const std::vector<uint64_t>&, const std::vector<uint64_t>&)> set_dist_proc_t;

extern set_dist_proc_t
set_dist_normal_acc_gen(const std::list<double> x_factor,                       // factors for input (FIR part)
                        std::list<std::vector<uint64_t> > *x_access,
                        std::list<std::vector<uint64_t> > *x_evict,
                        const std::list<double> y_factor,                       // factors for output (IIR part)
                        std::list<std::vector<double> > *y_access,
                        std::list<std::vector<double> > *y_evict,
                        std::string fn);

class AbnormalSetDetector_t
{
  const std::list<double> factor;
  const uint32_t nset;
  const uint32_t nway;
  const std::string cache_name;
  std::list<std::vector<double> > scaled_evict_hist;
  double threshold;
  std::unordered_set<uint32_t> detected_sets;
  uint64_t abnormals;
  uint64_t samples;

public:
  AbnormalSetDetector_t(const std::list<double> factor, uint32_t nset, uint32_t nway, const std::string& cache_name)
    : factor(factor), nset(nset), nway(nway), cache_name(cache_name), abnormals(0)
  {
    for(auto f:factor)
      scaled_evict_hist.push_front(std::vector<double>(nset, 0.0));
    threshold = sqrt((double)nset) * 0.50 * nway;
  }

  void detect(uint64_t time,
              const std::vector<uint64_t>& access_record,
              const std::vector<uint64_t>& evict_record);

  set_dist_proc_t gen();

  void report();
};

extern set_dist_proc_t set_dist_weighted_stat_acc_gen(std::string fn);

class AbnormalSetRecorder_t
{
  const std::string fn;
  const uint32_t nset;
public:
  AbnormalSetRecorder_t(const std::string &fn, uint32_t nset)
    : fn(fn), nset(nset) {}

  void detect(uint64_t time,
              const std::vector<uint64_t>& access_record,
              const std::vector<uint64_t>& evict_record);
  set_dist_proc_t gen();
};

#endif
