#include "CanInterface.h"

// Static member initialization
EventQueue* CanInterface::eventQueuePtr_ = nullptr;

CanInterface::CanInterface()
{
}

bool CanInterface::Init(EventQueue& eventQueue)
{
  eventQueuePtr_ = &eventQueue;

  // Configure CAN pins and initialize at 500 kbps
  CAN0.setCANPins(GPIO_NUM_35, GPIO_NUM_5);
  if (!CAN0.begin(500000))
  {
    return false;
  }

  // Try to set up dedicated mailbox for Cluster messages
  const int clusterMailbox = CAN0.watchFor(Cluster_CANID);
  if (clusterMailbox >= 0)
  {
    CAN0.setCallback(static_cast<uint8_t>(clusterMailbox), CanMsgHandler);
  }
  else
  {
    // Fallback to general callback if mailbox allocation failed
    CAN0.setGeneralCallback(CanMsgHandler);
  }

  return true;
}

void CanInterface::CanMsgHandler(CAN_FRAME* frame)
{
  if (frame == nullptr || eventQueuePtr_ == nullptr)
  {
    return;
  }

  // Validate frame ID, DLC, and IDE before parsing
  if ((frame->id != Cluster_CANID) || 
      (frame->length < Cluster_DLC) || 
      (frame->extended != Cluster_IDE))
  {
    return;
  }

  // Unpack the frame into a Cluster_t struct
  Cluster_t cluster{};
  Unpack_Cluster_lecture(&cluster, frame->data.bytes, frame->length);

  // Push event to queue from ISR context
  Event event = Event::MakeClusterFrame(cluster);
  eventQueuePtr_->PushFromISR(event);
}
