#include "build_config.h"
#include "platform/platform.h"
#include "platform/ping.h"
#include "unity/IUnityInterface.h"

#include "platform/platform.cpp"
#include "platform/timer.cpp"
#include "platform/ping.cpp"


extern "C"
{

// Unity plugin load event
void
UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(
    IUnityInterfaces* unityInterfaces)
{}


// Unity plugin unload event
void
UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{}


/**
 * Adds a ping job and runs it immediately on the job thread. This is a non-blocking call.
 * @returns Ping struct with a non-zero hnd on success, or 0 in hnd if job queue is full 
 */
Ping
UNITY_INTERFACE_EXPORT
CreatePing(
    const char* host,
    u16 numRequests = DefaultNumRequests,
    u16 dataSize    = DefaultDataSize,
    u8  ttl         = DefaultTTL,
    u16 timeoutMS   = DefaultTimeoutMS,
    u16 intervalMS  = DefaultIntervalMS)
{
    return ping(host, numRequests, dataSize, ttl, timeoutMS, intervalMS);
}

/**
 * Checks poll sequence status for completion and stores a copy of the resulting PingStats.
 * If ping.status is Sequence_Finished, ping.stats is filled.
 * If ping.status is Sequence_Error, ping.stats is not written to.
 * In both of the above cases, the job is removed and ping.hnd is cleared to zero.
 * If ping.status is Sequence_Running, the process is still running.
 * If ping.status is Sequence_Inactive, the process has not yet started running.
 * If called with a cleared ping.hnd, the function returns based on existing ping.status.
 * @returns true if job is finished running (Sequence_Finished or Sequence_Error), false if the job
 *  is still running (Sequence_Running or Sequence_Inactive)
 */
bool
UNITY_INTERFACE_EXPORT
PollPingResult(
    Ping* ping)
{
    if (ping == nullptr) {
        return false;
    }
    
    return pollResult(*ping);
}


}
