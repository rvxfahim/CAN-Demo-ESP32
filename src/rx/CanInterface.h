#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <esp32_can.h>
#include "lecture.h"
#include "EventQueue.h"

class CanInterface
{
public:
  CanInterface();
  
  bool Init(EventQueue& eventQueue);
  
  // ISR callback must be static to pass to C library
  static void CanMsgHandler(CAN_FRAME* frame);

private:
  static EventQueue* eventQueuePtr_;
};

#endif // CAN_INTERFACE_H
