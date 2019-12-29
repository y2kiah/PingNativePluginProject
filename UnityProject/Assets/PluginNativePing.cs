using System;
using System.Runtime.InteropServices;
using UnityEngine;

enum SequenceStatus : uint {
    Sequence_Inactive = 0,
    Sequence_Running,
    Sequence_Finished,
    Sequence_Error
};


[StructLayout(LayoutKind.Sequential)]
public struct PingStats
{
    uint  sent;
    uint  received;
    uint  lost;
    float pctLost;
    float minRoundTrip;
    float maxRoundTrip;
    float avgRoundTrip;
    float stdDevRoundTrip;
}


[StructLayout(LayoutKind.Sequential)]
public struct PingJob
{
    uint           hnd;
	SequenceStatus status;
	PingStats      stats;
}

public class PluginNativePing : MonoBehaviour
{
    const ushort DefaultNumRequests = 1;
    const ushort DefaultDataSize    = 32;
    const byte   DefaultTTL         = 128;
    const ushort DefaultTimeoutMS   = 1000;
    const ushort DefaultIntervalMS  = 16;
    

    [DllImport("unity-ping", CallingConvention = CallingConvention.Cdecl)]
    private static extern
    PingJob
    CreatePing(
        [MarshalAs(UnmanagedType.LPStr)]
        string host,
        ushort numRequests = DefaultNumRequests,
        ushort dataSize    = DefaultDataSize,
        byte   ttl         = DefaultTTL,
        ushort timeoutMS   = DefaultTimeoutMS,
        ushort intervalMS  = DefaultIntervalMS);

    
    [DllImport("unity-ping", CallingConvention = CallingConvention.Cdecl)]
    private static extern
    bool
    PollPingResult(
        ref PingJob ping);


    void
    Start()
    {
        Debug.Log("hello world");
        //Debug.Log(PrintANumber());
        //Debug.Log(Marshal.PtrToStringAnsi(PrintHello()));
        //Debug.Log(AddTwoIntegers(2, 2));
        //Debug.Log(AddTwoFloats(2.5F, 4F));

        PingJob[] pings = {
            CreatePing(
                "192.168.0.185", // host
                10,				 // number of requests in sequence
                64,			     // data size
                30,				 // ttl
                1000),			 // timeout ms
            CreatePing("google.com", 10),
            CreatePing("yahoo.com", 10),
            //CreatePing("127.0.0.1"), // Note: adding localhost to the mix seems to invalidate other sockets
            CreatePing("gamedev.net", 10),
            CreatePing("unity3d.com", 10)
        };

        for(;;) {
            int finishedCount = 0;
            
            for(int p = 0;
                p < pings.Length;
                ++p)
            {
                if (PollPingResult(ref pings[p])) {
                    ++finishedCount;
                }
            }

            if (finishedCount == pings.Length) {
                break;
            }
        }

    }
}