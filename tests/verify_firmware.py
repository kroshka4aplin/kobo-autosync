#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Inspect an official libnickel ELF without loading or executing it."""
import re
import struct
import subprocess
import sys
from pathlib import Path

binary = Path(sys.argv[1])
source = Path(__file__).resolve().parents[1] / 'src/daily.cc'
raw = binary.read_bytes()
assert raw[:6] == b'\x7fELF\x01\x01', 'Expected a 32-bit little-endian ELF'
elf = struct.unpack_from('<16sHHIIIIIHHHHHH', raw)
segments = [struct.unpack_from('<IIIIIIII', raw, elf[5] + i * elf[9]) for i in range(elf[10])]

def file_offset(address):
    for kind, offset, virtual, _, size, *_ in segments:
        if kind == 1 and virtual <= address < virtual + size:
            return offset + address - virtual
    raise ValueError(f'Unmapped ELF address: {address:x}')

def uints(address, count):
    return struct.unpack_from('<' + 'I' * count, raw, file_offset(address))

def qt_string(base, index):
    header = base + 16 * index
    relative = struct.unpack_from('<i', raw, file_offset(header + 12))[0]
    start = file_offset(header + relative)
    return raw[start:raw.index(0, start)].decode()

nm = subprocess.check_output(['nm', '-D', '--defined-only', str(binary)], text=True)
symbols = {parts[2]: int(parts[0], 16) for line in nm.splitlines() if len(parts := line.split()) == 3}
required = re.findall(r'(?:\.name|\.sym) = "(_Z[^"]+)"', source.read_text())
for symbol in required:
    assert symbol in symbols, f'Missing required symbol: {symbol}'
print(f'PASS: {len(required)} native function/hook symbols')
relocations = subprocess.check_output(['readelf', '-rW', str(binary)], text=True).splitlines()
hooks = re.findall(r'\.sym = "(_Z[^"]+)"', source.read_text())
for symbol in hooks:
    assert any('R_ARM_JUMP_SLOT' in line and symbol in line.split() for line in relocations), f'No hookable PLT relocation: {symbol}'
print(f'PASS: {len(hooks)} hook targets have ARM PLT relocations')

expected = {
    'N3PowerWorkflowManager': {'resumedFromSleep', 'aboutToSleep'},
    'SyncManagerBase': {'finished', 'cancelStarted', 'syncStarted'},
    'WirelessManager': {'wifiEnabled', 'networkFailedToConnect'},
    'PlugWorkflowManager': {'aboutToConnect', 'aboutToUnplug', 'doneProcessing'},
}
for class_name, required_signals in expected.items():
    meta = symbols[f'_ZN{len(class_name)}{class_name}16staticMetaObjectE']
    _, strings, data, *_ = uints(meta, 5)
    metadata = uints(data, 14)
    signals = set()
    for i in range(metadata[13]):
        name, argc, params, *_ = uints(data + 4 * (metadata[5] + 5 * i), 5)
        signals.add(qt_string(strings, name))
        if class_name == 'SyncManagerBase' and qt_string(strings, name) == 'syncStarted':
            assert argc == 1
            parameter_type = uints(data + 4 * params, 2)[1]
            assert parameter_type & 0x80000000
            assert qt_string(strings, parameter_type & 0x7fffffff) == 'SyncState::SyncType'
    assert required_signals <= signals, f'Missing signals for {class_name}: {required_signals - signals}'
    print(f'PASS: {class_name} signals ({len(required_signals)})')

# The firmware's SyncMenuController::syncNow and N3SyncManager::silentSync
# both dispatch the virtual method at offset 0xf4 in the N3SyncManager vtable.
# Resolve the relocation rather than treating the on-disk zero as a function pointer.
dispatch = symbols['_ZTV13N3SyncManager'] + 8 + 0xf4
slot_line = next(line for line in relocations if line.startswith(f'{dispatch:08x} '))
assert '_ZN13N3SyncManager4syncEv' in slot_line.split(), slot_line
assert '_ZN13N3SyncManager10silentSyncEv' in required
print('PASS: native menu and silentSync dispatch use N3SyncManager::sync, not the authentication-stage startUserSync')
