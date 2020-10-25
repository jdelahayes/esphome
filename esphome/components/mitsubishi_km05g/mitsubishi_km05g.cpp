#include "mitsubishi_km05g.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mitsubishi_km05g {

static const char *TAG = "mitsubishi_km05g.climate";

const uint32_t MITSUBISHI_OFF = 0x00;

const uint8_t MITSUBISHI_COOL = 0x18;
const uint8_t MITSUBISHI_DRY = 0x10;
const uint8_t MITSUBISHI_AUTO = 0x20;
const uint8_t MITSUBISHI_HEAT = 0x08;
const uint8_t MITSUBISHI_FAN_AUTO = 0x00;

// Pulse parameters in usec
// const uint16_t MITSUBISHI_HEADER_MARK = 3500;
// const uint16_t MITSUBISHI_HEADER_SPACE = 1700;
// const uint16_t MITSUBISHI_BIT_MARK = 430;
// const uint16_t MITSUBISHI_ONE_SPACE = 1250;
// const uint16_t MITSUBISHI_ZERO_SPACE = 390;
// const uint16_t MITSUBISHI_MIN_GAP = 17500;
const uint16_t MITSUBISHI_HEADER_MARK = 3600;
const uint16_t MITSUBISHI_HEADER_SPACE = 1700;
const uint16_t MITSUBISHI_BIT_MARK = 460;
const uint16_t MITSUBISHI_ONE_SPACE = 1260;
const uint16_t MITSUBISHI_ZERO_SPACE = 400;
const uint16_t MITSUBISHI_MIN_GAP = 17500;

void mitsubishi_km05gClimate::transmit_state() {
  uint32_t remote_state[18] = {
    0x23, // 0 -> ?
    0xCB, // 1 -> ?
    0x26, // 2 -> ?
    0x01, // 3 -> ?
    0x00, // 4 -> ?
    0x20, // 5 -> ON / OFF
    0x20, // 6 -> Mode
    0x00, // 7 -> Temperature
    0x30, // 8 -> ?
    0x58, // 9 -> ?
    0x61, //10 -> ?
    0x00, //11 -> ?
    0x00, //12 -> ?
    0x00, //13 -> ?
    0x10, //14 -> ?
    0x40, //15 -> ?
    0x00, //16 -> Fan / Vane
    0x00  //17 -> Checksum
  };

  // Set temperature
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      remote_state[6] = MITSUBISHI_COOL;
      break;
    case climate::CLIMATE_MODE_HEAT:
      remote_state[6] = MITSUBISHI_HEAT;
      break;
    case climate::CLIMATE_MODE_AUTO:
      remote_state[6] = MITSUBISHI_AUTO;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      remote_state[5] = MITSUBISHI_OFF;
      break;
  }

  // Set temperature
  remote_state[7] =
      (uint8_t) roundf(clamp(this->target_temperature, MITSUBISHI_TEMP_MIN, MITSUBISHI_TEMP_MAX) - MITSUBISHI_TEMP_MIN);

  ESP_LOGD(TAG, "Sending target temp: %.1f state: %02X mode: %02X temp: %02X", this->target_temperature,
           remote_state[5], remote_state[6], remote_state[7]);

  // Checksum
  for (int i = 0; i < 17; i++) {
    remote_state[17] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();

  data->set_carrier_frequency(38000);
  // repeat twice
  for (uint16_t r = 0; r < 2; r++) {
    // Header
    data->mark(MITSUBISHI_HEADER_MARK);
    data->space(MITSUBISHI_HEADER_SPACE);
    // Data
    for (uint8_t i : remote_state)
      for (uint8_t j = 0; j < 8; j++) {
        data->mark(MITSUBISHI_BIT_MARK);
        bool bit = i & (1 << j);
        data->space(bit ? MITSUBISHI_ONE_SPACE : MITSUBISHI_ZERO_SPACE);
      }
    // Footer
    if (r == 0) {
      data->mark(MITSUBISHI_BIT_MARK);
      data->space(MITSUBISHI_MIN_GAP);  // Pause before repeating
    }
  }
  data->mark(MITSUBISHI_BIT_MARK);

 ESP_LOGD(TAG, "Sending : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           remote_state[0],
           remote_state[1],
           remote_state[2],
           remote_state[3],
           remote_state[4],
           remote_state[5],   // On / Off
           remote_state[6],   // Mode
           remote_state[7],   // Temperature
           remote_state[8],
           remote_state[9],   // Fan / Vane
           remote_state[10],  // Fan / Vane
           remote_state[11],
           remote_state[12],
           remote_state[13],
           remote_state[14],
           remote_state[15],
           remote_state[16],
           remote_state[17]  // Checksum
  );

  transmit.perform();
}

bool mitsubishi_km05gClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t message[18] = {0};
  uint8_t message_length = 18;

  /* Validate header */
  if (!data.expect_item(MITSUBISHI_HEADER_MARK, MITSUBISHI_HEADER_SPACE)) {
    ESP_LOGD(TAG, "Header error");
    return false;
  }

  /* Decode bytes */
  for (uint8_t byte = 0; byte < message_length; byte++) {
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(MITSUBISHI_BIT_MARK, MITSUBISHI_ONE_SPACE)) {
        message[byte] |= 1 << bit;
      } else if (data.expect_item(MITSUBISHI_BIT_MARK, MITSUBISHI_ZERO_SPACE)) {
        /* Bit is already clear */
      } else {
        ESP_LOGD(TAG, "Bit error %04u / %1u", byte, bit);
        return false;
      }
    }
  }

  ESP_LOGD(TAG, "Decoded : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
          message[0],
          message[1],
          message[2],
          message[3],
          message[4],
          message[5],   // ON/OFF
          message[6],   // Mode
          message[7],   // Temperature
          message[8],
          message[9],   // Fan / Vane
          message[10],  // Fan / Vane
          message[11],
          message[12],
          message[13],
          message[14],
          message[15],
          message[16],
          message[17]   // Checksum
  );

  // Calculate expected checksum
  uint8_t expected_checksum = 0;
  for (int i = 0; i < 17; i++) {
    expected_checksum += message[i];
  }

  if(!(expected_checksum == message[17])) {
    ESP_LOGD(TAG, "Checksum error. Expected/Received : %02X / %02X", expected_checksum, message[17]);
    return false;
  }

  /* Get the mode. */
  switch (message[6]) {

    case MITSUBISHI_HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;

    case MITSUBISHI_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      break;

    case MITSUBISHI_AUTO:
      this->mode = climate::CLIMATE_MODE_AUTO;
      break;

    case MITSUBISHI_DRY:
      this->mode = climate::CLIMATE_MODE_DRY;
      break;

    case MITSUBISHI_OFF:
    default:
      this->mode = climate::CLIMATE_MODE_OFF;

  }

  /* Set the target temperature */
  this->target_temperature = message[7] + MITSUBISHI_TEMP_MIN;

  this->publish_state();
  return true;

}


}  // namespace mitsubishi_km05g
}  // namespace esphome
