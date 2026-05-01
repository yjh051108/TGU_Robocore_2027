# External Integrations

**Analysis Date:** 2026-05-01

## APIs & External Services

**Robotics Visualization (Foxglove):**
- Service: Foxglove WebSocket Server (vendored SDK in `/home/eldwen/TGU_Robocore_2027/tools/foxglove/`)
- SDK: Custom C++ SDK with prebuilt `libfoxglove.so`/`libfoxglove.a` at `/home/eldwen/TGU_Robocore_2027/tools/foxglove/lib/`
- Usage: Real-time robot state visualization via Foxglove Studio (WebSocket protocol on port 8765)
- Auth: None (localhost/lan only)
- Implementation: PImpl pattern in `tools::FoxGloveComm` at `/home/eldwen/TGU_Robocore_2027/tools/foxglove_comm.hpp` and `/home/eldwen/TGU_Robocore_2027/tools/foxglove_comm.cpp`
- Default host/port: `0.0.0.0:8765`

**No other external API integrations.** The project does not use REST APIs, cloud services, or third-party web services.

## Data Storage

**Databases:**
- None. No SQL or NoSQL databases are used.

**File Storage:**
- Local filesystem only
- Camera calibration images (saved from test programs)
- Log files (when file logging enabled, default path `log.txt`)
- TOML configuration files read from filesystem

**Caching:**
- None. No external caching layer (Redis, Memcached, etc.)

## Authentication & Identity

**Auth Provider:**
- None. No authentication system is implemented.
- Serial protocol uses CRC16 for data integrity checking (`/home/eldwen/TGU_Robocore_2027/tools/crc.hpp`), not authentication.

## Monitoring & Observability

**Error Tracking:**
- None. No Sentry, Rollbar, or similar error reporting.

**Logs:**
- Custom `tools::Logger` singleton at `/home/eldwen/TGU_Robocore_2027/tools/logger.hpp` and `/home/eldwen/TGU_Robocore_2027/tools/logger.cpp`
- Features: Log levels (Debug/Info/Warn/Error/Off), console output, file output, module tagging, `std::format`-based formatting
- Thread-safe (mutex-protected)
- `LOG_DEBUG` is compiled out in Release builds (`#ifdef NDEBUG`)
- Log macros: `LOG_INFO`, `LOG_DEBUG`, `LOG_WARN`, `LOG_ERROR`

## CI/CD & Deployment

**Hosting:**
- Not applicable. The software runs on a robotics onboard computer (NUC), not hosted.

**CI Pipeline:**
- None. No GitHub Actions, Jenkins, or other CI configuration detected.

**Deployment method:**
- Manual: `cmake -B build && make -C build -j$(nproc)`
- No autostart scripts or containerization (Docker) in the target project

## Source Project Deployment Comparison (sp_vision_25)

The source project at `/home/eldwen/sp_vision_25/` has autostart/self-healing infrastructure that the target project has not yet implemented:
- `autostart.sh` at `/home/eldwen/sp_vision_25/autostart.sh` — Runs the vision app via `screen` with logging
- `watchdog.sh` (referenced but not present in this repo tree) — Likely handles crash recovery
- Auto-start via `.desktop` file in `~/.config/autostart/sp_vision.desktop`
- Remote desktop: NoMachine
- External visualization: PlotJuggler (separate tool, not integrated)

## Hardware Interface

**Serial Communication (RoboMaster C-board):**
- Protocol: Custom struct-based binary protocol over USB virtual serial port
- Driver: `io::Serial` at `/home/eldwen/TGU_Robocore_2027/io/serial/serial.hpp`
- Library: Boost.Asio
- Features: Frame header synchronization, ring buffer anti-stick/unpack, CRC16 validation, template-based struct parsing
- Baud rate: 2,000,000 (2Mbps) used in tests
- Device path: `/dev/ttyACM0` (configurable)
- Direction: Bidirectional (send commands, receive IMU/state data)

**HikRobot Industrial Camera:**
- Model: HikRobot USB3 Vision (MV series, vid:pid `2bdf:0001`)
- Driver: Aravis 0.8 (GenICam/GStreamer-based) at `/home/eldwen/TGU_Robocore_2027/io/hikrobot/hikrobot.hpp`
- Interface: USB 3.0 with `libusb`
- Implementation status: **Placeholder/Incomplete** at `/home/eldwen/TGU_Robocore_2027/io/hikrobot/hikrobot.cpp` — the camera integration exists as Aravis API calls in test (`/home/eldwen/TGU_Robocore_2027/task/test/test_camera.cpp`) but not yet encapsulated in the `io/hikrobot/` class
- Image format: Bayer RG to BGR conversion via OpenCV
- Calibration: Camera matrix and distortion coefficients stored in TOML config (`/home/eldwen/TGU_Robocore_2027/config/testconfig.toml`)

**Source Project Camera Stack (sp_vision_25) — for reference:**
| Camera Type | SDK | Implementation |
|-------------|-----|----------------|
| HikRobot | MVS SDK (proprietary, `MvCameraControl`) | `/home/eldwen/sp_vision_25/io/hikrobot/` |
| MindVision | MVSDK (proprietary) | `/home/eldwen/sp_vision_25/io/mindvision/` |
| USB Camera | V4L2 (Linux) | `/home/eldwen/sp_vision_25/io/usbcamera/` |
| Abstraction | Polymorphic `CameraBase` + `Camera` factory | `/home/eldwen/sp_vision_25/io/camera.hpp` |

**Source Project Communication Stack (sp_vision_25) — for reference:**
| Interface | Protocol | Implementation |
|-----------|----------|----------------|
| CAN Bus | SocketCAN | `/home/eldwen/sp_vision_25/io/socketcan.hpp` |
| Serial | wjwwood/serial library | `/home/eldwen/sp_vision_25/io/serial/` (submodule) |
| ROS2 | rclcpp (optional) | `/home/eldwen/sp_vision_25/io/ros2/` |

## Environment Configuration

**No environment variables are used.** All runtime configuration uses TOML files.

**Critical configuration files:**
- `/home/eldwen/TGU_Robocore_2027/config/testconfig.toml` — Game settings (enemy_color), camera parameters (name, exposure, gain, vid_pid), camera calibration (camera_matrix, distort_coeffs)

**Required udev rules:**
- `/etc/udev/rules.d/99-hikrobot.rules` — USB permissions for HikRobot camera (vid:pid `2bdf:0001`)

## Webhooks & Callbacks

**Incoming:**
- None. No HTTP server or webhook endpoints.

**Outgoing:**
- None. No outbound webhook calls.

---

*Integration audit: 2026-05-01*
