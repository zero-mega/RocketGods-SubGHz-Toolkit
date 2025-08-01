# RocketGod's SubGHz Toolkit - Enhanced Edition
Reverse engineer Flipper Zero SubGHz protocols and Keeloq Manufacturer Codes with comprehensive analysis for C implementation

<img width="431" height="266" alt="image" src="https://github.com/user-attachments/assets/ef9011ce-8c8e-4ba9-b25b-f02b8d816ac4" />

<img width="1500" height="1000" alt="rocketgod_logo_transparent" src="https://github.com/user-attachments/assets/b14191a5-b509-44c4-be17-ad51e3226ec8" />

## 🚀 Enhanced Features for Protocol Analysis

This enhanced version of the SubGHz Toolkit provides comprehensive protocol implementation data to help you reproduce SubGHz protocols in C. The toolkit now includes advanced analysis features that extract detailed information about protocol implementations, function signatures, timing patterns, and more.

### 📋 Available Analysis Tools

#### 1. **Function Disassembly Analysis**
- Extracts raw bytes from protocol function pointers
- Performs ARM instruction pattern analysis
- Identifies common ARM assembly patterns (STMDB, LDMIA, MOV, LDR, STR, BL, B, BX)
- Helps understand function entry points and calling conventions
- **Output**: `/ext/subghz/analysis/function_disassembly.txt`

#### 2. **Protocol State Analysis**
- Analyzes decoder instance structures
- Dumps memory layout of protocol decoders
- Extracts internal state variables and their values
- Helps understand protocol state machines
- **Output**: `/ext/subghz/analysis/protocol_state_analysis.txt`

#### 3. **Signal Capture Analysis**
- Documents signal capture methodology
- Provides framework for raw signal analysis
- Helps understand timing and encoding patterns
- **Output**: `/ext/subghz/analysis/signal_capture_analysis.txt`

#### 4. **Timing Pattern Analysis**
- Analyzes protocol timing characteristics
- Identifies common timing patterns (Manchester, PWM, PPM, RAW)
- Maps protocol types to timing behaviors
- **Output**: `/ext/subghz/analysis/timing_analysis.txt`

#### 5. **C Header Generation**
- Automatically generates C header files for each protocol
- Includes function pointer type definitions
- Provides protocol structure templates
- Contains implementation notes and guidance
- **Output**: `/ext/subghz/analysis/protocol_headers.h`

#### 6. **Advanced Analysis** (Original)
- Comprehensive protocol registry analysis
- Memory layout and address mapping
- Function pointer extraction
- **Output**: `/ext/subghz/analysis/advanced_analysis.txt`

## 🔧 How to Use for C Protocol Reproduction

### Step 1: Run All Analysis Tools
1. Install the enhanced SubGHz Toolkit on your Flipper Zero
2. Navigate through each analysis option in the menu
3. Wait for each analysis to complete
4. Check the SD card for generated files in `/ext/subghz/analysis/`

### Step 2: Analyze the Generated Data

#### Function Disassembly Analysis
```c
// Example output from function_disassembly.txt
Function: decoder->feed @ 0x20012345
Raw Bytes (first 64 bytes):
  2D E9 00 48 8B B0 00 90 01 90 02 90 03 90 04 90
  ARM Instruction Analysis:
  +00: STMDB (stack push)
  +04: MOV immediate
  +08: LDR (load register)
```

#### Protocol State Analysis
```c
// Example output from protocol_state_analysis.txt
Protocol State Analysis:
  Decoder Instance: 0x20023456
  Decoder Size: 64 bytes
  Decoder Structure Dump:
    +00: 0x20012345  // Protocol pointer
    +04: 0x00000001  // State variable
    +08: 0x00000000  // Counter
```

#### Generated C Headers
```c
// Example from protocol_headers.h
#ifndef PRINCETON_PROTOCOL_H
#define PRINCETON_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// Protocol Information
#define PRINCETON_PROTOCOL_NAME "Princeton"
#define PRINCETON_PROTOCOL_TYPE 0x01
#define PRINCETON_PROTOCOL_FLAG 0x0000000F

// Function Pointer Types
typedef void* (*Princeton_alloc_func)(void* env);
typedef void (*Princeton_free_func)(void* decoder);
typedef void (*Princeton_reset_func)(void* decoder);
typedef void (*Princeton_feed_func)(void* decoder, bool level, uint32_t duration);
typedef void (*Princeton_get_string_func)(void* decoder, FuriString* output);

// Protocol Structure
typedef struct {
    const char* name;
    uint8_t type;
    uint32_t flag;
    struct {
        Princeton_alloc_func alloc;
        Princeton_free_func free;
        Princeton_reset_func reset;
        Princeton_feed_func feed;
        Princeton_get_string_func get_string;
    } decoder;
} Princeton_Protocol;

#endif // PRINCETON_PROTOCOL_H
```

### Step 3: Implement Protocol in C

Using the generated data, you can now implement the protocol:

```c
#include "princeton_protocol.h"

// Extract function pointers from firmware analysis
Princeton_alloc_func princeton_alloc = (Princeton_alloc_func)0x20012345;
Princeton_feed_func princeton_feed = (Princeton_feed_func)0x20012349;

// Implement protocol decoder
typedef struct {
    uint32_t state;
    uint32_t counter;
    uint32_t data;
} PrincetonDecoder;

void* princeton_decoder_alloc(void* env) {
    PrincetonDecoder* decoder = malloc(sizeof(PrincetonDecoder));
    decoder->state = 0;
    decoder->counter = 0;
    decoder->data = 0;
    return decoder;
}

void princeton_decoder_feed(void* decoder_ptr, bool level, uint32_t duration) {
    PrincetonDecoder* decoder = (PrincetonDecoder*)decoder_ptr;
    
    // Implement based on timing analysis
    if (duration < 500) {
        // Short pulse - logic 0
        decoder->data = (decoder->data << 1) | 0;
    } else {
        // Long pulse - logic 1
        decoder->data = (decoder->data << 1) | 1;
    }
    
    decoder->counter++;
}
```

## 📊 Data Extraction Strategy

### 1. **Function Pointer Extraction**
- Use `function_disassembly.txt` to get function addresses
- Analyze ARM instruction patterns to understand calling conventions
- Extract function signatures from disassembly

### 2. **Protocol State Machine**
- Use `protocol_state_analysis.txt` to understand internal state
- Map state variables to protocol behavior
- Understand state transitions and conditions

### 3. **Timing and Encoding**
- Use `timing_analysis.txt` to understand signal characteristics
- Combine with signal capture data for complete picture
- Implement correct timing patterns in your C code

### 4. **Structure Layout**
- Use generated C headers as templates
- Match memory layout from state analysis
- Ensure proper alignment and padding

## 🎯 Advanced Techniques

### Signal Analysis
```c
// Use signal capture data to understand encoding
typedef struct {
    uint32_t short_pulse;  // Logic 0 duration
    uint32_t long_pulse;   // Logic 1 duration
    uint32_t gap_duration; // Between bits
    uint32_t preamble;     // Preamble pattern
} ProtocolTiming;

// Extract from timing_analysis.txt
ProtocolTiming princeton_timing = {
    .short_pulse = 500,   // microseconds
    .long_pulse = 1500,   // microseconds
    .gap_duration = 2000, // microseconds
    .preamble = 0xAAAAAAAA
};
```

### State Machine Implementation
```c
typedef enum {
    STATE_IDLE,
    STATE_PREAMBLE,
    STATE_DATA,
    STATE_END
} ProtocolState;

void protocol_state_machine(PrincetonDecoder* decoder, bool level, uint32_t duration) {
    switch (decoder->state) {
        case STATE_IDLE:
            if (level && duration > 1000) {
                decoder->state = STATE_PREAMBLE;
                decoder->counter = 0;
            }
            break;
        case STATE_PREAMBLE:
            // Implement preamble detection
            break;
        case STATE_DATA:
            // Implement data decoding
            break;
    }
}
```

## 📁 Generated Files Structure

```
/ext/subghz/analysis/
├── function_disassembly.txt      # Function byte analysis
├── protocol_state_analysis.txt   # Decoder state analysis
├── signal_capture_analysis.txt   # Signal capture framework
├── timing_analysis.txt          # Timing pattern analysis
├── protocol_headers.h           # Generated C headers
├── advanced_analysis.txt        # Original comprehensive analysis
└── protocols.txt               # Basic protocol information
```

## 🔍 Tips for Successful C Reproduction

1. **Start with Simple Protocols**: Begin with static protocols that have fixed timing
2. **Use Generated Headers**: The C headers provide the exact structure layout
3. **Analyze Function Patterns**: Look for common patterns in disassembly
4. **Test Incrementally**: Implement and test each component separately
5. **Cross-Reference Data**: Use multiple analysis files to verify your understanding
6. **Document Assumptions**: Keep notes on any assumptions made during reverse engineering

## 🛠️ Building and Installation

1. Copy the `.fap` file to your Flipper Zero's `/apps/` directory
2. Install via Flipper Zero's application manager
3. Run the app and use each analysis tool
4. Extract the generated files from the SD card

## 📞 Support

- **Website**: https://betaskynet.com
- **Discord**: https://discord.gg/thepirates
- **GitHub**: Report issues and feature requests

---

**RocketGod was here** 🏴‍☠️

*Use this enhanced toolkit to unlock the secrets of SubGHz protocols and implement them in your own C applications!*
