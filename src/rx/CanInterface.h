/**
 * @file CanInterface.h
 * @brief RX-side CAN driver integration and ISR hook.
 *
 * Sets up filters and the ISR callback to receive frames, unpacks DBC Cluster and
 * forwards as EventQueue entries for task-level processing.
 */
#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <esp32_can.h>
#include "lecture.h"
#include "EventQueue.h"

/**
 * @class CanInterface
 * @brief Initializes driver, sets mailbox filters, and bridges ISR -> EventQueue.
 */
class CanInterface
{
public:
  CanInterface();
  
  /** Initialize CAN hardware and register ISR handler. */
  bool Init(EventQueue& eventQueue);
  
  // ISR callback must be static to pass to C library
  /** ISR entrypoint registered with the CAN driver. */
  static void CanMsgHandler(CAN_FRAME* frame);

private:
  static EventQueue* eventQueuePtr_;
};

#endif // CAN_INTERFACE_H
