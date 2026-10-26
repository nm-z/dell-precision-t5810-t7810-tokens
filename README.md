# Dell Precision Tower 5810, BIOS A34: SMBIOS tokens and setup options

Decoded from a Precision Tower 5810 on BIOS A34 (`T5810A34.exe`), on 2026-10-26.

| File | What |
|---|---|
| `tokens-decoded.tsv` | the 232 Dell SMBIOS (DA) tokens this BIOS lists: libsmbios name, state on the dumped machine, the value the BIOS writes, the Dell setting ID, the other tokens on that setting, and the evidence for the name |
| `DellSmbDaTokensConfig-records.tsv` | the BIOS's whole DA token table (954 records): token, value, Dell setting ID |
| `DellIntelMap.tsv` | the `Setup` variable's field names, offsets and sizes (the `DellIntelMap` EFI variable, EFI-compressed) |
| `setup-options.tsv` | every setup-form question (5,036): form, variable, offset, size, prompt, options with defaults |
| `ifrq.awk` | the gawk that turns an IFRExtractor-RS text dump into `setup-options.tsv` |

## How

- **Token list and names.** `smbios-token-ctl --dump-tokens` (libsmbios 2.4.3) names 210 of the 232 tokens; 22 are "unknown".
- **Token table.** The DXE module `DellSmbDaTokensConfig` holds the DA token table as 10-byte records
  `[token u16][value u16][Dell setting ID u16][0 u32]`. A token writes its value to its Dell setting; tokens that share
  a setting ID are one setting's choices.
- **What a setting ID is not.** The setting ID is not an offset into `Setup` or `IntelSetup`. No module carries a
  setting-ID map as data (the config module's only string is `Setup`), so the SMM token handler resolves it in code.
- **Setup options.** Extracted with UEFIExtract NE A76 and IFRExtractor-RS 1.6.1 from the A34 system BIOS
  (unpacked with BIOSUtilities 25.7.1 `DellPfsExtract`). `Setup` and `IntelSetup` share the GUID
  `EC87D643-EBA4-4BB5-A1E5-3F3E36B20DA9`; neither is visible to the OS.

## Tokens named here

| Token | Name | Evidence |
|---|---|---|
| 0x0487 | Master Password Lockout (Enabled) | activating it changed `Setup` 0x02D6 ("Enable Master Password Lockout") from 0 to 1 in the flash NVRAM |
| 0x0488 | Master Password Lockout (Disabled) | the same setting (0x0733), value 0. The firmware does not take it from the OS: activating it is a no-op, so Lockout can be turned on but not off from the OS |

## Still unnamed

| Tokens | Setting ID | Why they stay unnamed |
|---|---|---|
| 0xF100-0xF135 (12) and 0xF600-0xF604 (4) | 0x0E1C, value 1, shared by the whole 0xF1xx/0xF6xx family in the table | Code-handled. Not activated, since a code-handled token can be an action |
| 0x0498 | 0x06DA, value 0xFFFF | No token restores the current value, so activating one can't be undone from the OS. Not activated |
| 0x04A7 / 0x04A8 / 0x04A9 | 0x0E38, values 3 / 1 / 2 | Same reason. Not activated |

## PCIe link options this BIOS has but no token reaches

- `IntelSetup`, per root port:
  - "PCI-E Port" (0x072D-0x0736)
  - "Link Speed" (0x0808-0x0812): Auto, Gen 1, Gen 2, Gen 3
  - "Override Max Link Width": Auto, x1-x16
  - Port 3A (D03F0) is the CPU's third x16 port.
- `Setup`: Link Training Retry (0x38), Link Training Timeout (0x39), Unpopulated Links (0x3B), Target Link Speed (0x45),
  "Pcie LinkSpeed" (0x5F: 0 Gen3, 1 Gen1, 2 Gen2, 3 Auto).

🤖 Generated with [Claude Code](https://claude.com/claude-code)
