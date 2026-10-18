# Contributing

This is a personal experimental project. Issues and small, focused pull requests
are welcome; there is no promised support schedule. English and Russian are fine.

Before reporting a bug, include the model, exact firmware and plugin version,
steps to reproduce, expected behavior, and relevant lines from the plugin logs.
State whether the problem occurs after wake, sleep or USB connection. Redact
credentials, server addresses and book/account information from any server logs.
Do not upload device backups, databases or firmware.

For code changes, explain the problem and resulting behavior. Follow the C++11 /
Qt 5.2.1 constraints and validation commands in [docs/BUILD.md](docs/BUILD.md).
Add meaningful state-machine regressions for behavior changes. Explicitly distinguish
host tests, ABI checks and physical tests. Support for another firmware/model needs
separate ABI inspection and device evidence, not just an allowlist change.

Contributions to project-owned files are submitted under the project's MIT license.
Preserve upstream license/copyright notices. No contributor agreement is required.
