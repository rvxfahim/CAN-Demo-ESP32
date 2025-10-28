#include <Arduino.h>
#include <esp32_can.h>
#include <cstring>
#include "lecture.h"  // Generated from Lecture.dbc using cantools or c-coderdbc

namespace
{
constexpr uint32_t kBusSpeed = 500000U;
constexpr uint32_t kSendPeriodMs = 100U;
constexpr uint32_t kSerialBaudRate = 115200U;
}

// TODO: Define your CAN message data structure here
// Hint: Check lecture.h for the generated Cluster_t structure
// or define your own based on Lecture.dbc


// TODO: Implement your message packing function
// Option 1: Use generated helper functions from lecture.h (e.g., Pack_Cluster_lecture)
// Option 2: Implement your own bit packing based on the DBC signal definitions
//
// This function should take your data and pack it into a CAN frame
static void SendClusterFrame(uint16_t speed, bool leftTurn, bool rightTurn)
{
  // TODO: Create and populate your message structure
  CAN_FRAME frame{};
  // TODO: Pack the data into a byte array
  // Option 1 - Using generated functions:
  // check the generated header file for details
  // uint8_t payload[Cluster_DLC] = {0U};
  // uint8_t dlc = 0U;
  // uint8_t ide = 0U;
  // use the generated packing function
  // generated_function(address_of_your_structure, payload, &dlc, &ide);
  // put the payload into the CAN_FRAME data, check the structure definition for details
  
  // Option 2 - Manual packing:
  // Calculate byte positions and bit shifts based on DBC signal definitions
  // populate frame.data.bytes[] accordingly using speed, leftTurn, rightTurn and your own function.

  // further populate the CAN frame properties required for transmission
  
  // frame.id = ???;           // TODO: Set the CAN ID from your DBC
  // frame.extended = ???;      // TODO: Standard (0) or Extended (1)?
  // frame.length = ???;        // TODO: Set DLC (data length code)
  // frame.rtr = 0U;


  // Send the frame (this part is provided)
  // this is a driver call to send the CAN frame once everything is populated
  const bool sent = CAN0.sendFrame(frame);

  // Debug output
  if (Serial)
  {
    Serial.printf("[CAN] TX Result: %s\n", sent ? "OK" : "FAILED");
    // TODO: Add more debug info about your message content
  }
}

void setup()
{
  Serial.begin(kSerialBaudRate);
  delay(10);
  
  // CAN driver setup (provided)
  CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);
  CAN0.begin(kBusSpeed);

  if (Serial)
  {
    Serial.printf("[CAN] Initialized at %lu bps\n", static_cast<unsigned long>(kBusSpeed));
    Serial.println("[Exercise] Implement the SendClusterFrame function to transmit CAN messages");
  }
}

void loop()
{
  static uint32_t lastSendTick = 0U;
  const uint32_t now = millis();
  
  // Send messages periodically
  if ((now - lastSendTick) < kSendPeriodMs)
  {
    delay(1);
    return;
  }

  lastSendTick = now;
  
  // TODO: Modify these test values or create your own test patterns
  uint16_t testSpeed = 100U;      // Example speed value
  bool testLeft = true;            // Example left turn signal
  bool testRight = false;          // Example right turn signal
  
  SendClusterFrame(testSpeed, testLeft, testRight);
}