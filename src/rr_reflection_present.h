#pragma once
#include "rr_frame_slots.h"
namespace control_rr_reflection {
struct PresentStamp {std::uint64_t serial=0,present=0;};
class PresentLedger {
 std::array<PresentStamp,3> stamps_{};
public:
 bool Record(control_rr::Lease lease,std::uint64_t present) noexcept {
  if(lease.index>=3||!lease.serial||present==UINT64_MAX)return false;
  stamps_[lease.index]={lease.serial,present};return true;
 }
 bool Matches(control_rr::Lease lease,std::uint64_t present) const noexcept {
  return lease.index<3&&lease.serial&&stamps_[lease.index].serial==lease.serial&&stamps_[lease.index].present==present;
 }
 bool Covered(control_rr::Lease lease,std::uint64_t returnedPresent) const noexcept {
  if(lease.index>=3||!lease.serial||!returnedPresent)return false;
  const auto& stamp=stamps_[lease.index];
  return stamp.serial==lease.serial&&stamp.present<returnedPresent;
 }
};
}
