# Third-party notices

## Included: NickelHook

- Upstream: https://github.com/pgaskin/NickelHook
- Vendored commit: `bcd90ba94d90d22562fb64ef2920ed3323c1dae5`
- Copyright (c) 2020–2025 Patrick Gaskin
- License: MIT; complete original text: [vendor/NickelHook/LICENSE](vendor/NickelHook/LICENSE).

NickelHook source is included under `vendor/NickelHook` and compiled into the plugin.
Its complete license is also included in `.adds/daily-sync/LICENSES.txt` in each installer. Generated
objects, MOC output and plugin metadata are build products, not upstream modifications.
The top-level MIT license does not replace this upstream copyright notice.

## References and external dependencies

[NickelMenu](https://github.com/pgaskin/NickelMenu) was consulted as a reference for
native Wi-Fi controls. NickelMenu itself is not bundled; `resources/nickelmenu` is
this project's optional configuration file for an existing installation.

[NickelTC](https://github.com/pgaskin/NickelTC) supplies the cross-compiler, headers
and build-time dependencies. The build image and toolchain are not redistributed
in this repository or installer. Their own licenses continue to apply.

The plugin dynamically uses Qt 5.2.1 and system/native libraries already present
on the reader. Qt, glibc, Kobo's proprietary `libnickel` and the Kobo firmware are
not bundled. Their respective licenses and ownership are unchanged. Firmware used
for local ABI inspection must be obtained separately. This project's MIT license
covers project-owned files only and grants no rights in Kobo firmware or trademarks.
