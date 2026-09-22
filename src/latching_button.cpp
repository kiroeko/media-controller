#include "latching_button.h"

namespace {
// Vendor: latching on drives SIG high and lights the LED.
constexpr bool kSigActiveHigh = true;

// Bench decision pending: None assumes the module defines its own off level;
// switch to InputPull::Down if SIG is found floating while released.
constexpr InputPull kSigPull = InputPull::None;
}  // namespace

LatchingButton::LatchingButton(uint sig_pin)
    : sig_(sig_pin, kSigActiveHigh, kSigPull) {}

void LatchingButton::init(uint32_t now_ms) {
    sig_.init(now_ms);
}

void LatchingButton::update(uint32_t now_ms) {
    sig_.update(now_ms);
}

bool LatchingButton::is_on() const {
    return sig_.is_active();
}
