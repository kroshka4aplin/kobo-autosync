# Kobo AutoSync

[Русская инструкция](docs/INSTALL.ru.md) · [Build from source](docs/BUILD.md) · [Changelog](CHANGELOG.md)

A personal, experimental Nickel mod that requests a full Kobo sync at the first wake
of each local calendar day, then turns Wi-Fi off after the automatic attempt unless a book download or
explicit manual sync has taken over. Previously enabled Wi-Fi is also closed. It uses the sync service already
configured on the reader, including an existing Autocaliweb setup.

**Made for my own Kobo Forma. Development and device testing are limited to Kobo
Forma. The original tested reference is firmware 4.38.23828. Other models are unsupported. Version 0.1.4 removes
the firmware-number gate at the owner's request; that does not establish
compatibility with other firmware. A 0.1.4 test on Forma 4.38.23852 exposed missed success accounting; the owner reports 0.1.5 working on Forma 4.38.23852.**
This is an independent hobby project, with no affiliation with Rakuten Kobo.

## Current status

**0.1.5-experimental fixes missed native success accounting and changes automatic
Wi-Fi cleanup.** In addition to the existing hook, it reads Kobo's native
`LastSyncTime` without changing preferences. A newly advanced timestamp from this
started attempt can confirm success, including during bounded cleanup. A stale
timestamp or server-side success alone is insufficient. After an automatic
attempt, pre-existing Wi-Fi is also closed unless explicit manual sync, a download
or pause has handed the network to Nickel. No idle network guard is added.

On 2026-10-30, 0.1.4 ran on Forma 4.38.23852 and logged `UNCONFIRMED` even though
Kobo's native timestamp matched completion. The radio was retained as pre-existing
Wi-Fi. Both reference and 4.38.23852 ABI inspection pass; **The owner reports 0.1.5 working after installation**; comprehensive hardware
testing remains incomplete. Installs and updates remain paused. The firmware-number gate
removed in 0.1.4 remains disabled; the Forma restriction remains.

**0.1.3-experimental has partial device validation on Forma.** It removes
the idle Wi-Fi guard: manual Wi-Fi and native manual sync remain available until
sleep. A native book-download start also hands network control back to Nickel,
including during an automatic attempt. Tracking timeouts do not cancel user-owned
work while awake. This addresses a reported 0.1.2 regression where sync succeeded
but separate book downloads were blocked by the immediate Wi-Fi shutdown/guard.

On 2026-10-19, Forma logs showed native download-start notifications, network
release and a confirmed full-sync timestamp. The user reported working book
downloads during paused diagnostics. **Downloads taking over an enabled automatic attempt, offline opening
of the downloaded file, and complete sleep/USB/cancellation behavior remain
unverified on hardware.** See `verification.json` for details.

Version 0.1.1 logged a confirmed full sync,
Wi-Fi shutdown and same-day automatic skips on a physical Forma; the user also
confirmed Wi-Fi shutdown. Its strict network guard blocked native manual sync.
Version 0.1.2 was tested with a native manual request after the daily quota: the
server received it, the UI finished without an error, and the user confirmed Wi-Fi
shutdown. There were no expected updates in that test. The plugin still logged
`UNCONFIRMED`/`FAILED` because Kobo did not update its full-success timestamp.
**Manual access and shutdown are confirmed; success accounting for that attempt
remains unresolved.** These messages alone do not establish an exchange failure.
Subsequent wakes on 0.1.2 logged `ALREADY_DONE`, retaining the day's earlier
confirmed success. The on-device configuration was checked: automation enabled,
stock daily sync disabled, no pending installer or pause marker.
Host policy tests, the ARM build and exact-firmware ABI checks also pass.
Battery savings have not been measured. Keep a backup before trying it.

## Intended behavior

- At the first wake (or startup) each day, connect to a saved Wi-Fi network and
  request the reader's native full sync. No scheduled wake from sleep is needed.
- Record success only after native start, full-sync success and completion signals.
  A failed or unconfirmed attempt is retried at the next wake if today's success
  has not already been recorded, with bounded connection/sync timeouts.
- Turn Wi-Fi off after an automatic attempt even if already enabled.
  An explicit stock manual sync, pause, or a native book-download start
  hands the network to Nickel until sleep or manual Wi-Fi shutdown. The plugin
  never turns a manually disabled radio back on just to finish a download.
  **The stock “Sync now” button, book downloads, browser and Wikipedia are allowed.**
  There is no idle Wi-Fi guard. While manually online, Nickel may run background
  syncs; the daily limit applies to AutoSync's own scheduled requests.
- Pause automation while storage is exported over USB. Work only in the stock Kobo
  interface (Nickel); KOReader and Kobalt networking is outside its scope.
- Start every installation and update **paused**, so activation is an explicit step.

One daily sync can produce many HTTP requests and can overlap with connection-time
requests initiated by Kobo. Failed or unconfirmed attempts can run more than once
in a day. This is not a one-request-per-day limiter.

## Install

Requirements: Kobo Forma (model IDs ending in 377 or 380), a saved Wi-Fi network
and an already working native sync configuration. Firmware **4.38.23828** is the
tested reference; 0.1.4 does not block startup based on the firmware number.
NickelMenu is optional; NickelDBus is not required.

1. Download the **installer ZIP** from [Releases](https://github.com/kroshka4aplin/kobo-autosync/releases).
   Choose `kobo-autosync-forma-v0.1.5-experimental.zip`. GitHub's automatic
   “Source code” archives are for developers and cannot be installed.
   The ZIP contains only the installation folders, required licenses and a short
   bilingual `INSTALL.txt`. Detailed documentation and build metadata stay in the repository.
2. Extract it on your computer, showing hidden files. Connect the Forma over USB.
3. Merge the installer's `.adds` directory into the reader's root directory.
   Preserve your existing files, including `.adds/nm/doc` and other menu entries.
4. Copy `.kobo/KoboRoot.tgz` into the reader's existing `.kobo` directory. Finish
   installing any other pending `KoboRoot.tgz` first.
5. Safely eject, disconnect and wait for installation and reboot.
6. Disable Kobo's built-in daily automatic sync in its settings. Then choose
   **Daily Sync - resume** in NickelMenu, or delete only
   `.adds/daily-sync/paused` over USB and safely eject. Sleep and wake the reader.
7. Let the attempt finish. With no downloads/manual takeover, check that Wi-Fi is off and read
   `.adds/daily-sync/daily-sync.log`: expected success is
   `DONE: full sync confirmed; Wi-Fi disabled`, with today's date in `last-success.txt`.
   A `REQUESTING` or `START_RETURNED` line alone does not prove a sync occurred.
   If a download takes over, expect `DOWNLOAD_STARTED` and Wi-Fi left to Nickel.

Full Russian instructions, diagnostics and first-test checklist:
[Установка и использование](docs/INSTALL.ru.md).

## Update, pause and uninstall

**Update:** copy only the new `.kobo/KoboRoot.tgz`, eject and reboot. Existing settings
and logs remain. The update recreates `paused`; resume afterward. Do not overwrite
an existing `config.ini` unless you intend to reset its values.

**Pause:** choose **Daily Sync - pause**, or create an empty
`.adds/daily-sync/paused` file. Manual Wi-Fi is then available. If an attempt is
active, pausing also releases its Wi-Fi to Nickel. Resume by removing `paused`.
**Manual sync:** use the stock Kobo sync button or **Daily Sync - sync now** in
NickelMenu; today's automatic quota does not block an explicit request. A successful
managed manual sync also satisfies today's daily quota. The stock button leaves
Wi-Fi available afterwards for separate book downloads. The plugin does not launch
a second sync when the stock button already owns the request.

The NickelMenu command also works while AutoSync is paused: it runs one bounded
attempt, keeping the scheduler paused. It closes only Wi-Fi it opened and releases
it if a download/manual request takes over. Without
NickelMenu, create `.adds/daily-sync/sync-now` and eject. The stock Kobo button while
paused keeps its normal behavior; the paused plugin does not manage its Wi-Fi.

**Uninstall:** create an empty `.adds/daily-sync/uninstall` file and reboot.
NickelHook removes this plugin and its own menu file is removed. Alternatively,
remove `.adds/daily-sync/installed` and reboot. Afterward you may remove the remaining
`daily-sync` directory and turn Wi-Fi and Kobo's built-in daily sync back on.
No factory reset is part of installation or removal.

## Configuration and logs

Edit `.adds/daily-sync/config.ini` over USB, then safely eject:

```ini
[DailySync]
Enabled=true
WakeDelaySeconds=5
ConnectTimeoutSeconds=60
MaxSyncSeconds=300
```

Accepted ranges: wake delay 2–30 seconds, connection 15–180 seconds, running sync
60–1800 seconds. The plugin also waits up to 20 seconds for sync-start confirmation
and up to 15 seconds for cancellation of its own work. A tracking timeout while the
user owns the network stops monitoring without cancelling Nickel's work or marking
success. Downloads are handed to Nickel until sleep, rather than cut off by that
timeout. Sleep ends the manual network session while AutoSync is enabled; it is
not a promise to keep downloads running during sleep. Set date/time correctly.

`status.txt` shows the latest event. `daily-sync.log` has attempt details and one
rotated backup. `last-success.txt` stores the local date/time of the last confirmed
success. Inspect device logs alongside server logs when testing. USB connection
stops automation timers, so the last line before USB export can be an intermediate state.

## Development

See [BUILD.md](docs/BUILD.md), [CONTRIBUTING.md](CONTRIBUTING.md) and
[AGENTS.md](AGENTS.md). CI runs policy tests, builds the ARM plugin and checks the
installer's contents. Firmware verification is a separate local check against a
lawfully obtained official firmware library; the firmware is not redistributed.
CI success does not establish device compatibility or battery savings.

## License and credits

Project code and documentation: [MIT](LICENSE), copyright 2026 kroshka4aplin.
Vendored [NickelHook](https://github.com/pgaskin/NickelHook): MIT, copyright Patrick
Gaskin; its [original license](vendor/NickelHook/LICENSE) is preserved and included
in installers. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for provenance
and dependency boundaries. Thanks to [NickelMenu](https://github.com/pgaskin/NickelMenu)
for the native Wi-Fi control reference and [NickelTC](https://github.com/pgaskin/NickelTC)
for the build toolchain.
