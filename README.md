# Arduino Pixy2
Dedicated firmware to control the Pixy2 using serial communication.

## Request Packet Format

| Start Flag | Command | Payload Size |
|------------|---------|--------------|
| 1 byte     | 1 byte  | x bytes      |

## Response Packet Format

| Start Flag 1 | Start Flag 2 | Command | Response Data Length | Response |
|--------------|--------------|---------|----------------------|----------|
| `0xA6`       | `0x6A`       | 1 byte  | 1 byte               | x bytes  |

## Requests Overview

| Request | Value  | Payload (* is optional)                      |
|---------|--------|----------------------------------------------|
| IDLE    | `0x10` | pan* (1 byte), tilt* (1 byte), led* (1 byte) |
| LINE    | `0x15` | pan* (1 byte), tilt* (1 byte), led* (1 byte) |
| BLOCKS  | `0x20` | pan* (1 byte), tilt* (1 byte), led* (1 byte) |

### Idle Request
**Request:** `0xA6` `0x10` `0x[pan]` `0x[tilt]` `0x[led]`

This will put the Pixy2 in idle mode not sending any data out.

### Line Request
**Request:** `0xA6` `0x15` `0x[pan]` `0x[tilt]` `0x[led]`

**Response:** `0xA6` `0x6A` `0x15` `0x06` `0x[index]` `0x[flags]` `0x[x0]` `0x[y0]` `0x[x1]` `0x[y1]`

This will set the Pixy2 to line following mode.

### Blocks Request
**Request:** `0xA6` `0x20` `0x[pan]` `0x[tilt]` `0x[led]`

**Response:** `0xA6` `0x6A` `0x20` `0x08` `0x[signature]` `0x[x]` `0x[y]` `0x[width]` `0x[height]` `0x[index]` `0x[angle]` `0x[age]`

This will set the Pixy2 in blocks mode.

### Ready Response
**Response:** `0xA6` `0x6A` `0xFF`

This response will be sent when the Pixy2 is ready to be controlled.

### State Change Response
**Response:** `0xA6` `0x6A` `0x25` `0x[code]` `0x[state]` `0x[frameWidth]` `0x[frameHeight]`

This response will be sent for all occurring state changes.
