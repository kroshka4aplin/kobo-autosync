// SPDX-License-Identifier: MIT
#pragma once
#include <string>

// No Qt or device dependencies: the same policy is exercised by host tests.
struct SyncPolicy {
    enum Phase { Idle, Connecting, Requested, Running, Draining, Cancelling };
    enum Trigger { DailyWake, MenuRequest, NativeRequest };
    Phase phase = Idle;
    Trigger trigger = DailyWake;
    bool fullSuccess = false;
    bool failed = false;
    bool completed = false;

    bool begin(Trigger source = DailyWake) {
        if (phase != Idle) return false;
        trigger = source;
        // Nickel owns the connection and launch of a native manual request.
        phase = source == NativeRequest ? Requested : Connecting;
        fullSuccess = failed = completed = false;
        return true;
    }
    void launched() { phase = Requested; }
    void started() { if (phase == Requested) phase = Running; }
    void success() { if (phase == Running) fullSuccess = true; }
    void failure() { if (phase == Requested || phase == Running || phase == Cancelling) failed = true; }
    void cancel() { failed = true; phase = Cancelling; }
    void finish() {
        completed = phase == Running && fullSuccess && !failed;
        phase = Draining;
    }
    void reset() { phase = Idle; }
    bool active() const { return phase != Idle; }
    static bool due(const std::string &today, const std::string &lastSuccess, bool force) {
        return force || today != lastSuccess;
    }
};
