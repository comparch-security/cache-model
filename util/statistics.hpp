#ifndef UTIL_STATISTICS_HPP_
#define UTIL_STATISTICS_HPP_

#include <cstdint>
#include <vector>
#include <utility>

extern uint32_t init_mean_stat();
extern uint32_t init_window_stat(uint32_t window);
extern uint32_t init_histo_stat(uint32_t binN, uint32_t cacheS);
extern uint32_t init_tail_stat(bool dir, uint32_t cacheS);

extern void record_mean_stat(uint32_t handle, double sample);
extern void record_window_stat(uint32_t handle, double sample);
extern void record_histo_stat(uint32_t handle, double sample);
extern void record_tail_stat(uint32_t handle, bool dir, double sample);

extern uint32_t get_mean_count(uint32_t handle);
extern double get_mean_mean(uint32_t handle);
extern double get_mean_error(uint32_t handle);
extern double get_mean_variance(uint32_t handle);
extern uint32_t get_window_count(uint32_t handle);
extern double get_window_mean(uint32_t handle);
extern double get_window_variance(uint32_t handle);
extern uint32_t get_histo_count(uint32_t handle);
extern std::vector<std::pair<double, double> > get_histo_density(uint32_t handle);
extern double get_tail_quantile(uint32_t handle, bool dir, double ratio);

extern void close_mean_stat(uint32_t handle);
extern void close_window_stat(uint32_t handle);
extern void close_histo_stat(uint32_t handle);
extern void close_tail_stat(uint32_t handle, bool dir);

#endif
