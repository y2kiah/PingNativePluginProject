using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Text;
using System.Threading.Tasks;


public enum SequenceStatus : uint {
    Sequence_Inactive = 0,
    Sequence_Running,
    Sequence_Finished,
    Sequence_Error
};


[StructLayout(LayoutKind.Sequential)]
public struct PingStats
{
    public uint  sent;
    public uint received;
    public uint lost;
    public float pctLost;
    public float minRoundTrip;
    public float maxRoundTrip;
    public float avgRoundTrip;
    public float stdDevRoundTrip;


    public override string ToString()
    {
        StringBuilder sb = new StringBuilder();
        sb.AppendLine($"sent: {sent}");
        sb.AppendLine($"received: {received}");
        sb.AppendLine($"lost: {lost}");
        sb.AppendLine($"pctLost: {pctLost:F3}");
        sb.AppendLine($"minRoundTrip: {minRoundTrip:F3}ms");
        sb.AppendLine($"maxRoundTrip: {maxRoundTrip:F3}ms");
        sb.AppendLine($"avgRoundTrip: {avgRoundTrip:F3}ms");
        sb.AppendLine($"stdDevRoundTrip: {stdDevRoundTrip:F3}");
        return sb.ToString();
    }
}


[StructLayout(LayoutKind.Sequential)]
public struct PingJob
{
    public uint           hnd;
    public SequenceStatus status;
    public PingStats      stats;

    public override string ToString()
    {
        return $"status: {status}\n" + stats;
    }
}


public class PluginNativePing : MonoBehaviour
{
    const ushort DefaultNumRequests = 1;
    const ushort DefaultDataSize    = 32;
    const byte   DefaultTTL         = 128;
    const ushort DefaultTimeoutMS   = 1000;
    const ushort DefaultIntervalMS  = 16;
    

    [DllImport("unity-ping", CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Ansi)]
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


    async
    void
    Start()
    {
        PingJob[] pings = {
            // TODO: change this to a known IP on your local network
            CreatePing(
                "192.168.0.185", // host
                10,              // number of requests in sequence
                64,              // data size
                30,              // ttl
                1000),           // timeout ms
            CreatePing("google.com", 10),
            CreatePing("yahoo.com", 10),
            CreatePing("gamedev.net", 10),
            CreatePing("unity3d.com", 10),
            // we expect this to fail name resolution and return an error with 0 packets sent
            CreatePing("intentionallycantfindthis.com", 10),
            // we expect error result or packet loss with these due to too-low ttl and timeout values
            CreatePing("google.com", 10, 32, 1, 1000), // low ttl
            CreatePing("google.com", 10, 32, 128, 1) // low timeout
            // Note: adding localhost to the mix seems to invalidate other sockets
            //CreatePing("127.0.0.1")
        };

        for(;;) {
            int finishedCount = 0;
            
            for(int p = 0;
                p < pings.Length;
                ++p)
            {
                bool wasFinished = (pings[p].status > SequenceStatus.Sequence_Running);

                if (PollPingResult(ref pings[p])) {
                    ++finishedCount;

                    // make sure we only print the results once
                    if (!wasFinished) {
                        Debug.Log(pings[p]);
                    }
                }
            }

            if (finishedCount == pings.Length) {
                break;
            }

            await Task.Delay(TimeSpan.FromMilliseconds(16));
        }
    }
}