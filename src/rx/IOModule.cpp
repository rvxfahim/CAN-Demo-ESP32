#include "IOModule.h"

IOModule::IOModule()
{
}

bool IOModule::Init(uint8_t leftPin, uint8_t rightPin, bool activeHigh)
{
  leftPin_ = leftPin;
  rightPin_ = rightPin;
  activeHigh_ = activeHigh;

  pinMode(leftPin_, OUTPUT);
  pinMode(rightPin_, OUTPUT);

  // default OFF
  digitalWrite(leftPin_, activeHigh_ ? LOW : HIGH);
  digitalWrite(rightPin_, activeHigh_ ? LOW : HIGH);
  leftOn_ = false;
  rightOn_ = false;
  nextToggleMs_ = 0;
  lastUpdateMs_ = 0;
  leftRequested_ = false;
  rightRequested_ = false;
  return true;
}

bool IOModule::Start(MessageRouter& router)
{
  return router.SubscribeCluster(&IOModule::ClusterCb_, this);
}

void IOModule::Stop(MessageRouter& router)
{
  router.UnsubscribeCluster(&IOModule::ClusterCb_, this);
}

void IOModule::Update(uint32_t nowMs)
{
  // Staleness: if no update for > 1000 ms, force off
  if (lastUpdateMs_ != 0 && (nowMs - lastUpdateMs_) > 1000)
  {
    leftRequested_ = false;
    rightRequested_ = false;
  }

  // Phase sync requested when a rising edge was detected in OnCluster_
  if (phaseSync_)
  {
    // Start from ON phase for any requested side, align next toggle to half period
    leftOn_ = leftRequested_;
    rightOn_ = rightRequested_;
    nextToggleMs_ = nowMs + kBlinkPeriodMs_ / 2;
    ApplyOutputs_();
    phaseSync_ = false;
    return; // let next tick handle regular blinking
  }

  // If requested, blink; otherwise ensure off
  const bool needBlink = leftRequested_ || rightRequested_;
  if (needBlink)
  {
    if (nowMs >= nextToggleMs_)
    {
      nextToggleMs_ = nowMs + kBlinkPeriodMs_ / 2; // toggle every half period
      if (leftRequested_) leftOn_ = !leftOn_; else leftOn_ = false;
      if (rightRequested_) rightOn_ = !rightOn_; else rightOn_ = false;
      ApplyOutputs_();
    }
  }
  else
  {
    // force off if was on
    if (leftOn_ || rightOn_)
    {
      leftOn_ = false;
      rightOn_ = false;
      ApplyOutputs_();
    }
  }
}

void IOModule::ClusterCb_(const Cluster_t& msg, uint32_t tsMs, void* ctx)
{
  auto* self = static_cast<IOModule*>(ctx);
  if (!self) return;
  self->OnCluster_(msg, tsMs);
}

void IOModule::OnCluster_(const Cluster_t& msg, uint32_t tsMs)
{
  lastUpdateMs_ = tsMs;
  const bool newLeftReq = msg.Left_Turn_Signal != 0;
  const bool newRightReq = msg.Right_Turn_Signal != 0;

  // Edge-detect start requests to align blink phase once; do not touch GPIO here
  const bool risingLeft = newLeftReq && !leftRequested_;
  const bool risingRight = newRightReq && !rightRequested_;

  leftRequested_ = newLeftReq;
  rightRequested_ = newRightReq;

  if (risingLeft || risingRight)
  {
    // Ask Update() to sync phase and reflect ON immediately on its next tick
    phaseSync_ = true;
  }
  // Do not reset nextToggleMs_ on every message; let Update() drive cadence
}

void IOModule::ApplyOutputs_()
{
  const int leftLevel = (leftOn_ ? (activeHigh_ ? HIGH : LOW) : (activeHigh_ ? LOW : HIGH));
  const int rightLevel = (rightOn_ ? (activeHigh_ ? HIGH : LOW) : (activeHigh_ ? LOW : HIGH));
  digitalWrite(leftPin_, leftLevel);
  digitalWrite(rightPin_, rightLevel);
}
