# Secure Bootloader Extension: Design Plan

Status: draft, not implemented. Nothing in this document is in the code yet.

## 1. Goals and threat model

The bootloader today trusts anything that can reach the DARTT register map. This extension makes the bootloader safe to expose on a sniffable, injectable bus (RS485, UART, CAN, UDP).

**Attacker model.** Full read/inject/replay/reorder access to the bus. Can observe all traffic and capture valid frames. Can power-cycle the target. May own one other node on the bus.

**Goals**

1. **Authenticity of code.** Only firmware signed by the vendor ever executes, regardless of how the bytes reached flash.
2. **Authorization of commands.** Only a holder of the device key can read, write, erase, save settings or start the application.
3. **Confidentiality (obfuscation) of the traffic and the image in transit.**
4. **Replay resistance across power cycles** with no assumption that a counter survives a reboot.

**Out of scope**

- Denial of service by a bus attacker. Anyone on the bus can jam it, flood it, or overwrite a half-written buffer. The design only ensures this stays a nuisance and never becomes code execution or data loss that is not recoverable.
- Physical attacks (SWD, voltage glitching, decapping). RDP and similar mitigations are noted in section 9 but not designed here.
- Side channels.

## 2. Why the current design needs this

Findings from reading `shared/dartt_bl.c` and the target stubs:

- The entire `dartt_bl_t` struct is the DARTT memory alias (`bootloader_alias` in each target's `dartt_bl_stubs.c`). Every field is readable and writable by any bus peer, including `action_flag`, `erase_page`, `erase_num_pages`, `working_size`, `attr` and `fds`. "Read-only at runtime" comments are convention only.
- `handle_comms` reads `pbl->fds.module_number` live, so a bus peer can change the node address in RAM without `SAVE_SETTINGS`.
- `SAVE_SETTINGS` persists `pbl->fds` as it sits in the mapped struct, so an attacker-written `fds` can be committed to flash.
- `dartt_bl_read_mem` checks only the upper bound (`flash_end`), not `application_start_addr__`, so the bootloader region is readable. A key stored there would be readable over the bus.
- The startup integrity check is `application_crc32`, which is integrity, not authenticity.
- `dartt_bl_init` sets `action_flag = START_APPLICATION` itself for autolaunch and dispatches through the same switch as bus-originated actions.

## 3. Design overview

Two independent layers.

| Layer | Provides | Needs on device |
|---|---|---|
| **A. Image signature (secure boot)** | Only signed firmware runs. Makes the transport's authenticity requirement moot. | Public key only. No secret. |
| **B. Sealed command channel** | Command authorization, confidentiality, replay protection. | Per-device secret key `K_dev`, entropy or persistent counter for `R`. |

Layer A is valuable on its own and is built first. With A in place, the worst a bus attacker can do through a mistake in B is wipe or corrupt the app (DoS), never run unsigned code. The bootloader region is already protected from write and erase, so the device stays recoverable.

## 4. Layer A: image signature

### 4.1 Manifest

```
manifest = { magic, format_version, image_version, image_size, hash[32] }   // hash = SHA-256 or BLAKE2b of the image bytes
signature = Ed25519(vendor_private_key, manifest)                            // 64 bytes
```

The vendor signs the manifest (a short digest), not the raw image. The bootloader holds only the Ed25519 public key as a compile-time constant. The public key is not secret and can be fleet-wide.

### 4.2 Verification flow

The image is written to flash first, unverified, through the normal write path. Nothing in the app region executes until it verifies.

1. Read the manifest and signature from their reserved location.
2. Check `magic`, `image_size` fits the app region, and `image_version >= min_version` (anti-rollback).
3. Hash `[application_start_addr__, + image_size)` directly out of flash in small chunks (constant RAM).
4. Compare the hash to `manifest.hash`.
5. Verify the Ed25519 signature over the manifest.
6. Jump only if all checks pass. Otherwise stay in the bootloader and report `DARTT_BL_IMAGE_INVALID`.

This is the same loop as `dartt_bl_get_crc32`, with a cryptographic hash.

### 4.3 Rules

- **Verify on every boot and before every jump out of DFU.** Do not cache a "verified" flag in settings, because settings can be corrupted by other paths.
- **Verify what is in flash, not what was sent.** It catches flash write errors as well as tampering.
- **Anti-rollback.** Keep the highest accepted `image_version` in persistent storage and refuse older signed images. This needs a field in persistent storage (see open decisions).
- **Single app slot.** An interrupted or invalid update leaves no valid app, as with the current CRC flow. An A/B layout would keep the old image alive until the new one verifies, but it is out of scope here.

## 5. Layer B: sealed command channel

### 5.1 Primitives

- AEAD, for example XChaCha20-Poly1305 via Monocypher (24-byte nonce, so the counter is padded into it). Verify the exact API against the vendored version.
- KDF for key derivation (HKDF-SHA256, or keyed BLAKE2b).
- Each message is `[counter][ciphertext][tag]`. The counter is the nonce, sent in the clear and authenticated. The tag is 16 bytes.

### 5.2 Key hierarchy and sessions

```
K_root                      host/factory only, never on a device
K_dev   = KDF(K_root, "dev"  || UID)           per-unit, provisioned at factory
R       = fresh value generated at each DFU entry (16 bytes, public)
K_h2d   = KDF(K_dev, "h2d"   || UID || R)      host -> device session key
K_d2h   = KDF(K_dev, "d2h"   || UID || R)      device -> host session key
```

- **Replay across reboots.** A new `R` gives new session keys. Anything recorded in an earlier session fails the tag under the new key. This also fixes nonce reuse after the in-RAM counter restarts.
- **`R` is public.** It is returned in the clear in a read-only field. It is not a secret and not an encryption input.
- **`R` source.** Preferred: a hardware TRNG. Several target parts may not have one (verify G0/C0/G431 specifically). Fallback: a boot counter in persistent storage, incremented only on DFU entry (not on autolaunch, so flash wear is negligible), combined with the UID. Either way the host confirms the session (5.5) before sending anything dangerous.
- Separate keys per direction avoid any host/device nonce collision.

### 5.3 Counter rules

- Start of session: `last_accepted = 0`, first valid counter is 1.
- Accept iff `counter > last_accepted`.
- Update `last_accepted` **only after the tag verifies.**
- Refuse and end the session at `0xFFFFFFFF` rather than wrapping.
- Responses use the request's counter under `K_d2h`, so each response nonce is unique because each request counter is accepted once.

### 5.4 Wire format: the working buffer

```
 0..3        counter (LE)
 4..4+n-1    ciphertext          n = working_size - 20
 last 16     tag
AAD = counter || protocol_version
```

`working_size` (a clear register) is used only to locate the tag. Tampering with it moves the tag position or the ciphertext length and makes the tag fail. A fixed-size frame (always the full buffer, zero-padded inside the plaintext) is an alternative that removes this dependence at the cost of more bytes per command.

**Plaintext (the command is sealed, not just the data):**

```
op(1) | data_len(1) | p0(4) | p1(4) | data[data_len]
```

The device **executes only from the decrypted header** and ignores the clear `action_flag`, `erase_*` and `working_size` for protected ops. This is the central rule. Without it the tag proves only that someone with the key sent counter N, and a valid sealed buffer could be reused as a ticket for any attacker-chosen action by writing the clear registers.

**Do not use random filler** for commands with no data. There is nothing to authenticate. Every command has a real header and the data part is just empty.

### 5.5 Ops

| Op | p0 | p1 | data | Notes |
|---|---|---|---|---|
| `OP_HELLO` | | | | Key confirmation. A verifying sealed response proves the device holds the same key and `R`. |
| `OP_READ` | address | length | | Response is sealed. |
| `OP_WRITE` | address | | firmware bytes | Address is in every write, not stateful. Replaces `SET_WORKING_ADDR` for secure use. |
| `OP_ERASE` | page | num_pages | | |
| `OP_VERIFY_IMAGE` | | | | Runs the Layer A check and reports the result. |
| `OP_SAVE_SETTINGS` | | | new settings fields | Persists from the sealed payload and a private copy, never from the mapped `pbl->fds`. |
| `OP_GET_INFO` | | | | Version hash, app start, page size, etc. |
| `OP_START_APP` | | | | Runs the Layer A check, then jumps. |

Seal everything bus-originated, including reads and queries. Do not allow unsealed read requests: they would let an attacker make the device emit unlimited responses.

### 5.6 Responses

Plaintext is `{status(4), data_len(1), data}`, sealed under `K_d2h` with the request counter as nonce, placed in the working buffer. The host reads the working buffer and verifies it. The clear `action_status` register stays as an untrusted convenience. A forged `OK` there means nothing. Authenticated results come from the sealed status.

On a failed open, the device sets `action_status = DARTT_BL_AUTH_FAILED`, produces no sealed response, and changes nothing.

## 6. Device state and mapped-struct changes

### 6.1 Protected state

Never in the mapped struct (file-static in `dartt_bl.c`, like `working_target_ptr_`):

- `K_dev`, `K_h2d`, `K_d2h`
- `last_accepted` counter
- the authoritative `R`
- private copies of every protected configuration value: `module_number`, `boot_mode`, `application_size`, `min_version`, `attr`

The mapped struct holds display copies only. A reload step runs unconditionally after every `handle_comms` pass and overwrites every protected field from its private copy. The comms layer handles one frame per pass, so a bus write to a protected field is overwritten before the next message is processed. Nothing reads protected values back from the struct.

Target stubs must use accessors for protected values. In particular `dartt_bl_handle_comms` must read the node address from the private copy, not `pbl->fds.module_number`.

### 6.2 New fields, actions and codes

- `uint8_t session_r[16]` in `dartt_bl_t`, display only.
- UID readable (public). The host needs it for key derivation.
- New action `SECURE_EXEC = 12`: opens the working buffer and dispatches the inner op.
- New status codes:
  - `DARTT_BL_AUTH_FAILED = -17`
  - `DARTT_BL_SECURE_REQUIRED = -18` (a legacy `action_flag` was written in a secure build)
  - `DARTT_BL_IMAGE_INVALID = -19`
- A single `AUTH_FAILED` for both bad tag and bad counter. Both conditions are public and splitting them leaks nothing useful, but the choice is open (section 12).

### 6.3 Event handler

- In a secure build, any bus-originated `action_flag` other than `SECURE_EXEC` returns `DARTT_BL_SECURE_REQUIRED` without effect.
- **Autolaunch is exempt.** `dartt_bl_init` currently sets `action_flag` itself. Move autolaunch into a dedicated local path that runs the Layer A check and jumps. The bus gate then cannot block it.
- Open the sealed buffer into a private scratch copy and execute from that copy. The event loop is single-threaded and the buffer only changes inside `handle_comms`, so there is no race today. The copy is cheap insurance if comms ever becomes interrupt- or DMA-driven.
- Fail closed: a failed open leaves the counter, working state, and settings untouched.

### 6.4 Read bounds

- `dartt_bl_read_mem` and `OP_READ` deny any range overlapping the bootloader region or the key page. Fix the current missing `app_start` check regardless of the secure extension.
- Keep the key page separate from the settings page. `SAVE_SETTINGS` erases the settings page.

### 6.5 Build option

A `DARTT_BL_SECURE` compile option. The legacy unauthenticated bootloader remains buildable for development and bring-up. A secure build removes the legacy dispatch cases.

## 7. Target stubs to add

Declared weak in `dartt_bl_stubs.h`, defined per target:

- `dartt_bl_get_uid()`
- `dartt_bl_get_entropy()`: returns failure if the part has no TRNG, falling back to the boot-counter scheme
- `dartt_bl_get_device_key()`: reads the key page
- persistent boot-counter accessor, if the fallback is used

## 8. Budget and throughput

- **Working buffer.** 64 B minus a 4 B counter, a 16 B tag, and a 10 B header leaves 34 B, so 32 B of firmware per `OP_WRITE` (write granularity is 8). That roughly halves DFU throughput. Consider growing the buffer to 128 or 256 B. This changes the register map, so the flashing tool and any struct-size assumptions need updating.
- **Flash.** The bootloader partition is fixed and the application linker scripts depend on it. Crypto (hash, Ed25519 verify, AEAD, KDF) adds code. Measure first. If the partition is too small, growing it breaks every deployed app linker script. This is the first thing to check.
- **Boot time.** One hash pass over the image plus one Ed25519 verify per boot. Measure on each target.

## 9. Key provisioning and storage

- **`K_dev` should be per unit.** If it is baked into the common bootloader binary, every unit shares it, and extraction from one device breaks the fleet. The factory station computes `KDF(K_root, UID)` and programs it into the reserved key page, separate from the common image. `K_root` never touches a device.
- **Fleet-wide key (fallback).** Simpler, but one extraction breaks everything. Accept only knowingly.
- **Protection of the key page.** Software read-bounds (6.4). Set RDP level 1 against SWD readout. RDP does not stop the bootloader from reading its own flash, so it is not a substitute for the software block. Check PCROP and securable-memory options per target part (unverified for the G0/C0/G4 parts used here).
- **Vendor signing key.** The Ed25519 private key lives with the build and release process (ideally an HSM or a protected CI secret). Out of scope here beyond noting it.

## 10. Host and flashing-tool changes

- Secure session in `flashing_tool/`: read `R` and the UID, derive `K_dev` and the session keys, seal and open commands, verify sealed responses, run `OP_HELLO` before anything else.
- Offline signing tool (script under `scripts/`) that builds the manifest and signs it.
- Factory provisioning tool for the key page.
- The tool must not trust the clear `action_status` for anything security relevant.

## 11. Threat checklist

| Attack | Mitigation | Residual |
|---|---|---|
| Replay a recorded command after reboot | New `R`, new session key, old tags fail | None |
| Replay within a session | Strict counter, committed after verify | None |
| Forge or modify a command | AEAD tag over counter, ciphertext, sealed header | None |
| Reuse a valid sealed buffer with a different action | Op and params are inside the sealed payload; clear registers ignored | None |
| Write unsigned firmware | Layer A signature verified from flash at every boot | Bootloader refuses to boot it |
| Roll back to an old signed image | `min_version` | Needs persistent field |
| Extract key over the bus | Key not in mapped struct; read bounds block the key region | Physical access, section 1 |
| Overwrite `R` or other protected fields over the bus | Private copies plus unconditional reload | None |
| Reflect device responses back as requests | Separate direction keys | None |
| Wipe or truncate image, overwrite the buffer, jam the bus | Not prevented | DoS only. Bootloader region is protected, so recoverable by reflashing. |
| Delay a valid message across a boot | New `R` per session; `OP_HELLO` key confirmation | Delay within one session is not prevented |
| MITM substitutes a stale `R` | Host's key confirmation fails | DoS only |
| One compromised unit | Per-unit `K_dev` limits it to that unit | Fleet-wide key would not |

## 12. Open decisions

1. Per-unit keys (factory provisioning) or fleet-wide key.
2. `R` source on each target: TRNG, boot counter, or both.
3. Persistent layout: extend `dartt_bl_persistent_t` (needs a migration strategy, since the layout is documented as stable) or add a new reserved page for `min_version`, boot counter, and the manifest.
4. Whether to enlarge `working_buffer`, and the register-map compatibility consequences.
5. Fixed-size versus variable-length sealed frames.
6. One `AUTH_FAILED` code or separate codes for bad tag and bad counter.
7. Which queries, if any, stay unsealed. The default here is none.
8. Flash budget: whether the crypto fits the current bootloader partition on every target.

## 13. Implementation phases

0. **Feasibility.** Measure code size and boot-time cost of the hash, Ed25519 verify, and AEAD on every target. Confirm TRNG availability. Decide the partition-size question. Nothing else should start before this.
1. **Layer A (secure boot).** Manifest, verification on boot and before jump, anti-rollback, signing tool. Independent value, and it provides the safety net for everything after.
2. **Protected state.** Private copies and the reload step, accessors in target stubs, read-bounds fix, key page, `DARTT_BL_SECURE` option, autolaunch refactor.
3. **Sealed channel.** Sessions, KDF, `SECURE_EXEC`, ops, sealed responses, new status codes.
4. **Host tooling and provisioning.** Flashing tool session, signing and provisioning scripts.
5. **Hardening and tests.** RDP and PCROP where available, fuzzing of the sealed-buffer parser.

## 14. Test plan

Extend the existing Ceedling setup (`project.yml`, `test/test_dartt_bl.c`).

- AEAD and KDF known-answer vectors.
- Tampering with each field (counter, ciphertext, tag, `working_size`) fails the open and changes no state.
- Replay of an accepted counter is rejected. A session restart with a new `R` rejects the old session's traffic.
- A valid sealed buffer combined with a mismatched clear `action_flag` or `erase_*` executes only the sealed op.
- A legacy `action_flag` write in a secure build returns `DARTT_BL_SECURE_REQUIRED`.
- Autolaunch still works in a secure build and still requires a valid image.
- Read bounds: the bootloader region and key page are unreadable via `OP_READ` and legacy reads.
- Protected-field reload: a bus write to `R`, `module_number`, `boot_mode`, or `attr` has no effect after the next pass.
- Image verification: modified image, modified manifest, bad signature, oversize image, and rollback each refuse to boot.
- Counter exhaustion ends the session.
