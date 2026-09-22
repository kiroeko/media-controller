#include "device/mode_device.h"

namespace {
// Vendor: latching on drives SIG high and lights the LED.
constexpr bool kSigActiveHigh = true;

// Bench decision pending: None assumes the module defines its own off level;
// switch to InputPull::Down if SIG is found floating while released.
constexpr InputPull kSigPull = InputPull::None;
}  // namespace

ModeDevice::ModeDevice(uint sig_pin)
    : sig_(sig_pin, kSigActiveHigh, kSigPull) {}

void ModeDevice::init(uint32_t now_ms) {
    sig_.init(now_ms);
}

void ModeDevice::update(uint32_t now_ms) {
    sig_.update(now_ms);
}

bool ModeDevice::is_on() const {
    return sig_.is_active();
}
