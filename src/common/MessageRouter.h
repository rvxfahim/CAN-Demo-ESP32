#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H

#include <cstdint>
#include <cstddef>
#include "lecture.h"

// Lightweight router to fan out typed messages (initially Cluster_t)
// - Static storage, ISR-safe API by design (but Publish called from task context)
// - Subscribers are invoked in task context only

class MessageRouter
{
public:
  using ClusterCallback = void(*)(const Cluster_t& msg, uint32_t tsMs, void* ctx);
  struct SystemStatus {
    // System state as numeric code (e.g., static_cast<uint8_t>(SystemState)) to avoid header coupling
    uint8_t state;
    bool outputsEnabled;
  };
  using SystemStatusCallback = void(*)(const SystemStatus& status, uint32_t tsMs, void* ctx);

  MessageRouter();

  bool Init(std::size_t maxClusterSubs = 8);

  // Typed Cluster topic
  bool SubscribeCluster(ClusterCallback cb, void* ctx);
  void UnsubscribeCluster(ClusterCallback cb, void* ctx);

  // Publish latest Cluster_t to all subscribers; should be called from loop/task context
  void PublishCluster(const Cluster_t& msg, uint32_t tsMs);

  // Access last value (if any). Returns false if none published yet.
  bool GetLastCluster(Cluster_t& out, uint32_t& tsMs) const;

  // Last-seen timestamp helper (ms since boot)
  bool GetLastSeenMs(uint32_t& tsMs) const;

  // SystemStatus topic
  bool SubscribeSystemStatus(SystemStatusCallback cb, void* ctx);
  void UnsubscribeSystemStatus(SystemStatusCallback cb, void* ctx);
  void PublishSystemStatus(const SystemStatus& status, uint32_t tsMs);
  bool GetLastSystemStatus(SystemStatus& out, uint32_t& tsMs) const;

private:
  struct ClusterSub {
    ClusterCallback cb;
    void* ctx;
  };

  struct SystemStatusSub {
    SystemStatusCallback cb;
    void* ctx;
  };

  ClusterSub* clusterSubs_ = nullptr;
  std::size_t maxClusterSubs_ = 0;
  std::size_t clusterSubCount_ = 0;

  // Sticky cache
  bool haveCluster_ = false;
  Cluster_t lastCluster_{};
  uint32_t lastClusterTsMs_ = 0;

  // SystemStatus storage
  SystemStatusSub* statusSubs_ = nullptr;
  std::size_t maxStatusSubs_ = 8;
  std::size_t statusSubCount_ = 0;
  bool haveStatus_ = false;
  SystemStatus lastStatus_{};
  uint32_t lastStatusTsMs_ = 0;
};

#endif // MESSAGE_ROUTER_H
