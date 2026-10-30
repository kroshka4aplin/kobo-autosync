// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <cstdint>

// Daily attempts may close an already enabled radio, but explicit sync/download
// activity hands the network back to Nickel for the rest of this awake session.
struct WifiPolicy {
    bool manualSession = false;
    bool owned = false;
    bool explicitSession = false;
    void begin(bool nativeManual, bool alreadyEnabled, bool daily = false) {
        if (nativeManual) allowManual();
        if (daily && !explicitSession) { manualSession = false; owned = true; }
        else owned = !manualSession && !alreadyEnabled;
    }
    void allowExisting() { manualSession = true; owned = false; }
    void allowManual() { explicitSession = true; allowExisting(); }
    bool closeAfterAttempt() const { return owned && !manualSession; }
    bool cancelNativeOnTimeout(bool awake) const { return !awake || closeAfterAttempt(); }
    void finished() { owned = false; }
    void reset() { manualSession = owned = explicitSession = false; }
};

// No Qt or device dependencies: the same policy is exercised by host tests.
struct SyncPolicy {
    enum Phase { Idle, Connecting, Requested, Running, Draining, Cancelling };
    enum Trigger { DailyWake, MenuRequest, NativeRequest };
    Phase phase = Idle;
    Trigger trigger = DailyWake;
    bool fullSuccess = false;
    bool failed = false;
    bool completed = false;
    bool nativeStarted = false;

    bool begin(Trigger source = DailyWake) {
        if (phase != Idle) return false;
        trigger = source;
        // Nickel owns the connection and launch of a native manual request.
        phase = source == NativeRequest ? Requested : Connecting;
        fullSuccess = failed = completed = false;
        nativeStarted = false;
        return true;
    }
    void launched() { phase = Requested; }
    void started() { if (phase == Requested) { nativeStarted = true; phase = Running; } }
    void success() {
        if (nativeStarted && (phase == Running || phase == Draining)) {
            fullSuccess = true;
            if (phase == Draining) completed = !failed;
        }
    }
    void failure() { if (phase == Requested || phase == Running || phase == Cancelling) failed = true; }
    void cancel() { failed = true; phase = Cancelling; }
    void finish() {
        completed = phase == Running && fullSuccess && !failed;
        phase = Draining;
    }
    void reset() { phase = Idle; }
    bool active() const { return phase != Idle; }
    static bool freshFullStamp(std::int64_t baseline, std::int64_t stamp, std::int64_t requested) {
        return requested > 0 && stamp >= requested && stamp > baseline;
    }
    static bool due(const std::string &today, const std::string &lastSuccess, bool force) {
        return force || today != lastSuccess;
    }
};
