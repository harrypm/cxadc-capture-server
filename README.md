# cxadc-capture-server

A HTTP server for synchronized RF capture of various analog media formats (VHS, Betamax, V2000, Video8, Hi8, etc.) using multiple CX cards and [clockgen mod](https://github.com/oyvindln/vhs-decode/wiki/Clockgen-Mod) hardware.

## Features

- **Multi-card synchronization**: Synchronized capture from up to 2 CX cards
- **Baseband audio capture**: Simultaneous capture of baseband audio (linear or device-decoded HiFi)
- **Dynamic file naming**: Automatic inclusion of sample rates and bit depths in filenames
- **Advanced compression**: FLAC compression with maximum compression levels (11)
- **Intelligent resampling**: SoX-based resampling for bandwidth optimization
- **Signal protection**: Prevents ALSA device locks on interruption
- **Real-time statistics**: Buffer monitoring and overflow detection

## Usage

`cxadc-capture-server version|<port>|unix:<socket>`

> ⚠️ Do not expose the server to the public internet. It is not intended to be secure.

### HTTP Endpoints

- **GET `/`**: Hello world response
- **GET `/version`**: Returns server version
- **GET `/start`**: Start a capture session. Returns JSON with statistics
  - Parameters:
    - `cxadc<number>`: Capture from `/dev/cxadc<number>` device
    - `lname=<device>`: ALSA device for baseband capture (default: `hw:CARD=CXADCADCClockGe`)
    - `lformat=<format>`: Baseband capture format (default: auto-detect)
    - `lrate=<rate>`: Baseband sample rate in Hz (default: auto-detect)
    - `lchannels=<channels>`: Baseband channel count (default: auto-detect)
- **GET `/cxadc`**: Stream RF data from CX card
  - Parameter: `<number>` - Access the Nth captured card (0-indexed)
- **GET `/baseband`**: Stream baseband audio data
- **GET `/stats`**: Real-time capture statistics and buffer status
- **GET `/stop`**: Stop current capture and return overflow statistics


## Examples

### Remote capture

Start the server on the capture machine:

```bash
cxadc-capture-server 8080
```

Queue up the download of the streams:

```bash
aria2c -Z \
    http://192.168.1.1:8080/baseband \
    http://192.168.1.1:8080/cxadc?0 \
    http://192.168.1.1:8080/cxadc?1
```

Start the capture:

```bash
curl http://192.168.1.1:8080/start?cxadc0&cxadc1
```

Stop the capture:

```bash
curl http://192.168.1.1:8080/stop
```

### Local capture

The included `local-capture.sh` script provides an easy interface for local captures using a UNIX socket. This approach offers sample drop resilient buffering and better synchronization than direct streaming.

#### Dependencies

**Required:**
- `bash` - Shell for script execution
- `curl` - HTTP client for server communication  
- `jq` - JSON processor for parsing server responses
- `cxadc-capture-server` - The capture server binary

**Optional (for advanced features):**
- `flac` - FLAC compression (v1.5.0+ recommended for optimal compression)
- `sox` - Audio resampling and processing
- `ffmpeg` - Baseband audio processing

**Installation:**

*RHEL / Fedora:*
```bash
yum install bash curl jq flac sox ffmpeg
```

*Debian / Ubuntu:*
```bash
apt install bash curl jq flac sox ffmpeg
```

The `cxadc-capture-server` binary can be obtained from releases or compiled from source. Binary releases support glibc 2.17 and later.


#### Usage

```bash
local-capture.sh [options] <basepath>
```

**Options:**
- `--video=N` - Use CX card N for video capture (disabled if unset)
- `--hifi=N` - Use CX card N for hifi capture (disabled if unset)  
- `--baseband=DEVICE` - ALSA device for baseband audio (default: auto-detect)
- `--add-date` - Add timestamp to filenames
- `--convert-baseband` - Convert baseband to FLAC + separate headswitch track
- `--compress-video` - Compress video using FLAC
- `--compress-video-level=N` - Video compression level 0-11 (default: 11)
- `--compress-hifi` - Compress hifi using FLAC
- `--compress-hifi-level=N` - Hifi compression level 0-11 (default: 11)
- `--resample-hifi` - Resample hifi from 40 MSps to 10 MSps
- `--resample-video` - Resample video from 40 MSps to 20 MSps
- `--debug` - Show executed commands
- `--help` - Display usage information

#### Example

**Basic capture with compression and resampling:**

```bash
./local-capture.sh --video=0 --hifi=1 --convert-baseband --compress-video --compress-hifi --resample-hifi --resample-video my_tape
```

**Terminal output:**

```
Server started (PID 3854)
server listening on unix:/tmp/tmp.qDMBd0Ynxu/server.sock
PID 3872 is capturing video to my_tape-video_20msps_8-bit.flac
PID 3874 is capturing hifi to my_tape-hifi_10msps_8-bit.flac
PID 3876 is capturing baseband to my_tape-baseband_46.9msps_24-bit.flac, headswitch to my_tape-headswitch_46.9msps_8-bit.u8
Capture running... Press 'q' to stop the capture.
Capturing for 0m 0s... Buffers:  0%  0%  0%
Capturing for 0m 5s... Buffers:  0%  0%  0%
```

Press 'q' to stop:

```
Stopping capture
Encountered 0 overflows during capture
Waiting for writes to finish...
Killing server
Finished!
```

## File Naming Convention

Files are automatically named with sample rate and bit depth information:

- **Video**: `name-video_40msps_8-bit.u8` (raw) or `name-video_20msps_8-bit.flac` (resampled + compressed)
- **HiFi**: `name-hifi_40msps_8-bit.u8` (raw) or `name-hifi_10msps_8-bit.flac` (resampled + compressed)  
- **Baseband**: `name-baseband_{rate}msps_24-bit.s24` (raw) or `name-baseband_{rate}msps_24-bit.flac` (compressed)
- **Headswitch**: `name-headswitch_{rate}msps_8-bit.u8`

Sample rates are dynamically detected and included in filenames for easy identification.
