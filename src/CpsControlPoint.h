#ifndef CPS_CONTROL_POINT_H
#define CPS_CONTROL_POINT_H

#include <cstddef>
#include <cstdint>

struct CpsControlPointResponse {
  uint8_t bytes[3] = {0x20, 0x00, 0x02};
};

class CpsControlPoint {
public:
  auto write(const uint8_t *request, const size_t length,
             const uint32_t baseWheelRevolutions) -> CpsControlPointResponse {
    CpsControlPointResponse response{};
    if (request == nullptr || length == 0) {
      return response;
    }

    response.bytes[1] = request[0];
    if (request[0] != 0x01) {
      return response;
    }
    if (length != 5) {
      response.bytes[2] = 0x03;
      return response;
    }

    const uint32_t requested = static_cast<uint32_t>(request[1]) |
                               (static_cast<uint32_t>(request[2]) << 8U) |
                               (static_cast<uint32_t>(request[3]) << 16U) |
                               (static_cast<uint32_t>(request[4]) << 24U);
    _offset = static_cast<int64_t>(requested) -
              static_cast<int64_t>(baseWheelRevolutions);
    response.bytes[2] = 0x01;
    return response;
  }

  auto apply(const uint32_t baseWheelRevolutions) const -> uint32_t {
    const int64_t adjusted =
        static_cast<int64_t>(baseWheelRevolutions) + _offset;
    if (adjusted <= 0) {
      return 0;
    }
    if (adjusted >= static_cast<int64_t>(UINT32_MAX)) {
      return UINT32_MAX;
    }
    return static_cast<uint32_t>(adjusted);
  }

private:
  int64_t _offset = 0;
};

#endif // CPS_CONTROL_POINT_H
