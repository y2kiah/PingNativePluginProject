# Introduction 
This project is a Unity native plugin that implements "ping" (ICMP echo request) on Windows and Linux.
The native project includes a test.exe/.out, as well as the .dll/.so for Unity.
A sample Unity project is also included that calls the plugin from managed code.

# Overview
This library uses non-blocking raw sockets to handle up to 64 ping sequences simultaneously.
Ping sequences allow a series of requests to be sent to a host, and statistics to be calculated from the results.
A background thread is automatically managed to handle the ping workload in a way that will collect accurate timing while not blocking a GUI/game thread.

# Getting Started
The API is extremely simple, only two functions are required. See `test.cpp` and `PluginNativePing.cs` for native C++ and managed C# examples respectively.
```c++
// creates ping sequence job and begins running it on background thread
Ping p =
    ping(
        "google.com", // hostname or dotted-quad IP
        10,           // number of requests in sequence
        32U,          // data size
        128,          // ttl
        1000U);       // timeout ms

// demonstrates a busy-wait for finished, error or timeout (normally you would not do this)
while (!pollResult(p)) {}
```

# Build and Test
## Windows
run `shell.bat` or open a MSVC console
run `build.bat`

Tested with MSVC 2017 Community on Windows 7 Professional SP1

## Linux
run `./build.sh`

Raw sockets require superuser privileges on linux, to avoid this, set cap_net_raw on the executable with this command:
`sudo setcap cap_net_raw,cap_net_admin,cap_dac_override+eip test.out`

Tested with g++ (GCC) 9.2.1 on Fedora 30
