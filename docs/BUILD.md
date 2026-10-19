# Build and verify

Run commands from the repository root on Linux. Host checks require a C++ compiler,
Python 3.8+ and GNU binutils. ARM builds use Docker and the official NickelTC image.
The image is pinned to the digest used for the initial build:

```sh
export NICKELTC_IMAGE=ghcr.io/pgaskin/nickeltc@sha256:3296eb400d270cfd26f5773ab09625d4469f85680b3a191951dfb78d8ade7840
```

## Host regression tests

```sh
g++ -std=c++11 -Wall -Wextra -Werror tests/policy_test.cc -o /tmp/kobo-policy-test
/tmp/kobo-policy-test
```

These cover the date policy and success/failure/cancellation transitions, including
the regression where a native entry returns without confirming that sync started.
They do not simulate Qt/Nickel, physical Wi-Fi, sleep or USB storage.
Network regressions also cover ownership across automatic sync, native manual
sync, pre-existing Wi-Fi, download takeover, non-destructive tracking timeouts and
sleep/USB resets. Firmware inspection checks the `downloadStarted(Volume)` signal;
only a physical download test can confirm the complete content-transfer behavior.

## ARM plugin and installer

Pulling the image requires network access. The build itself runs without networking.
Use a clean build after changing headers or vendored code.

```sh
docker pull "$NICKELTC_IMAGE"
docker run --rm --network none --read-only --cap-drop ALL   --security-opt no-new-privileges --user "$(id -u):$(id -g)"   --tmpfs /tmp --mount "type=bind,src=$PWD,dst=/work" --workdir /work   "$NICKELTC_IMAGE" sh -c 'make clean && make -j2 all && make koboroot'
python3 package.py
```

`dist/` contains the installer ZIP, source ZIP and `SHA256SUMS`. Packaging rejects
unexpected KoboRoot entries, verifies marker/library bytes and supplies project and
NickelHook licenses. Source packaging uses an explicit file allowlist; local device
files, build products and firmware are excluded. The installer starts paused.
Package metadata describes performed checks conservatively; packaging alone does
not prove that policy tests or firmware verification were run.

## Reference firmware ABI check

Obtain the official firmware independently using the URL in `verification.json`.
Extract `libnickel.so.1.0.0` to a directory outside the repository. Do not commit it.
For the reference build its SHA-256 must match `libnickel_sha256` in that file.

```sh
sha256sum /path/to/libnickel.so.1.0.0
python3 tests/verify_firmware.py /path/to/libnickel.so.1.0.0
```

The script inspects ELF symbols, hookable ARM PLT relocations, Qt signal metadata
and the expected full-sync vtable slot without executing firmware code. The comment
about the native menu and silentSync using that slot is based on manual inspection
of this exact reference firmware; the script does not disassemble both callers.
A passing result is ABI evidence, not an end-to-end device test.

## Release checklist

1. Keep `VERSION`, runtime version text, `resources/installed`, changelog and test
   status consistent. Preserve all license notices.
2. Run host tests, a clean ARM build, packaging and the reference ABI check when
   changing native bindings. Inspect the installer manifest and `SHA256SUMS`.
3. Record actual physical tests, if any, without upgrading the status by inference.
4. Create a prerelease tag matching `v$(cat VERSION)`, attach both ZIPs and
   `SHA256SUMS`, and call out remaining hardware limitations in the release notes.

CI performs the host tests, ARM build and packaging. It does not download firmware
or run hardware tests. CI artifacts are experimental build outputs.
