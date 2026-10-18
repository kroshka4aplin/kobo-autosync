# Changelog

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
