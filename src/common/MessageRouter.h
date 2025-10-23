/**
 * @file MessageRouter.h
 * @brief Lightweight publish/subscribe message bus for typed data across RX runtime.
 *
 * Responsibilities:
 * - Fan-out of the DBC-generated Cluster_t payload to interested consumers (sticky last value)
 * - Optional publication of system status snapshots to gate IO and UI safely
 *
 * Design notes:
 * - All subscription and publish calls are intended from task/loop context (not ISR)
 * - ISR should queue raw frames elsewhere; decode and publish in task context
 * - Storage is static, with configurable subscriber capacity
 */
#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H

#include <cstdint>
#include <cstddef>
#include "lecture.h"

// Lightweight router to fan out typed messages (initially Cluster_t)
// - Static storage, ISR-safe API by design (but Publish called from task context)
// - Subscribers are invoked in task context only

/**
 * @class MessageRouter
 * @brief Typed topics for CAN cluster and system status with sticky last values.
 */
class MessageRouter
{
public:
  /** Callback signature for Cluster topic subscribers */
  using ClusterCallback = void(*)(const Cluster_t& msg, uint32_t tsMs, void* ctx);
  /** Lightweight snapshot of system state for consumers (e.g., IO gating) */
  struct SystemStatus {
    // System state as numeric code (e.g., static_cast<uint8_t>(SystemState)) to avoid header coupling
    uint8_t state;
    bool outputsEnabled;
  };
  /** Callback signature for SystemStatus topic subscribers */
  using SystemStatusCallback = void(*)(const SystemStatus& status, uint32_t tsMs, void* ctx);

  /** Construct an empty router (allocate subscribers via Init). */
  MessageRouter();

  /**
   * @brief Initialize storage for subscribers.
   * @param maxClusterSubs Maximum number of Cluster subscribers to support.
   * @return true on success, false if allocation fails.
   */
  bool Init(std::size_t maxClusterSubs = 8);

  // Typed Cluster topic
  /** Register a callback for Cluster messages. */
  bool SubscribeCluster(ClusterCallback cb, void* ctx);
  /** Unregister a previously registered Cluster callback. */
  void UnsubscribeCluster(ClusterCallback cb, void* ctx);

  // Publish latest Cluster_t to all subscribers; should be called from loop/task context
  /** Publish a new Cluster message to all subscribers. */
  void PublishCluster(const Cluster_t& msg, uint32_t tsMs);

  // Access last value (if any). Returns false if none published yet.
  /** Retrieve last published Cluster message if available. */
  bool GetLastCluster(Cluster_t& out, uint32_t& tsMs) const;

  // Last-seen timestamp helper (ms since boot)
  /** Retrieve timestamp of last Cluster publication if available. */
  bool GetLastSeenMs(uint32_t& tsMs) const;

  // SystemStatus topic
  /** Register a callback for SystemStatus updates. */
  bool SubscribeSystemStatus(SystemStatusCallback cb, void* ctx);
  /** Unregister a previously registered SystemStatus callback. */
  void UnsubscribeSystemStatus(SystemStatusCallback cb, void* ctx);
  /** Publish a new SystemStatus snapshot to all subscribers. */
  void PublishSystemStatus(const SystemStatus& status, uint32_t tsMs);
  /** Retrieve last published SystemStatus if available. */
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
