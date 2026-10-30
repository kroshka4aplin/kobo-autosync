// SPDX-License-Identifier: MIT
#include "../src/policy.h"
#include <cassert>
#include <iostream>
int main() {
    // Fresh settings alone must not revive an old success, even if an earlier
    // date was absent, the clock moved backwards, or the value stayed unchanged.
    assert(SyncPolicy::freshFullStamp(1000, 3000, 2000));
    assert(SyncPolicy::freshFullStamp(0, 3000, 2000));
    assert(!SyncPolicy::freshFullStamp(1000, 1000, 2000));
    assert(!SyncPolicy::freshFullStamp(0, 1000, 2000));
    assert(!SyncPolicy::freshFullStamp(4000, 3000, 2000));
    assert(!SyncPolicy::freshFullStamp(0, 0, 2000));
    assert(!SyncPolicy::freshFullStamp(0, 3000, 0));
    assert(SyncPolicy::due("2026-10-18", "", false));
    assert(!SyncPolicy::due("2026-10-18", "2026-10-18", false));
    assert(SyncPolicy::due("2026-10-19", "2026-10-18", false));
    assert(SyncPolicy::due("2026-10-18", "2026-10-18", true));
    SyncPolicy p;
    // Native settings can become visible after finished(), during bounded
    // cleanup. They count only for an attempt whose native start was observed.
    assert(p.begin()); p.launched(); p.started(); p.finish();
    assert(!p.completed); p.success(); assert(p.completed); p.reset();
    assert(p.begin()); p.launched(); p.finish(); p.success();
    assert(!p.completed); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.failure(); p.finish(); p.success();
    assert(!p.completed); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.cancel(); p.success(); p.finish();
    assert(!p.completed); p.reset();
    assert(p.begin()); assert(!p.begin());
    p.success(); // A background success during Wi-Fi setup is not our full sync.
    p.failure(); // Neither is an unrelated connection-time failure.
    p.launched(); p.finish(); assert(!p.completed); p.reset();
    // Regression: returning from the native entry is not evidence the sync started.
    assert(p.begin()); p.launched(); assert(p.phase == SyncPolicy::Requested);
    p.success(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin()); p.failure(); p.launched(); p.started(); p.success(); p.finish(); assert(p.completed); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.success(); p.finish(); assert(p.completed); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.success(); p.failure(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.cancel(); p.success(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin()); p.cancel(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin()); p.launched(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.success(); p.finish(); p.reset();
    assert(p.begin()); p.launched(); p.started(); p.finish(); assert(!p.completed);
    p.reset();
    // A native button press bypasses today's automatic quota, without causing a
    // second launch by our connection driver. It still needs native start/success.
    assert(!SyncPolicy::due("2026-10-18", "2026-10-18", false));
    assert(p.begin(SyncPolicy::NativeRequest));
    assert(p.phase == SyncPolicy::Requested);
    assert(!p.begin(SyncPolicy::DailyWake));
    assert(p.trigger == SyncPolicy::NativeRequest);
    p.started(); p.success(); p.finish(); assert(p.completed); p.reset();
    assert(p.begin(SyncPolicy::NativeRequest));
    p.success(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin(SyncPolicy::NativeRequest));
    p.started(); p.cancel(); p.success(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin(SyncPolicy::NativeRequest));
    p.started(); p.failure(); p.success(); p.finish(); assert(!p.completed); p.reset();
    assert(p.begin(SyncPolicy::MenuRequest)); assert(p.phase == SyncPolicy::Connecting);
    p.launched(); p.started(); p.success(); p.finish(); assert(p.completed); p.reset();
    assert(p.begin()); assert(p.trigger == SyncPolicy::DailyWake); assert(p.phase == SyncPolicy::Connecting);
    p.reset();
    WifiPolicy net;
    // Daily sync opened an offline radio: completion may turn it off.
    net.begin(false, false);
    assert(net.closeAfterAttempt());
    assert(net.cancelNativeOnTimeout(true));
    assert(p.begin()); p.launched(); p.started(); p.success(); p.finish();
    assert(p.completed && net.closeAfterAttempt());
    net.finished(); p.reset();
    assert(!net.closeAfterAttempt());
    // A native manual sync after today's quota must leave the radio available
    // so a separate book download can start after metadata sync finishes.
    net.begin(true, false);
    assert(p.begin(SyncPolicy::NativeRequest));
    p.started(); p.success(); p.finish();
    assert(p.completed && !net.closeAfterAttempt());
    assert(!net.cancelNativeOnTimeout(true));
    assert(net.cancelNativeOnTimeout(false));
    net.finished(); p.reset();
    net.begin(false, false); // Menu request during the same user session.
    assert(!net.closeAfterAttempt());
    // A download beginning inside an automatic attempt takes over its radio.
    net.reset(); net.begin(false, false);
    assert(net.closeAfterAttempt());
    assert(p.begin()); p.launched(); p.started();
    net.allowManual();
    p.success(); p.finish();
    assert(!net.cancelNativeOnTimeout(true));
    assert(p.completed && !net.closeAfterAttempt());
    net.finished(); p.reset();
    // No success signal still means unconfirmed, even after granting a download.
    assert(p.begin()); p.launched(); p.started(); p.finish();
    assert(!p.completed && !net.closeAfterAttempt());
    p.reset();
    // Already enabled Wi-Fi belongs to Nickel, even without a captured enable
    // event (for example, it was enabled before the plugin initialized).
    net.reset(); net.begin(false, true);
    assert(!net.closeAfterAttempt());
    // A radio enable before the daily attempt no longer grants permanent manual
    // ownership. Daily cleanup must close it even if Nickel connected first.
    net.allowExisting(); net.begin(false, true, true);
    assert(net.closeAfterAttempt());
    net.finished();
    // Explicit native sync/download handoff still wins over the daily policy.
    net.allowManual(); net.begin(false, true, true);
    assert(!net.closeAfterAttempt());
    assert(!net.cancelNativeOnTimeout(true));
    net.reset(); net.begin(false, true, true);
    assert(net.closeAfterAttempt());
    // Sleep/USB resets permission: next daily attempt may own a fresh connection.
    net.allowManual(); net.reset(); net.begin(false, false);
    assert(net.closeAfterAttempt());
    std::cout << "policy: native-start confirmation, success, failure, cancellation, connection-only, stale success passed\n";
    std::cout << "manual: native button/menu requests, busy guard, native-start requirement, cancel/failure passed\n";
    std::cout << "network: automatic ownership, pre-existing Wi-Fi, manual sync, downloads, sleep/USB reset passed\n";
}
