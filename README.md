# Multi-Threaded SDR FM Streamer

A high-performance, multi-threaded C++ server that processes raw I/Q data from an RTL-SDR device and streams demodulated FM radio stations directly to standard media players via HTTP.

## Architecture
The application consists of a main **HTTP Dispatcher** and independent **Worker Threads**. 
1. The user connects to the main API (`http://<IP>:8080/<FREQUENCY>`).
2. The Dispatcher calculates the required frequency offset, spawns a new DSP Worker Thread on an available port, and replies with an `HTTP 302 Redirect`.
3. The client (e.g., VLC) seamlessly reconnects to the new port and receives a continuous PCM audio stream wrapped in a WAV header.

## Features
* **One-Click Playback:** Native support for VLC, web browsers, and command-line tools (`wget`, `curl`). No manual offset calculations are needed.
* **Multi-Threaded:** Each client/station gets its own worker thread for independent DSP processing.
* **On-the-fly DSP:** Real-time frequency shifting (phase rotation), low-pass FIR filtering, decimation (anti-aliasing), and FM demodulation.
* **SIMD Optimized:** Utilizes `-fopenmp-simd` and `-ffast-math` for efficient signal processing.
* **Dynamic Safety Bounds:** Automatically rejects tuning requests outside the physical bandwidth (e.g., +/- 1.2 MHz for a 2.4 MS/s rate) with an `HTTP 416` response.

## Prerequisites
* Linux OS
* `rtl-sdr` package (for the `rtl_sdr` CLI tool)
* `cmake` (version 3.10 or higher)
* `g++` (with C++17 support and pthread)

## Building the Project
The project uses CMake for building. Run the following commands in the project root:

    mkdir build
    cd build
    cmake ..
    make -j$(nproc)

## Running the Server
The application reads raw 8-bit unsigned I/Q data from the standard input (`stdin`). You need to pipe the output of the `rtl_sdr` tool directly into the application.

**Usage:**

    ./program <center_frequency_MHz>

**Complete Example:**
Tune the RTL-SDR to 100.0 MHz with a sample rate of 2.4 MS/s, and start the streaming server:

    rtl_sdr -f 100.0M -s 2.4M - | ./program 100.0

## Listening to a Station
Once the server is running, the main API listens on port `8080`. 
To listen to a specific station, connect using the desired frequency in the URL.

**Using VLC Media Player:**
1. Open VLC.
2. Press `Ctrl + N` (Open Network Stream).
3. Enter the URL: `http://127.0.0.1:8080/101.1` (replace `127.0.0.1` with your server's IP if accessing over a LAN).
4. Click Play.

**Using Command Line:**
You can test the stream using `curl` or `wget`.

    # Using wget (automatically follows HTTP 302 redirects)
    wget http://127.0.0.1:8080/101.1 -O stream.wav
    
    # Using curl (requires -L to follow redirects)
    curl -L http://127.0.0.1:8080/101.1 -o stream.wav

*(Stop the stream download with `Ctrl+C` and play the resulting file with `aplay stream.wav`)*