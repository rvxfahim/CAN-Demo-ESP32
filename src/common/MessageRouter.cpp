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
