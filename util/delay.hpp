#ifndef CM_UTIL_DELAY_HPP_
#define CM_UTIL_DELAY_HPP_

class DelaySim
{
  uint64_t delay;

public:
  DelaySim() : delay(0) {}
  DelaySim(uint32_t delay) : delay(delay) {}

  virtual void latency_acc(uint64_t *latency, uint32_t extra_param = 0) const {
    if(delay && latency) *latency += delay;
  }

  void set_delay(uint64_t d) { delay = d; }

};

#endif
