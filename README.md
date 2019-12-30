# Introduction 
This project is a Unity native plugin that implements "ping" (ICMP echo request) on Windows and Linux.
The native project includes a test.exe/.out, as well as the .dll/.so for Unity.
A sample Unity project is also included that calls the plugin from managed code.

# Overview
This library uses non-blocking raw sockets to handle up to 64 ping sequences simultaneously.
Ping sequences allow several requests to be sent, and statistics are calculated from the results.
A background thread is automatically managed to handle the ping workload in a way that will collect accurate timing while not blocking a GUI/game thread.

# Build and Test
## Windows
run shell.bat or open a MSVC console
run build.bat

Tested with MSVC 2017 Community on Windows 7 Professional SP1

## Linux

run ./build.sh

Raw sockets require superuser privileges on linux, to avoid this, set cap_net_raw on the executable with this command:
sudo setcap cap_net_raw,cap_net_admin,cap_dac_override+eip test.out

Tested with g++ (GCC) 9.2.1 on Fedora 30
