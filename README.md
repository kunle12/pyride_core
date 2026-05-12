# PyRideCore

Core C++ library for PyRIDE (Python-based Robot Interactive Development Environment). Provides network communication, device control, Python interpreter hosting, and a telnet-based interactive Python console.

## Dependencies

- **ccrtp** / **ccgnu2** — RTP streaming for audio/video
- **libjpeg** — JPEG encode/decode for camera snapshots and video
- **OpenSSL** — message encryption (`-DUSE_ENCRYPTION`)
- **Python 3** — C API for hosting the Python interpreter
- **opus** — audio encoding/decoding
- **tinyxml** — configuration file parsing
- **async-web-server-cpp** — HTTP video streaming bridge
- **Pthreads** / **dl** — threading and dynamic loading

## Build

### QiBuild (legacy NAO robot)
```bash
qibuild configure
qibuild make
```

### ROS2 colcon (current)
```bash
colcon build --packages-select pyride
```

The CMake flag `-DOLD_NAO` selects between `<opus/opus.h>` and `<opus.h>`.

## Files

### Core infrastructure
| File | Role |
|------|------|
| `PyRideCommon.h/.cpp` | Shared types, encryption (`endecryptInit`, AES-CBC + Base64), utility functions |
| `PyRideNetComm.h/.cpp` | UDP/TCP network I/O, client list management, timers, video/audio stream dispatch |
| `PyModuleStub.h/.cpp` | Python module glue — exposes PyRIDE C++ functions to Python scripts via `PyObject` methods |
| `PyModulePyCommon.cpp` | Python-bound functions: video dispatch, audio dispatch, sensor data |
| `AppConfigManager.h/.cpp` | XML config parser (robot info, user access, device records) via TinyXML |

### Telnet Python Console
| File | Role |
|------|------|
| `PythonServer.h/.cpp` | Telnet server + Python REPL. Manages TCP listener, client sessions, line editing, history |
| `ServerDataProcessor.h/.cpp` | Server-side command routing, timer management |
| `ConsoleDataProcessor.h/.cpp` | Console-side callback handling (telemetry, stream control) |

### Audio / Video
| File | Role |
|------|------|
| `DeviceController.h/.cpp` | Base class for audio/video devices. RTP session, UDP port discovery, snapshot file I/O |
| `AudioDataReceiver.h/.cpp` | Opus audio decoder, RTP audio stream receiver |
| `ImageDataReceiver.h/.cpp` | JPEG image stream receiver via RTP |
| `VideoToWebBridge.h/.cpp` | HTTP multipart JPEG streamer for web clients |
| `RTPDataReceiver.h/.cpp` | Generic RTP data receiver thread |
| `jdatabufferdst.h/.cpp` | libjpeg memory destination manager (writes JPEG to memory buffer) |

### Miscellaneous
| File | Role |
|------|------|
| `Constants.h` | iOS layout constants (legacy, mostly unused) |
| `PyRideCustom.h` | Extended command handler interface |
| `qiproject.xml` | QiBuild project metadata |

## Telnet Console

The telnet server listens on port **27005**. Each connecting client gets a `PythonSession` with full line editing, command history (up/down arrows), and tab-completion for Python objects.

### UTF-8 Support

The telnet console accepts multi-byte UTF-8 input including CJK characters:

- Input bytes ≥ 128 are detected as UTF-8 leading bytes via sequence-length decoding
- Multi-byte sequences are buffered and inserted as complete characters
- Character deletion navigates by UTF-8 character boundaries
- Cursor movement (left/right/home/end) uses display width (1 for ASCII, 2 for CJK)
- Tab completion cursor positioning accounts for display width
- The connection handshake includes `IAC DO CHARSET` / `IAC SB CHARSET 1 UTF-8 IAC SE` (RFC 2066) to request UTF-8 encoding from the client

### Telnet Protocol Handling

- Line mode (`IAC DO LINEMODE`) for character-by-character editing
- Local echo (`IAC WILL ECHO`) — the server manages echo
- VT100 escape sequences for arrow keys
- Subnegotiation data is ignored

## Notes

- `USE_ENCRYPTION` is always defined (set in `CMakeLists.txt`).
- Most source files use C-style casts and raw pointers — a known codebase convention.
- The library was originally built for the NAO robot platform and is in active migration from ROS1 to ROS2.
