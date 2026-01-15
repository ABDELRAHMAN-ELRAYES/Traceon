# Packet VS TLP

## Packet (Raw Hexadecimal)

**transaction level container**

1. timestamp
2. direction
3. Raw Hexadecimal bytes

## TLP (Protocol Level)

### Core Identity

- timestamp
- direction
- original packet index

### Header Fields

- fmt
- type
- length (DW) `null`
- requester ID
- tag

### Address / Routing

- address (if applicable) `null`
- completer ID (if applicable) `null`

### Payload Info

- has_data
- byte_count (for completions) `null`
- status (for completions -> ) `null`
  - `"SC"` (Success)
  - `"UR"` (Unsupported Request)
  - `"CA"` (Completer Abort)
  - `"TO"` (Timeout)

### Decode Status

Very important:

- `is_malformed`
- `decode_errors[]`

---

# Errors Types

## 1. Structural Errors (Maformed TLP)

- Detected in: **Decoding Layer**

Examples:

    - Header bits illegal
    - Length field impossible
    - Required fields missing

Result:

    is_malformed = true
    decode_errors[] = {...}

## 2. Protocol Errors (Protocol Rules violation)

Detected in: **Validation Layer**

Examples:

    - Completion without request
    - Read never completed
    - Wrong byte count

Result:

    Validation error report
    But packet is still a valid TLP

`Validator must skip Malformed Packets but they must be reported with the decod_errors in the report.`
