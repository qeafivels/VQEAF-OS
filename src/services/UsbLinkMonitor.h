#pragma once
#include <stdint.h>

// Debounces native USB Serial/JTAG HOST detection without claiming to measure
// Type-C VBUS. Charging-only cables/chargers require a separate VBUS sense circuit.
class UsbLinkMonitor {
public:
  enum class Event : uint8_t { None, HostConnected, HostDisconnected };
  Event sample(bool hostPresent, uint32_t now) {
    if (!initialized) {
      initialized=true;stable=false;candidate=hostPresent;
      changedAt=now;return Event::None;
    }
    if(hostPresent!=candidate) {candidate=hostPresent;changedAt=now;}
    if(candidate!=stable && (uint32_t)(now-changedAt)>=750U) {
      stable=candidate;
      return stable?Event::HostConnected:Event::HostDisconnected;
    }
    return Event::None;
  }
  bool hostConnected() const {return stable;}
private:
  bool initialized=false,stable=false,candidate=false;
  uint32_t changedAt=0;
};
