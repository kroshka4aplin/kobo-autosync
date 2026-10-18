// SPDX-License-Identifier: MIT
#include "../src/policy.h"
#include <cassert>
#include <iostream>
int main() {
    assert(SyncPolicy::due("2026-10-18", "", false));
    assert(!SyncPolicy::due("2026-10-18", "2026-10-18", false));
    assert(SyncPolicy::due("2026-10-19", "2026-10-18", false));
    assert(SyncPolicy::due("2026-10-18", "2026-10-18", true));
    SyncPolicy p;
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
    std::cout << "policy: native-start confirmation, success, failure, cancellation, connection-only, stale success passed\n";
    std::cout << "manual: native button/menu requests, busy guard, native-start requirement, cancel/failure passed\n";
}
