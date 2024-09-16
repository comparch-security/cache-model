#include "util/detector.hpp"
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iterator>

void set_dist_normal_acc(uint64_t time,
                         const std::vector<uint64_t>& access_record,
                         const std::vector<uint64_t>& evict_record,
                         const std::list<double> x_factor,
                         std::list<std::vector<uint64_t> > *x_access,
                         std::list<std::vector<uint64_t> > *x_evict,
                         const std::list<double> y_factor,
                         std::list<std::vector<double> > *y_access,
                         std::list<std::vector<double> > *y_evict,
                         std::string fn)
{
  // IIR/FIR filter
  // prepare
  if(x_access->size() == 0) {
    for(auto x:x_factor) {
      x_access->push_front(std::vector<uint64_t>(access_record.size(), 0));
      x_evict->push_front(std::vector<uint64_t>(evict_record.size(), 0));
    }
    for(auto y:y_factor) {
      y_access->push_front(std::vector<double>(access_record.size(), 0.0));
      y_evict->push_front(std::vector<double>(evict_record.size(), 0.0));
    }
  }
  // record the x history
  x_access->push_front(access_record);
  x_evict->push_front(evict_record);
  x_access->pop_back();
  x_evict->pop_back();
  
  // calculate y and record y history
  std::vector<double> c_access(access_record.size(), 0.0);
  std::vector<double> c_evict(evict_record.size(), 0.0);
  for(size_t i=0; i<access_record.size(); i++) {
    std::list<std::vector<uint64_t> >::iterator xa = x_access->begin();
    std::list<std::vector<uint64_t> >::iterator xe = x_evict->begin();
    std::list<double>::const_iterator           xf = x_factor.begin();
    for(; xf != x_factor.end(); xa++, xe++, xf++) {
      c_access[i] += (*xa)[i] * (*xf);
      c_evict[i]  += (*xe)[i] * (*xf);
    }
    std::list<std::vector<double> >::iterator ya = y_access->begin();
    std::list<std::vector<double> >::iterator ye = y_evict->begin();
    std::list<double>::const_iterator         yf = y_factor.begin();
    for(; yf != y_factor.end(); ya++, ye++, yf++) {
      c_access[i] += (*ya)[i] * (*yf);
      c_evict[i]  += (*ye)[i] * (*yf);
    }
  }
  y_access->push_front(c_access);
  y_evict->push_front(c_evict);
  y_access->pop_back();
  y_evict->pop_back();

  std::ofstream ofile(fn, std::ios_base::app);
  ofile << time << ",access";
  for(auto a:c_access) ofile << "," << a;
  ofile << std::endl;
  ofile << time << ",evict";
  for(auto a:c_evict) ofile << "," << a;
  ofile << std::endl;
  ofile.close();
}

set_dist_proc_t
set_dist_normal_acc_gen(const std::list<double> x_factor,
                        std::list<std::vector<uint64_t> > *x_access,
                        std::list<std::vector<uint64_t> > *x_evict,
                        const std::list<double> y_factor,
                        std::list<std::vector<double> > *y_access,
                        std::list<std::vector<double> > *y_evict,
                        std::string fn)
{
  return std::bind(set_dist_normal_acc,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   std::placeholders::_3,
                   x_factor, x_access, x_evict,
                   y_factor, y_access, y_evict,
                   fn);
}

void AbnormalSetDetector_t::detect(uint64_t time,
                                   const std::vector<uint64_t>& access_record,
                                   const std::vector<uint64_t>& evict_record
                                   )
{
  // calculate the scaled evict distribution
  std::vector<double> evict_scaled(evict_record.begin(), evict_record.end());
  // calculate the root mean square
  double qrm  = sqrt(std::accumulate(evict_record.begin(), evict_record.end(), 0,
                                     [](double q, const uint64_t& d){return q + d * d;})
                     / (nset-1.0)
                     );
  double mu   = std::accumulate(evict_record.begin(), evict_record.end(), 0) / (double)(nset);
  //std::cerr << time << ": calculated squared root mean = " << qrm << " and mu = " << mu << std::endl;
  for(auto& s:evict_scaled) s = qrm == 0.0 ? 0.0 : s * (s - mu) / qrm;

  // record the evict history
  std::unordered_set<uint32_t> cur_detected_sets;
  for(size_t i=0; i<nset; i++) {
    std::list<std::vector<double> >::iterator eh = scaled_evict_hist.begin();
    std::list<double>::const_iterator         f  = factor.begin();
    for(; f != factor.end(); eh++, f++) {
      (*eh)[i] = (*f) * (*eh)[i] + evict_scaled[i];
      if((*eh)[i] >= threshold) {
        cur_detected_sets.insert(i);
        if(!detected_sets.count(i)) {
          std::cerr << time << ": Detect set adnormal set in " << cache_name
                    << " set " << i
                    << " with z-score " << (*eh)[i]
                    << " (av facor " << *f <<")."
                    << std::endl;
          detected_sets.insert(i);
          abnormals++;
        }
      }
    }
  }

  detected_sets = cur_detected_sets;
  samples++;
}

set_dist_proc_t
AbnormalSetDetector_t::gen() {
  return std::bind(&AbnormalSetDetector_t::detect,
                   this,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   std::placeholders::_3);
}

void AbnormalSetDetector_t::report() {
  std::cerr << "Detected " << abnormals << " in " << samples << " samples." << std::endl;
}

void AbnormalSetRecorder_t::detect(uint64_t time,
                                   const std::vector<uint64_t>& access_record,
                                   const std::vector<uint64_t>& evict_record
                                   )
{
  std::ofstream ofile(fn, std::ios_base::app);
  ofile << time;

  // calculate the scaled evict distribution
  std::vector<double> evict_scaled(evict_record.begin(), evict_record.end());
  // calculate the root mean square
  double qrm  = sqrt(std::accumulate(evict_record.begin(), evict_record.end(), 0,
                                     [](double q, const uint64_t& d){return q + d * d;})
                     / (nset-1.0)
                     );
  double mu   = std::accumulate(evict_record.begin(), evict_record.end(), 0) / (double)(nset);
  for(auto& s:evict_scaled) s = qrm == 0.0 ? 0.0 : s * (s - mu) / qrm;
  auto max_it = std::max_element(evict_scaled.begin(), evict_scaled.end());
  ofile << "," << mu << "," << *max_it << "," << std::distance(evict_scaled.begin(), max_it);

  // calculate the scaled evict distribution
  std::vector<double> access_scaled(access_record.begin(), access_record.end());
  qrm  = sqrt(std::accumulate(access_record.begin(), access_record.end(), 0,
                              [](double q, const uint64_t& d){return q + d * d;})
              / (nset-1.0)
              );
  mu   = std::accumulate(access_record.begin(), access_record.end(), 0) / (double)(nset);
  for(auto& s:access_scaled) s = qrm == 0.0 ? 0.0 : s * (s - mu) / qrm;
  max_it = std::max_element(access_scaled.begin(), access_scaled.end());
  ofile << "," << mu << "," << *max_it << "," << std::distance(evict_scaled.begin(), max_it);

  ofile << std::endl;
  ofile.close();
}

set_dist_proc_t
AbnormalSetRecorder_t::gen() {
  return std::bind(&AbnormalSetRecorder_t::detect,
                   this,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   std::placeholders::_3);
}

void set_dist_weighted_stat_acc(uint64_t time,
                                const std::vector<uint64_t>& access_record,
                                const std::vector<uint64_t>& evict_record,
                                std::string fn)
{
  std::vector<double> access_bins(64, 0), evict_bins(64, 0);
  std::ofstream ofile(fn, std::ios_base::app);
  for(uint32_t i=0; i<access_record.size(); i++)
    if(access_record[i] < 63) access_bins[access_record[i]] += 1.0;
    else                      access_bins[63] += 1.0;
  ofile << time << ",access";
  for(uint32_t i=0; i<64; i++)
    ofile << "," << access_bins[i] * i;
  ofile << std::endl;
  for(uint32_t i=0; i<evict_record.size(); i++)
    if(evict_record[i] < 63) evict_bins[evict_record[i]] += 1.0;
    else                     evict_bins[63] += 1.0;
  ofile << time << ",evict";
  for(uint32_t i=0; i<64; i++)
    ofile << "," << evict_bins[i] * i;
  ofile << std::endl;
  ofile.close();
}

set_dist_proc_t set_dist_weighted_stat_acc_gen(std::string fn) {
  return std::bind(set_dist_weighted_stat_acc,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   std::placeholders::_3,
                   fn);
}
