#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Create allowlisted, paused installer and corresponding source distribution."""
from pathlib import Path
import hashlib
import json
import re
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parent
SOURCE_FILES = (
    'README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'CHANGELOG.md',
    'AGENTS.md', 'CONTRIBUTING.md', 'SECURITY.md', 'VERSION', 'Makefile',
    'package.py', 'verification.json', '.gitignore', '.gitattributes', '.editorconfig',
    'docs/BUILD.md', 'docs/INSTALL.ru.md',
    'src/daily.cc', 'src/daily.h', 'src/policy.h', 'src/storage.h',
    'tests/policy_test.cc', 'tests/storage_test.cc', 'tests/verify_firmware.py',
    'resources/INSTALL.txt', 'resources/config.ini', 'resources/installed', 'resources/paused', 'resources/nickelmenu',
    'vendor/NickelHook/NickelHook.mk', 'vendor/NickelHook/NickelHook.h',
    'vendor/NickelHook/nh.c', 'vendor/NickelHook/nhplugin.h',
    'vendor/NickelHook/README.md', 'vendor/NickelHook/LICENSE', 'vendor/NickelHook/.gitignore',
    '.github/workflows/build.yml', '.github/ISSUE_TEMPLATE/bug_report.md',
    '.github/pull_request_template.md',
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def write_zip(target, entries):
    with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in entries.items():
            archive.writestr(name, data)
    with zipfile.ZipFile(target) as archive:
        require(archive.testzip() is None, 'ZIP integrity check failed')
        require(set(archive.namelist()) == set(entries), 'ZIP file list mismatch')


def main():
    version = (ROOT / 'VERSION').read_text().strip()
    require(re.fullmatch(r'\d+\.\d+\.\d+-experimental', version), 'Expected experimental version')
    tgz = ROOT / 'KoboRoot.tgz'
    expected = {
        './usr/local/Kobo/imageformats/libdailysync.so': (ROOT / 'libdailysync.so').read_bytes(),
        './mnt/onboard/.adds/daily-sync/installed': (ROOT / 'resources/installed').read_bytes(),
        './mnt/onboard/.adds/daily-sync/paused': (ROOT / 'resources/paused').read_bytes(),
    }
    with tarfile.open(tgz) as archive:
        members = archive.getmembers()
        require(len(members) == len(expected), 'Unexpected tar member count')
        require({m.name for m in members} == set(expected), 'Unexpected tar paths')
        require(all(m.isfile() for m in members), 'Only regular files may be installed')
        for name, data in expected.items():
            require(archive.extractfile(name).read() == data, f'Incorrect tar payload: {name}')
    entries = {
        '.kobo/KoboRoot.tgz': tgz.read_bytes(),
        '.adds/daily-sync/config.ini': (ROOT / 'resources/config.ini').read_bytes(),
        '.adds/daily-sync/paused': (ROOT / 'resources/paused').read_bytes(),
        '.adds/nm/daily-sync': (ROOT / 'resources/nickelmenu').read_bytes(),
        'INSTALL.md': (ROOT / 'resources/INSTALL.txt').read_bytes(),
        '.adds/daily-sync/LICENSES': (
            'Kobo AutoSync — MIT license\n\n' + (ROOT / 'LICENSE').read_text() +
            '\nNickelHook — https://github.com/pgaskin/NickelHook\n' +
            'Vendored commit: bcd90ba94d90d22562fb64ef2920ed3323c1dae5\n\n' +
            (ROOT / 'vendor/NickelHook/LICENSE').read_text()
        ).encode(),
    }
    manifest = {
        'version': version,
        'target': 'Kobo Forma 377/380; no firmware version gate; reference ABI 4.38.23828',
        'initial_state': 'paused',
        'physical_device_test': json.loads((ROOT / 'verification.json').read_text()).get(
            'current_version_device_test', 'end_to_end_sync_not_confirmed_for_this_version'),
        'battery_savings': 'not_measured',
        'packaging_checks': ['exact tar paths and payloads', 'regular files only', 'ZIP integrity'],
        'validation_reference': json.loads((ROOT / 'verification.json').read_text()),
        'sha256': {name: hashlib.sha256(data).hexdigest() for name, data in entries.items()},
    }
    # Detailed verification stays with source/build outputs, not on the reader.
    build_info = (json.dumps(manifest, indent=2) + '\n').encode()
    dist = ROOT / 'dist'
    dist.mkdir(exist_ok=True)
    (dist / 'build-info.json').write_bytes(build_info)
    installer = dist / f'kobo-autosync-forma-v{version}.zip'
    source = dist / f'kobo-autosync-source-v{version}.zip'
    write_zip(installer, entries)
    write_zip(source, {f'kobo-autosync/{name}': (ROOT / name).read_bytes() for name in SOURCE_FILES})
    (dist / 'SHA256SUMS').write_text(''.join(
        f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in (installer, source)))
    print(f'Installer: {installer.name} ({installer.stat().st_size} bytes)')
    print(f'Source: {source.name} ({source.stat().st_size} bytes)')
    print('Verified exact payloads, paused marker, licenses and ZIP integrity; SHA256SUMS written.')


if __name__ == '__main__':
    main()
