#pragma once
// The side thumbwheel. It is a potentiometer with end stops, not an endless encoder, so its
// angle maps straight onto the list: the travel is split into one zone per item. Plain logic
// with no hardware calls, so the host preview can test it.
#include <stdint.h>

namespace ui {

struct Dial {
  int32_t filt = -1;  // smoothed reading, 0..4095; -1 until the first sample
  int zone = -1;      // list position the wheel is in; -1 until the first sample

  // Feed one raw 12-bit ADC reading and the list length. Returns the new position when the
  // wheel moves to another item, otherwise -1. The first reading only records where the wheel
  // is, so powering on never changes the selection.
  int update(int raw, int count) {
    if (count < 1) return -1;
    if (raw < 0) raw = 0;
    if (raw > 4095) raw = 4095;
    filt = filt < 0 ? raw : filt + (raw - filt) / 4;
    const int width = 4096 / count;
    int z = filt / width;
    if (z >= count) z = count - 1;
    if (zone >= 0 && zone < count && z != zone) {
      // Hysteresis: stay put until the wheel is clearly inside the next zone.
      const int margin = width / 6;
      if (filt >= zone * width - margin && filt < (zone + 1) * width + margin) z = zone;
    }
    if (z == zone) return -1;
    const bool first = zone < 0;
    zone = z;
    return first ? -1 : z;
  }
};

}  // namespace ui
