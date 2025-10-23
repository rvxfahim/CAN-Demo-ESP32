#include "MessageRouter.h"

MessageRouter::MessageRouter()
{
}

bool MessageRouter::Init(std::size_t maxClusterSubs)
{
  if (maxClusterSubs == 0) return false;
  maxClusterSubs_ = maxClusterSubs;
  // Static allocation using new to keep simple; can be replaced by fixed array if needed
  clusterSubs_ = new ClusterSub[maxClusterSubs_];
  if (!clusterSubs_) return false;
  clusterSubCount_ = 0;
  haveCluster_ = false;
  lastClusterTsMs_ = 0;

  // Allocate default capacity for status subscribers (same as cluster by default)
  maxStatusSubs_ = maxClusterSubs_;
  statusSubs_ = new SystemStatusSub[maxStatusSubs_];
  if (!statusSubs_) return false;
  statusSubCount_ = 0;
  haveStatus_ = false;
  lastStatusTsMs_ = 0;
  return true;
}

bool MessageRouter::SubscribeCluster(ClusterCallback cb, void* ctx)
{
  if (!cb || !clusterSubs_ || clusterSubCount_ >= maxClusterSubs_) return false;
  // Avoid duplicates
  for (std::size_t i = 0; i < clusterSubCount_; ++i)
  {
    if (clusterSubs_[i].cb == cb && clusterSubs_[i].ctx == ctx) return true;
  }
  clusterSubs_[clusterSubCount_++] = ClusterSub{cb, ctx};
  return true;
}

void MessageRouter::UnsubscribeCluster(ClusterCallback cb, void* ctx)
{
  if (!clusterSubs_ || clusterSubCount_ == 0) return;
  for (std::size_t i = 0; i < clusterSubCount_; ++i)
  {
    if (clusterSubs_[i].cb == cb && clusterSubs_[i].ctx == ctx)
    {
      // compact
      for (std::size_t j = i + 1; j < clusterSubCount_; ++j)
      {
        clusterSubs_[j - 1] = clusterSubs_[j];
      }
      --clusterSubCount_;
      break;
    }
  }
}

void MessageRouter::PublishCluster(const Cluster_t& msg, uint32_t tsMs)
{
  lastCluster_ = msg;
  lastClusterTsMs_ = tsMs;
  haveCluster_ = true;
  // fan-out
  for (std::size_t i = 0; i < clusterSubCount_; ++i)
  {
    if (clusterSubs_[i].cb)
    {
      clusterSubs_[i].cb(msg, tsMs, clusterSubs_[i].ctx);
    }
  }
}

bool MessageRouter::GetLastCluster(Cluster_t& out, uint32_t& tsMs) const
{
  if (!haveCluster_) return false;
  out = lastCluster_;
  tsMs = lastClusterTsMs_;
  return true;
}

bool MessageRouter::GetLastSeenMs(uint32_t& tsMs) const
{
  if (!haveCluster_) return false;
  tsMs = lastClusterTsMs_;
  return true;
}

bool MessageRouter::SubscribeSystemStatus(SystemStatusCallback cb, void* ctx)
{
  if (!cb || !statusSubs_ || statusSubCount_ >= maxStatusSubs_) return false;
  for (std::size_t i = 0; i < statusSubCount_; ++i)
  {
    if (statusSubs_[i].cb == cb && statusSubs_[i].ctx == ctx) return true;
  }
  statusSubs_[statusSubCount_++] = SystemStatusSub{cb, ctx};
  return true;
}

void MessageRouter::UnsubscribeSystemStatus(SystemStatusCallback cb, void* ctx)
{
  if (!statusSubs_ || statusSubCount_ == 0) return;
  for (std::size_t i = 0; i < statusSubCount_; ++i)
  {
    if (statusSubs_[i].cb == cb && statusSubs_[i].ctx == ctx)
    {
      for (std::size_t j = i + 1; j < statusSubCount_; ++j)
      {
        statusSubs_[j - 1] = statusSubs_[j];
      }
      --statusSubCount_;
      break;
    }
  }
}

void MessageRouter::PublishSystemStatus(const SystemStatus& status, uint32_t tsMs)
{
  lastStatus_ = status;
  lastStatusTsMs_ = tsMs;
  haveStatus_ = true;
  for (std::size_t i = 0; i < statusSubCount_; ++i)
  {
    if (statusSubs_[i].cb)
    {
      statusSubs_[i].cb(status, tsMs, statusSubs_[i].ctx);
    }
  }
}

bool MessageRouter::GetLastSystemStatus(SystemStatus& out, uint32_t& tsMs) const
{
  if (!haveStatus_) return false;
  out = lastStatus_;
  tsMs = lastStatusTsMs_;
  return true;
}
