#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace mitsubishi_km05g {

// Temperature
const uint8_t MITSUBISHI_TEMP_MIN = 16;  // Celsius
const uint8_t MITSUBISHI_TEMP_MAX = 31;  // Celsius

class mitsubishi_km05gClimate : public climate_ir::ClimateIR {
 public:
  mitsubishi_km05gClimate() : climate_ir::ClimateIR(MITSUBISHI_TEMP_MIN, MITSUBISHI_TEMP_MAX) {}

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
   bool on_receive(remote_base::RemoteReceiveData data) override;

};

}  // namespace mitsubishi_km05g
}  // namespace esphome
