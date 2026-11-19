#pragma once
#include "rr_reflection_binding.h"
namespace control_nrd {
// CPU-only handoff within one native reflection invocation. Consume once;
// never carry an owner into another frame, view, command list or queue.
class SpecularHandoff {
 control_rr_reflection::Address owner_=0;
 control_rr_reflection::Context context_{};
public:
 void Arm(control_rr_reflection::Address owner,const control_rr_reflection::Context& context) noexcept {
  owner_=context.frame&&context.list&&context.queue&&context.device&&context.view?owner:0;context_=context;
 }
 control_rr_reflection::Address Take(const control_rr_reflection::Context& context) noexcept {
  const auto owner=owner_;owner_=0;
  return control_rr_reflection::Same(context_,context)?owner:0;
 }
};
}
