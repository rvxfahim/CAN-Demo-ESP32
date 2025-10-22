#include <Arduino.h>
#include <esp32_can.h>
#include <cstring>
#include "lecture.h"

namespace
{
constexpr uint32_t kBusSpeed = 500000U;
constexpr uint32_t kSendPeriodMs = 100U;
constexpr uint16_t kMaxSpeed = 4095U;
constexpr uint32_t kSerialBaudRate = 115200U;
}

static void SendClusterFrame(uint16_t speed, bool leftTurn, bool rightTurn)
{
  Cluster_t cluster{};
  cluster.speed = speed;
  cluster.Left_Turn_Signal = static_cast<uint8_t>(leftTurn);
  cluster.Right_Turn_Signal = static_cast<uint8_t>(rightTurn);

  uint8_t payload[Cluster_DLC] = {0U};
  uint8_t dlc = 0U;
  uint8_t ide = 0U;
  Pack_Cluster_lecture(&cluster, payload, &dlc, &ide);

  CAN_FRAME frame{};
  frame.id = Cluster_CANID;
  frame.extended = ide;
  frame.length = dlc;
  frame.rtr = 0U;
  memcpy(frame.data.bytes, payload, dlc);

  const bool sent = CAN0.sendFrame(frame);

  if (Serial)
  {
    Serial.printf("[CAN] TX Cluster: speed=%u left=%u right=%u dlc=%u result=%s\n",
                  static_cast<unsigned>(cluster.speed),
                  static_cast<unsigned>(cluster.Left_Turn_Signal),
                  static_cast<unsigned>(cluster.Right_Turn_Signal),
                  static_cast<unsigned>(frame.length),
                  sent ? "ok" : "fail");
  }
}

void setup()
{
  Serial.begin(kSerialBaudRate);
  delay(10);
  CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);
  CAN0.begin(kBusSpeed);

  if (Serial)
  {
    Serial.printf("[CAN] Initialized at %lu bps\n", static_cast<unsigned long>(kBusSpeed));
  }
}

void loop()
{
  static uint32_t lastSendTick = 0U;
  const uint32_t now = millis();
  if ((now - lastSendTick) < kSendPeriodMs)
  {
    delay(1);
    return;
  }

  lastSendTick = now;

  static uint16_t currentSpeed = 0U;
  static bool leftTurn = false;
  static bool rightTurn = true;

  SendClusterFrame(currentSpeed, leftTurn, rightTurn);

  constexpr uint16_t kSpeedStep = 64U;
  if (currentSpeed >= (kMaxSpeed - kSpeedStep))
  {
    currentSpeed = 0U;
    leftTurn = !leftTurn;
    rightTurn = !rightTurn;
  }
  else
  {
    currentSpeed = static_cast<uint16_t>(currentSpeed + kSpeedStep);
  }
}
