// SPDX-License-Identifier: MIT
#pragma once

// Storage supplies read/write/move/remove/exists and timestamp parsing. Keeping
// the migration separate lets host tests cover USB exclusion and failed writes.
template<class Storage>
void migrateLegacyState(Storage &s, bool allowWrites) {
    if (!allowWrites) return;
    const char *oldSuccess = "last-success.txt", *newSuccess = "last-success.dat";
    if (s.exists(oldSuccess)) {
        if (!s.exists(newSuccess)) s.move(oldSuccess, newSuccess);
        else {
            const auto oldValue = s.read(oldSuccess);
            const auto oldDate = s.timestamp(oldValue);
            const auto newDate = s.timestamp(s.read(newSuccess));
            if (oldDate >= 0 && (newDate < 0 || oldDate > newDate)) {
                if (s.write(newSuccess, oldValue)) s.remove(oldSuccess);
            } else if (newDate >= 0) s.remove(oldSuccess);
            else s.move(oldSuccess, "last-success-legacy.dat");
        }
    }
    const char *oldNames[] = {"status.txt", "LICENSES.txt"};
    const char *newNames[] = {"status.log", "LICENSES"};
    for (unsigned i = 0; i < sizeof(oldNames) / sizeof(*oldNames); ++i) {
        if (!s.exists(oldNames[i])) continue;
        if (s.exists(newNames[i])) s.remove(oldNames[i]);
        else s.move(oldNames[i], newNames[i]);
    }
}
