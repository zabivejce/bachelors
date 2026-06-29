# SDR Server

A C++ server application for real-time FM demodulation and streaming.

## Prerequisites

The following packages are required for compilation and execution:
- build-essential (gcc, make)
- librtlsdr-dev
- rtl-sdr

On Ubuntu/Debian, install them using:
sudo apt update && sudo apt install build-essential rtl-sdr librtlsdr-dev

## Building the Project

The project uses a standard Makefile. To compile the source code and generate the executable:

make

This will create a binary file named 'program'.

## Cleaning Build Files

To remove compiled object files and the executable:

make clean

## Usage

The application receives raw I/Q data from standard input. Connect your RTL-SDR hardware and pipe the data to the program:

rtl_sdr -f [frequency] -s 2.4M - | ./program

Example for 101.1 MHz:
rtl_sdr -f 101.1M -s 2.4M - | ./program

## Connecting

Once the server is running, use a media player like VLC to connect to the output TCP stream:
vlc tcp://127.0.0.1:8008