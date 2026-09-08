#include <unity.h>
#include "IBleService.h"

class RecordingBleService final : public IBleService {
public:
  CyclingTelemetry last{};

  void start() override {}
  void updateData(const CyclingTelemetry& telemetry) override { last = telemetry; }
  bool isConnected() override { return false; }
};

void test_ble_service_accepts_named_cycling_telemetry() {
  RecordingBleService service;
  const CyclingTelemetry telemetry{250, 12345, 54321};

  service.updateData(telemetry);

  TEST_ASSERT_EQUAL_UINT16(250, service.last.powerWatts);
  TEST_ASSERT_EQUAL_UINT32(12345, service.last.crankRevolutions);
  TEST_ASSERT_EQUAL_UINT16(54321, service.last.crankEventTime1024);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ble_service_accepts_named_cycling_telemetry);
  return UNITY_END();
}
