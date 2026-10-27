# Dell Precision Tower 5810 and 7810, BIOS A34: SMBIOS tokens and setup options

Decoded on 2026-10-26 from a Precision Tower 5810 (`T5810A34.exe`) and a Precision Tower 7810 (`T7810A34.exe`), both on BIOS A34.
Their DA token tables, setting maps, `Setup` field maps and setup forms are identical; only which tokens are present differs
(mostly the sensor slots). Every token both BIOSes list is named:

| Model | Tokens listed | Named by libsmbios | Named here |
|---|---|---|---|
| T5810 | 232 | 210 | 22 |
| T7810 | 236 | 210 | 26 |

The extra names come from the BIOS images and read-only token reads. Upstream: [dell/libsmbios#157](https://github.com/dell/libsmbios/pull/157)
adds them to `doc/token_list.csv`.

| File | What |
|---|---|
| `t5810/tokens-decoded.tsv`, `t7810/tokens-decoded.tsv` | each model's tokens. Columns: name; state on that machine; value written; Dell setting ID; the `Setup` byte, field name and setup prompt the setting lands on; the machine's value there; evidence |
| `setting-to-setup-map.tsv` | the BIOS's Dell setting ID → `Setup` byte table (144 records, from DellPolicyDxe; BindingsPei holds the same table) |
| `DellSmbDaTokensConfig-records.tsv` | the BIOS's whole DA token table: token → value → Dell setting ID (954 records) |
| `DellIntelMap.tsv` | the `Setup` variable's field names, offsets and sizes (the `DellIntelMap` EFI variable, EFI-compressed) |
| `setup-options.tsv` | every setup-form question (5,036): form, variable, offset, size, prompt, options with defaults |
| `ifrq.awk` | the gawk that turns an IFRExtractor-RS text dump into `setup-options.tsv` |
| `tools/daread.c` | read-only token read through the Dell calling interface (class 0 select 0): status, current, min, max. Builds against libsmbios (`smbios_c/smi.h`) |
| `tools/policy-map-scan.c` | the byte scanner that found and decoded the setting map |

## How a token turns into a setting

1. **Discovery.** SMBIOS type 218 (0xDA) structures list the supported tokens and the call: SMI port 0xB2, command 0x37.
2. **The call.** A request buffer with signature `DSCI` carries class, select and arguments. Class 0 reads a token, class 1 writes it.
   The SMM handler `DellSmBiosDaCiSmm` checks the buffer, keeps it out of SMRAM, and routes it by (class, select). It has no
   per-token code.
3. **Token → setting.** `DellSmbDaTokensConfig` holds 10-byte records `[token u16][value u16][Dell setting ID u16][0 u32]`.
   Tokens that share a setting ID are one setting's choices. Value 0xFFFF marks a value token, which takes an arbitrary value.
4. **Setting → storage.** `DellPolicyDxe` (and `BindingsPei`) hold 12-byte records
   `[setting ID u32][0x0010][Setup offset u16][size 1][attr 4]`: each setting is one byte of the AMI `Setup` variable
   (GUID `EC87D643-EBA4-4BB5-A1E5-3F3E36B20DA9`, 0x3F1 bytes). This was checked against 7 named settings and one NVRAM diff.
5. **Exception.** Setting 0x0E1C has no `Setup` byte. It marks tokens answered in code: the 0xF1xx/0xF6xx embedded-controller sensor
   reads, and the power-supply LED blink tokens. The sensor values come from `PwsEcSmm` (Dell Pws EC driver), which also carries
   0x0E1C. That driver shadows the EC registers and holds the probe names `CPU_TEMPERATURE`, `VIDEO_TEMPERATURE` and `CPU_THROTTLE`.
   It takes its thermal configuration from the protocol `DellThermInfoConfigSmm` installs (GUID 4093DA92-61D2-47EC-A0CA-0640433CEDB0).
   The calling-interface handler just fetches the value by ID. Neither binary has a per-field switch, so the field meanings below
   come from live reads.

## The 22 tokens named here

| Token | Name | Evidence |
|---|---|---|
| 0x0487 / 0x0488 | Master Password Lockout, Enabled / Disabled | setting 0x0733 = `Setup` 0x02D6 "Enable Master Password Lockout". Activating 0x0487 changed that byte 0 → 1 in the flash NVRAM. The firmware takes 0x0488 from the OS as a no-op, so the OS can turn it on but not off |
| 0x0498 | UEFI Boot Path Security (value token) | setting 0x06DA = `Setup` 0x02D5: 0 Never, 1 Always, 2 Always Except Internal HDD. A live read gives 2 with a range of 0–2 |
| 0x04A7 / 0x04A8 / 0x04A9 | PCIe LinkSpeed, Auto / Gen1 / Gen2 | setting 0x0E38 = `Setup` 0x005F "Pcie LinkSpeed" (3 Auto, 1 Gen1, 2 Gen2, 0 Gen3). Gen3 has no token; Dell Command \| Configure's `--PcieLinkSpeed` takes Auto, Gen1 and Gen2 only |
| 0xF1s0 (s = 0, 2, 3) | EC temperature sensor s: reading = °C + 127 | live reads 172/159/163 matched `dell_smm` CPU/memory 45/32/36 °C (6 points). Public: HWiNFO forum thread 7482 |
| 0xF1s3 / 0xF1s5 | EC temperature sensor s: two thresholds, likely warning / critical (°C + 127) | CPU 219/229 (92/102 °C), memory 202/212 (75/85 °C) |
| 0xF1s2 | EC temperature sensor s: field 2 | reads 3 on every sensor; meaning unknown |
| 0xF600 | EC fan 0 (processor fan): RPM | live reads 1005/1004/1000 matched `dell_smm` at the same moments. Public: DellFanManagement `Token.cs` Fan1Rpm = 0xF600 |
| 0xF601 / 0xF602 / 0xF604 | EC fan 0: fields 1 / 2 / 4 | read 3 / 96 / 100; meaning unknown |

Fields 1, 4, 6 and 7 of the temperature tokens and fields 3 and 5 of the fan tokens return "not supported". Temperature sensor s
is `dell_smm` temp(s+1) and fan s is `dell_smm` fan(s+1):
- **T5810:** sensors 0 (CPU), 2 and 3 (memory), and fan 0 (Processor Fan).
- **T7810:** sensors 0 and 1 (CPUs), 3 and 4 (memory), and fan 3. F630 read 2547 RPM against `dell_smm` fan4's 2548; F110/F140 read 31/30 °C.

The thresholds read the same on both: 92/102 °C on CPU sensors, 75/85 °C on memory sensors. On the T7810, PCIe LinkSpeed reads Auto (3).

Sources: [HWiNFO forum thread](https://www.hwinfo.com/forum/threads/new-dell-systems-ec-sensor-values-not-available-fix.7482/),
[DellFanManagement Token.cs](https://github.com/AaronKelley/DellFanManagement/blob/master/DellSmbiosSmiLib/DellSmi/Token.cs),
[Dell Command | Configure --PcieLinkSpeed](https://www.dell.com/support/manuals/en-us/command-configure/dcc_5.x_ref_guide/-pcielinkspeed?guid=guid-5bf3f31f-58b2-49ff-9bfa-99f0280e2c0b&lang=en-us).

## PCIe link options this BIOS has

- **`Setup`:**
  - "Pcie LinkSpeed" 0x5F: tokens 0x04A7-0x04A9; Gen3 has no token.
  - Link Training Retry 0x38, Link Training Timeout 0x39, Unpopulated Links 0x3B, Target Link Speed 0x45: no tokens.
- **`IntelSetup`, per root port (no tokens):**
  - "PCI-E Port" 0x072D-0x0736
  - "Link Speed" 0x0808-0x0812: Auto, Gen 1, Gen 2, Gen 3
  - "Override Max Link Width": Auto, x1-x16
  - "Hot Plug Capable"

🤖 Generated with [Claude Code](https://claude.com/claude-code)
