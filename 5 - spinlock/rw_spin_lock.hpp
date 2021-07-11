#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>

struct rw_spin_lock {
  void r_lock() {
    int retry_count = 0;

    while (true) {
      uint32_t oldVal = _value;
      uint32_t newVal = oldVal + 1;

      if (!(oldVal & WRITE_BIT) && _value.compare_exchange_strong(oldVal, newVal))
        break;

      retry_count++;
      if (oldVal & WRITE_BIT)
        std::this_thread::yield();
    }
  }

  void w_lock() {
    while (true) {
      uint32_t oldVal = _value;
      uint32_t newVal = oldVal | WRITE_BIT;
      if (!(oldVal & WRITE_BIT) && _value.compare_exchange_strong(oldVal, newVal))
        break;
      std::this_thread::yield();
    }

    while (true) {
      if (_value == WRITE_BIT) 
          break;
      std::this_thread::yield();
    }
  }

  void unlock() {
    if (_value == WRITE_BIT)
      _value = 0;
    else
      _value--;
  }

 private:
  std::atomic<uint32_t> _value;
  uint32_t WRITE_BIT = 1 << 31;
  std::thread::id _lock_owner_id;
};
