#ifndef _PING_H
#define _PING_H

#include "icmp.h"

#define MaxPingJobs         64
#define MaxSequenceRequests 16
#define DefaultNumRequests  1
#define DefaultDataSize     32
#define DefaultTTL          128
#define DefaultTimeoutMS    1000
#define DefaultIntervalMS   16
#define MaxPacketSize       512
#define ReceiveBufferSize   1024


enum Result : s32 {
    Result_Error   = -1,
    Result_Success = 0,
    Result_Ignore  = 1,
    Result_Pending = 2
};

bool ok(s32 r) {
    return (r != Result_Error);
}

enum PingStatus : u8 {
    Ping_Inactive = 0,
    Ping_Requested,
    Ping_WaitingForReply,
    Ping_Received,
    Ping_TimedOut,
    Ping_Error
};

enum SequenceStatus : u32 {
    Sequence_Inactive = 0,
    Sequence_Running,
    Sequence_Finished,
    Sequence_Error
};

typedef h32 PingJobHnd;

struct PingRequest {
    ICMPHeader  requestHdr;
    ICMPHeader  replyHdr;
    i64         sendTime;
    i64         replyTime;
    r32         elapsedMS;
    u8          ttl;
    PingStatus  status;
    
    u16         _pad;
};

struct PingStats {
    u32         sent;
    u32         received;
    u32         lost;
    r32         pctLost;
    r32         minRoundTrip;
    r32         maxRoundTrip;
    r32         avgRoundTrip;
    r32         stdDevRoundTrip;
};

struct PingSequence {
    char*       host;
    atomic_u32  status;
    u16         dataSize;
    u16         numRequests;
    u16         timeoutMS;
    u16         intervalMS;
    u16         seq;
    u8          ttl;
    
    u8          _pad;

    PingRequest requests[MaxSequenceRequests];
    PingStats   stats;
};

struct Ping {
	PingJobHnd     hnd;
	SequenceStatus status;
	PingStats      stats;
};


#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

// Linux
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#endif

struct PingJob {
    PingSequence   sequence;
    SOCKET         socket;
    sockaddr_in    destAddr;
    sockaddr_in    sourceAddr;
    u8             sendBuffer[MaxPacketSize];
    u8             receiveBuffer[ReceiveBufferSize];
};


#include "../utility/sparse_handle_map_16.h"
#include "../utility/concurrent_queue.h"

SparseHandleMap16_Typed_WithBuffer(
	PingJob,
	PingJobMap,
	PingJobHnd,
	0,
	MaxPingJobs);

ConcurrentQueue_Typed_WithBuffer(
	PingJobHnd,
	PingJobQueue,
	MaxPingJobs,
	0);



/**
 * Adds a ping job and runs it immediately on the job thread. This is a non-blocking call.
 * @returns Ping struct with a non-zero hnd on success, or 0 in hnd if job queue is full 
 */
Ping
ping(
    const char* host,
    u16 numRequests = DefaultNumRequests,
    u16 dataSize    = DefaultDataSize,
    u8  ttl         = DefaultTTL,
    u16 timeoutMS   = DefaultTimeoutMS,
    u16 intervalMS  = DefaultIntervalMS); // TODO: interval not implemented

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
pollResult(
    Ping& ping);


SequenceStatus
runPingSequence(
    PingJob& job);

#endif