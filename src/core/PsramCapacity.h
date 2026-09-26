#pragma once
#include <stdint.h>

// Arduino ESP.getPsramSize() reports the heap-exposed PSRAM capacity, which
// can be a few KiB smaller than the physical N16R8 module's 8 MiB.
// Accept only a small reserved-memory margin; real missing/4 MiB PSRAM fails.
namespace VqeafMemory {
static constexpr uint32_t kExpectedPsramBytes = 8UL * 1024UL * 1024UL;
static constexpr uint32_t kReservedMarginBytes = 64UL * 1024UL;
inline bool hasN16R8Psram(bool detected, uint32_t reportedBytes) {
  return detected && reportedBytes >= kExpectedPsramBytes - kReservedMarginBytes;
}
}
