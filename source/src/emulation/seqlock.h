#ifndef SEQLOCK_H
#define SEQLOCK_H

#include <atomic>
#include <stdint.h>
#include <string.h>
#include <type_traits>

// Hands a small state struct from the emulation core to the video core.
//
// run_frame() and prepare_frame()/render_row() run concurrently on two
// cores. Reading live sprite RAM / scroll registers from the video core
// races the game's vblank IRQ handler: sprites and scroll from different
// frames, a +-1px wobble that persists while the cores stay phase-locked.
//
//   emulation core                      video core
//   run_frame():                        prepare_frame():
//     ... emulate frame ...               state.read(video);
//     state.publish(fill);  <-- render    render from video.*, never live
//     vblank IRQ                          RAM/registers
//
// Seqlock: seq odd = write in progress. The writer never waits; the reader
// retries if a publish overlapped its copy. Single writer only.
// RAM cost: 2 x sizeof(T) (published copy here + caller's render copy).
static_assert(ATOMIC_INT_LOCK_FREE == 2, "Seqlock needs lock-free 32-bit atomics");

template <typename T> class Seqlock {
  static_assert(std::is_trivially_copyable<T>::value, "Seqlock payload is copied with memcpy");

public:
  // fill(T &) writes the new state directly into the published copy.
  template <typename Fill> void publish(Fill fill) {
    const uint32_t s = seq.load(std::memory_order_relaxed);
    seq.store(s + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);

    fill(data);

    seq.store(s + 2, std::memory_order_release);
  }

  void read(T &dst) const {
    uint32_t s;
    do {
      s = seq.load(std::memory_order_acquire);
      memcpy(&dst, &data, sizeof(T));
      std::atomic_thread_fence(std::memory_order_acquire);
    } while ((s & 1) || s != seq.load(std::memory_order_relaxed));
  }

private:
  std::atomic<uint32_t> seq{0};
  T data = {};
};

#endif
