# Changelog

## 0.1.6-experimental — 2026-10-30

- Package licenses without a document extension and instructions as INSTALL.md;
  migrate the old module-owned LICENSES.txt too.
- Use status.log and last-success.dat so diagnostic/state files do not have
  book extensions. Migrate legacy .txt files automatically outside USB export,
  even while paused, preserving the newer valid success date.
- Keep the old success date usable if a move fails; do not reset the daily quota.
- Guard status-file writes during USB export, including bootstrap diagnostics.
- Do not edit Nickel's library database. Existing imported entries may remain
  until a native rescan or removal through the normal reader UI.

Validation: host policy and storage regressions (USB exclusion, repeated migration,
conflicting dates, malformed state and failed writes/moves), clean ARM build and
minimal installer packaging.
Native ABI bindings are unchanged. Hardware migration/rescan testing is pending.

## 0.1.5-experimental — 2026-10-30

- Confirm a started full-sync attempt using an advanced native LastSyncTime as a
  read-only fallback when the PLT hook is missed; allow confirmation during bounded
  cleanup. Old timestamps, completion alone and server responses do not count.
- Close pre-existing Wi-Fi after automatic attempts, at the owner's request.
  Explicit manual sync, pause and download takeover still retain the network.
  No idle network guard is restored.
- Keep updates paused and the firmware-number gate disabled.
- Provide a minimal installer ZIP: installation folders, required licenses and a
  short bilingual INSTALL.txt. Keep developer documentation and build metadata
  out of the installer; publish one downloadable installer asset.

Validation: host regressions, clean ARM build, packaging and ABI inspection for
4.38.23828 and 4.38.23852. The owner reported 0.1.5 working on Forma 4.38.23852; comprehensive hardware testing remains incomplete. On 0.1.4,
Forma 4.38.23852 logged UNCONFIRMED while its native full-sync timestamp matched
the completion time; this is diagnostic evidence, not a test of the fix.

## 0.1.4-experimental — 2026-10-30

- Remove the exact firmware-number startup gate at the owner's explicit request.
  Keep the Forma 377/380 model restriction and required native ABI bindings.
- Keep manual Wi-Fi/download handoff, USB handling, bounded timers, full-sync
  confirmation and paused installation/update behavior unchanged.
- Name the installer without a firmware number; retain 4.38.23828 as the ABI
  reference. This build and Forma firmware 4.38.23852 are not yet device-verified.

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
