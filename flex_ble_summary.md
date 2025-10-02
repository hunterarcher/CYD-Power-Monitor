# BLE Reverse Engineering – Flex Adventure Fridge

## 1. Core Discoveries

- **BLE Device**: Flex Adventure camping fridge, advertises as `A1-FFFF11C62950`.
- **Primary Service UUID**: `00001234-0000-1000-8000-00805f9b34fb`.
- **Key Characteristics**:
  - **0x1236** – Notify/Read
    - Must enable notifications via CCCD (0x2902 → `01 00`).
    - All telemetry/ack responses arrive here.
  - **0x1235** – Write
    - Used for keepalive and control commands (setpoint, Eco mode, battery protection, etc.).
  - **0xFFF1** – Also listed, but role unclear (likely alt control/debug).

---

## 2. Message Patterns

- **Keepalive frame**

  ```
  120600FEFE03010200
  ```

  Must be sent periodically, otherwise notifications stop (“waiting for notif 2” stall).

- **Setpoint attempts**\
  Payloads of form:

  ```
  120600FEFE0406F102XX
  ```

  Where `XX` seems to encode the setpoint (still mapping exact meaning).

- **Notifications**\
  Come back on 0x1236 after successful write. If you don’t get them:

  - Re-enable notify on 0x1236.
  - Resend keepalive before sending command.

---

## 3. What Worked

- **Enabling notify** on 0x1236 (via CCCD).
- **Keepalive** write to 0x1235 (device responds).
- **Write requests to 0x1235** using **Byte array** format (continuous hex, no spaces, even length).
- **Manually sending frames** in nRF Connect worked once syntax was correct.
- Some macros imported and ran successfully when formatted correctly.

---

## 4. What Failed / Issues

- **nRF XML macros**
  - Import errors: “odd number of characters”, “invalid enum value”, etc.
  - Caused by differences in schema (some app versions require different XML attribute names).
- **Running multiple macros at once** → not possible in nRF. You must chain actions inside one macro, or run them manually.
- **Stalls** (“waiting for notification 2”) when keepalive wasn’t sent before a command.
- **Changing setpoint** didn’t visibly update fridge yet – payload likely not fully correct. More testing required.

---

## 5. Critical Lessons / Things to Watch

- Always **enable notifications** first (0x1236 CCCD).
- Always **send keepalive** (`120600FEFE03010200`) before any control command.
- **Use “Byte array” format** in nRF write dialog:
  - Continuous hex string, no `0x`, no spaces.
  - Must have even number of characters.
- If no data comes back → re-enable notify and resend keepalive.
- Don’t fight XML import – easier to build macros inside the app UI (New Macro → Add actions → Save).
- Logs: only handle 0x0003 / 0x1236 notifications are relevant.
- Device disconnects (Error 0x8, GATT timeout) are common – re-connect and repeat.

---

## 6. Practical Workflow (Tap-Only, No XML)

1. Connect to fridge.
2. Enable notifications on 0x1236.
3. Write keepalive to 0x1235.
4. Write command to 0x1235 (e.g. setpoint).
5. Watch notifications on 0x1236.
6. If stalled → re-enable notify, resend keepalive, retry.

---

## 7. Next Steps

- Map exact **setpoint encoding** (logs suggest last byte varies with °C).
- Do systematic tests:
  - Left vs right compartment separately.
  - Step changes (e.g. -15 → -12 → back).
- Capture btsnoop logs when using the **official Flex app** to compare payloads → confirm exact byte meaning.
- Extend with Eco mode and Battery protection writes once setpoint is confirmed.

---

### ⚡ Key Takeaway

Forget fighting XML — you can do everything in nRF by:

- Enabling notify once,
- Sending a keepalive,
- Writing raw hex as a Byte array,
- Watching notifications.

That loop will let you brute-force and map every command the fridge accepts.

---

# Cheat Sheet (ready-to-copy hex payloads)

> Use **Byte array** (no spaces) in nRF Connect write dialog. Use **WRITE\_REQUEST** or the UI's "Request" option.

## Keepalive / Handshake

- **Keepalive (one-shot)**
  - `120600FEFE03010200`
  - Purpose: keep session alive, trigger immediate status notifications.

## Poll / Status

- **Poll Status (alternate)**
  - `1206000002FDFDFD000000000A04`
  - Purpose: request a status frame; device replies on 0x1236.

## Setpoint candidates (try in this order)

> Replace `XX` if noted; these are hypothesis frames derived from captures.

- **Setpoint family (04 family example)**

  - `120600FEFE0406F102XX`  — where `XX` = encoded temp byte.

- **Examples:**

  - +7 °C candidate: `120600FEFE0406F10207`
  - +5 °C candidate: `120600FEFE0406F10205`
  - 0 °C candidate:  `120600FEFE0406F10200`
  - -15 °C candidate: `120600FEFE0406F102F1` *(if negative temps use two's complement byte; F1 is -15 in signed 8-bit)*

## Eco & Battery protection (placeholders)

- **Eco toggle (hypothesis)**
  - `120600FEFE1C0200...` *(needs exact bytes from sniff)*
- **Battery protection H/M/L (hypothesis)**
  - `120600FEFE1c02...` *(needs exact bytes from sniff)*

## Descriptor / Notifications

- **Enable notifications CCCD write (descriptor 0x2902)**
  - `0100` (written to descriptor UUID `00002902-0000-1000-8000-00805f9b34fb` for characteristic 0x1236)

---

# Troubleshooting Quick Reference

- **Import errors**: check for unsupported XML attributes (`title`, `service`, `command_no_response`) — prefer building macros inside the app.
- **Odd hex error**: remove spaces; ensure even-length hex string.
- **No notification after write**: re-enable notify; send keepalive; retry.
- **GATT CONN TIMEOUT (Error 8)**: reconnect and retry; consider shorter intervals between commands.

---

# Example nRF Connect Tap Sequence (copy/paste values)

1. Enable notifications on `00001236-...` (Write descriptor value `0100`).
2. Write keepalive to `00001235-...`: `120600FEFE03010200` (Request).
3. Write setpoint candidate to `00001235-...`: `120600FEFE0406F10207` (Request).
4. Watch notifications on `00001236-...` and check fridge display.

---

# File versions & history

- v1: Initial sniffing & service/char identification.
- v2: Macro schema experiments (multiple failing imports due to mismatched attributes).
- v3: Legacy-format macros that matched user's working files (used `WRITE_REQUEST`, `service-uuid`, etc.).
- v4: Final cheat-sheet + tap-only workflow recommendation.

---

Created as a reference to continue reverse-engineering and testing the Flex Adventure fridge via BLE.

