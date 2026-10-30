# Instructions for coding agents

## Scope and status

This is a personal experimental mod for Kobo Forma 377/380, with firmware
4.38.23828 as the tested reference. On 2026-10-30 the owner explicitly requested
removal of the firmware-number gate for v0.1.4. Preserve that decision; do not
restore exact firmware-number blocking. The Forma model restriction remains.
Do not claim compatibility with another firmware without ABI evidence and a
recorded device test. Keep claims consistent with `verification.json`:
v0.1.1 logged a confirmed full sync and same-day skip on Forma; Wi-Fi shutdown
was confirmed by the user. v0.1.2 manual permission and Wi-Fi shutdown were tested
on Forma, but that attempt lacked the full-success timestamp and was logged
UNCONFIRMED/FAILED despite no UI error. Keep this success-accounting limitation explicit.
On 2026-10-19, v0.1.2 logs showed confirmed manual full syncs, but the user reported
that immediate Wi-Fi shutdown blocked separate book downloads. v0.1.3 has partial
Forma validation recorded in verification.json: native download-start notifications,
network release and a full-sync timestamp were observed. Working book downloads
were reported after a separate server-port fix during paused diagnostics; do not
claim automatic-attempt download takeover or offline opening was verified. It removes the idle guard and releases
network ownership for manual Wi-Fi/sync, pre-existing connections and native book
download starts. Do not restore strict idle blocking or infer completed book files
from a full sync timestamp. Keep manual network timeout handling non-destructive.
Never describe a build or ABI check
as a successful hardware test, or claim measured battery savings without measurements.

## Layout

- `src/daily.cc`, `src/daily.h`: Qt controller, native ABI bindings and NickelHook hooks.
- `src/policy.h`: pure state machine; `tests/policy_test.cc`: host regression scenarios.
- `tests/verify_firmware.py`: read-only ELF/Qt ABI inspection; requires external firmware.
- `resources/`: default configuration, paused/install markers and optional NickelMenu file.
- `vendor/NickelHook/`: pinned upstream source; retain its MIT license and notices.
- `package.py`: allowlisted installer/source archives and checksums in `dist/`.
- `docs/`: installation and build instructions. `VERSION`: packaged build version.

## Constraints

Use C++11 and Qt 5.2.1 APIs compatible with NickelTC. Preserve event-driven idle
behavior, bounded active timers, USB export handling, cancellation and Wi-Fi cleanup.
Never write `/mnt/onboard` while it is exported over USB. Do not alter account
configuration, book databases, bootloader, kernel or system startup scripts.
Do not infer success from a native call returning, connection alone, or an incidental
book sync. Preserve explicit native-start and full-sync success confirmation.
Version 0.1.5 additionally observes advancement of the native LastSyncTime via
read-only preferences; require a native start and a fresh timestamp from this
attempt, never stale settings or server-only success. Automatic attempts may now
close pre-existing Wi-Fi at the owner's request, but explicit manual sync, pause
and download takeover remain protected until sleep. Keep this distinction tested.
Keep new installs and updates paused. Do not overwrite other NickelMenu files.

Do not commit reader backups, firmware, private logs, credentials, server URLs,
book databases or build toolchains. Keep packaging allowlisted. Preserve dependency
copyright/license text in source and binary distributions. Avoid unrelated vendor edits.

## Validation

Follow `docs/BUILD.md`. After code or packaging changes run host policy tests,
clean ARM build and `python3 package.py`. After ABI changes also run the firmware
verifier against the exact reference binary; record what was and was not checked.
Do not weaken tests just to pass a build. Update English and Russian user instructions
when behavior changes. Keep `VERSION`, installed marker, runtime version message and
changelog consistent. Test changes that affect sleep, USB, cancellation or success
accounting with meaningful regressions. Do not install to a connected reader or
publish a release unless the user's task authorizes it.
