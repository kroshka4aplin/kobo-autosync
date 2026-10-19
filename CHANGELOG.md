# Changelog

## 0.1.3-experimental — 2026-10-19

- Remove the idle Wi-Fi guard, which blocked separate book downloads after a
  successful full sync in 0.1.2. Daily quota checks no longer force Wi-Fi off.
- Close only connections opened by a managed attempt. Preserve pre-existing Wi-Fi
  and stock manual-sync connections until sleep or manual radio shutdown.
- Release the network to Nickel on native `downloadStarted(Volume)`, including
  when downloads start inside an automatic attempt. Continue conservative full-sync
  success accounting; download activity is not itself proof of sync success.
- Stop tracking at a deadline without cancelling user-owned work while awake.
  Pausing an active attempt also hands its network to Nickel. Sleep still ends
  managed network sessions with bounded cancellation; USB export resets tracking.

Validation: host policy regressions, clean ARM build, packaging and exact-firmware
signal/ABI verification. Partial Forma tests observed native download-start/network
release messages and a confirmed full-sync timestamp. The user reported working
downloads during paused diagnostics. Automatic-attempt download takeover, offline opening and complete
sleep/USB/cancellation behavior remain unverified on hardware.

## 0.1.2-experimental — 2026-10-18

- Allow native explicit manual sync via `fireManualSyncEvent`, even after today's
  daily quota, without starting a duplicate sync. Keep bounded cleanup and Wi-Fi-off.
- Allow NickelMenu's explicit sync command while scheduling is paused.
- Keep the strict guard for unrelated background connections.

Validation: host manual/daily policy regressions and exact-firmware ABI checks.
Forma test: native manual request accepted after the daily quota; server received
it, UI completed without an error, Wi-Fi shutdown confirmed by user. No updates
were expected. The plugin logged UNCONFIRMED/FAILED (no full-success timestamp);
success accounting for this attempt remains unresolved.
Subsequent 0.1.2 wakes logged ALREADY_DONE using the earlier confirmed daily success.
On-device configuration checked: automation enabled, stock daily sync disabled,
NickelMenu entries match the release, no pending installer or command markers.

## 0.1.1-experimental — 2026-10-18

- Replace direct `startUserSync()` authentication-stage invocation with native
  `silentSync()` full-sync dispatch.
- Require the native `syncStarted` signal; bound start confirmation to 20 seconds.
- Log request, return, native start and progress separately; improve USB diagnostics.
- Publish source, installation/removal instructions, licenses and build workflow.

Validation: ARM/Qt build, host policy regressions and reference-firmware ABI checks.
Subsequent Forma logs confirmed a full sync and same-day skips; the user confirmed
Wi-Fi shutdown. Native manual sync was blocked by the strict guard (fixed in 0.1.2).

## 0.1.0 — unpublished prototype

- Implement daily first-wake policy, native Wi-Fi control, success tracking,
  cancellation, USB handling and optional NickelMenu commands.
- Initial physical testing on Kobo Forma 4.38.23828 showed the UI remained usable
  and sleep-time cleanup ran, but full sync did not start. Superseded by 0.1.1.
