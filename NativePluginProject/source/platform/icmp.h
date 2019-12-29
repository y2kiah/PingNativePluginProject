#ifndef _ICMP_H
#define _ICMP_H

#include "../utility/common.h"

enum ICMPType : u8 {
    ICMPType_EchoReply              = 0x00,
    ICMPType_DestinationUnreachable = 0x03,
    ICMPType_RedirectMessage        = 0x05,
    ICMPType_EchoRequest            = 0x08,
    ICMPType_RouterAdvertisement    = 0x09,
    ICMPType_RouterSolicitation     = 0x0A,
    ICMPType_TimeExceeded           = 0x0B,
    ICMPType_ParameterProblem       = 0x0C,
    ICMPType_Timestamp              = 0x0D,
    ICMPType_TimestampReply         = 0x0E,
    ICMPType_ExtendedEchoRequest    = 0x2A,
    ICMPType_ExtendedEchoReply      = 0x2B
};

enum ICMPControlMessage : u16 {
    // EchoReply messages
    ICMP_EchoReply                        = 0x0000,
    // DestinationUnreachable messages
    ICMP_DestinationNetworkUnreachable    = 0x0003,
    ICMP_DestinationHostUnreachable       = 0x0103,
    ICMP_DestinationProtocolUnreachable   = 0x0203,
    ICMP_DestinationPortUnreachable       = 0x0303,
    ICMP_FragmentationRequired            = 0x0403,
    ICMP_SourceRouteFailed                = 0x0503,
    ICMP_DestinationNetworkUnknown        = 0x0603,
    ICMP_DestinationHostUnknown           = 0x0703,
    ICMP_SourceHostIsolated               = 0x0803,
    ICMP_NetworkAdminProhibited           = 0x0903,
    ICMP_HostAdminProhibited              = 0x0A03,
    ICMP_NetworkUnreachableForToS         = 0x0B03,
    ICMP_HostUnreachableForToS            = 0x0C03,
    ICMP_CommunicationAdminProhibited     = 0x0D03,
    ICMP_HostPrecedenceViolation          = 0x0E03,
    ICMP_PrecedenceCutoffInEffect         = 0x0F03,
    // RedirectMessage messages
    ICMP_RedirectDatagramForNetwork       = 0x0005,
    ICMP_RedirectDatagramForHost          = 0x0105,
    ICMP_RedirectDatagramForTosAndNetwork = 0x0205,
    ICMP_RedirectDatagramForTosAndHost    = 0x0305,
    // EchoRequest messages
    ICMP_EchoRequest                      = 0x0008,
    // RouterAdvertisement messages
    ICMP_RouterAdvertisement              = 0x0009,
    // RouterSolicitation messages
    ICMP_RouterSolicitation               = 0x000A,
    // TimeExceeded messages
    ICMP_TTLExpiredInTransit              = 0x000B,
    ICMP_FragmentReassemblyTimeExceeded   = 0x010B,
    // ParameterProblem messages
    ICMP_PointerIndicatesError            = 0x000C,
    ICMP_MissingRequiredOption            = 0x010C,
    ICMP_BadLength                        = 0x020C,
    // Timestamp messages
    ICMP_Timestamp                        = 0x000D,
    // TimestampReply messages
    ICMP_TimestampReply                   = 0x000E,
    // ExtendedEchoRequest messages
    ICMP_RequestExtendedEcho              = 0x002A,
    // ExtendedEchoReply messages
    ICMP_NoError                          = 0x002B,
    ICMP_MalformedQuery                   = 0x012B,
    ICMP_NoSuchInterface                  = 0x022B,
    ICMP_NoSuchTableEntry                 = 0x032B,
    ICMP_MultipleInterfacesSatisfyQuery   = 0x042B
};

struct ICMPControlMessageString {
    ICMPControlMessage cm;
    const char* str;
};

const ICMPControlMessageString ControlMessageStrings[] = {
    // EchoReply messages
    { ICMP_EchoReply,                        "Echo Reply" },
    // DestinationUnreachable messages
    { ICMP_DestinationNetworkUnreachable,    "Destination Network Unreachable" },
    { ICMP_DestinationHostUnreachable,       "Destination Host Unreachable" },
    { ICMP_DestinationProtocolUnreachable,   "Destination Protocol Unreachable" },
    { ICMP_DestinationPortUnreachable,       "Destination Port Unreachable" },
    { ICMP_FragmentationRequired,            "Fragmentation Required" },
    { ICMP_SourceRouteFailed,                "Source Route Failed" },
    { ICMP_DestinationNetworkUnknown,        "Destination Network Unknown" },
    { ICMP_DestinationHostUnknown,           "Destination Host Unknown" },
    { ICMP_SourceHostIsolated,               "Source Host Isolated" },
    { ICMP_NetworkAdminProhibited,           "Network Admin Prohibited" },
    { ICMP_HostAdminProhibited,              "Host Admin Prohibited" },
    { ICMP_NetworkUnreachableForToS,         "Network Unreachable For ToS" },
    { ICMP_HostUnreachableForToS,            "Host Unreachable For ToS" },
    { ICMP_CommunicationAdminProhibited,     "Communication Admin Prohibited" },
    { ICMP_HostPrecedenceViolation,          "Host Precedence Violation" },
    { ICMP_PrecedenceCutoffInEffect,         "Precedence Cutoff In Effect" },
    // RedirectMessage messages
    { ICMP_RedirectDatagramForNetwork,       "Redirect Datagram For Network" },
    { ICMP_RedirectDatagramForHost,          "Redirect Datagram For Host" },
    { ICMP_RedirectDatagramForTosAndNetwork, "Redirect Datagram For Tos And Network" },
    { ICMP_RedirectDatagramForTosAndHost,    "Redirect Datagram For Tos And Host" },
    // EchoRequest messages
    { ICMP_EchoRequest,                      "Echo Request" },
    // RouterAdvertisement messages
    { ICMP_RouterAdvertisement,              "Router Advertisement" },
    // RouterSolicitation messages
    { ICMP_RouterSolicitation,               "Router Solicitation" },
    // TimeExceeded messages
    { ICMP_TTLExpiredInTransit,              "TTL Expired In Transit" },
    { ICMP_FragmentReassemblyTimeExceeded,   "Fragment Reassembly Time Exceeded" },
    // ParameterProblem messages
    { ICMP_PointerIndicatesError,            "Pointer Indicates Error" },
    { ICMP_MissingRequiredOption,            "Missing Required Option" },
    { ICMP_BadLength,                        "Bad Length" },
    // Timestamp messages
    { ICMP_Timestamp,                        "Timestamp" },
    // TimestampReply messages
    { ICMP_TimestampReply,                   "Timestamp Reply" },
    // ExtendedEchoRequest messages
    { ICMP_RequestExtendedEcho,              "Request Extended Echo" },
    // ExtendedEchoReply messages
    { ICMP_NoError,                          "No Error" },
    { ICMP_MalformedQuery,                   "Malformed Query" },
    { ICMP_NoSuchInterface,                  "No Such Interface" },
    { ICMP_NoSuchTableEntry,                 "No Such Table Entry" },
    { ICMP_MultipleInterfacesSatisfyQuery,   "Multiple Interfaces Satisfy Query" }
};


#pragma pack(1)

struct IPHeader
{
    u8  headerLen  : 4;  // Length of the header in dwords
    u8  version    : 4;  // Version of IP
    u8  ecn        : 2;  // Explicit Congestion Notification
    u8  dcsp       : 6;  // Differentiated Services Code Point
    u16 totalLen;        // Total length of the packet in bytes
    u16 ident;           // unique identifier
    u16 fragOffset : 13;
    u16 flags      : 3;
    u8  ttl;            // Time to live
    u8  protocol;       // Protocol number (TCP, UDP etc)
    u16 checksum;       // IP checksum
    u32 sourceIP;
    u32 destIP;
    
    // options follow, use headerLen rather than sizeof(IPHeader)
};

/**
 * @see http://www.networksorcery.com/enp/protocol/icmp/msg8.htm
 */
struct ICMPHeader
{
    union {
        struct {
            ICMPType type;
            u8       code;
        };
        ICMPControlMessage message;
    };
    u16 checksum;
    u16 id;
    u16 seq;
};

#pragma pack()


/**
 * @see http://www.networksorcery.com/enp/protocol/icmp/msg8.htm
 * The 16-bit one's complement of the one's complement sum of the ICMP message, starting with the
 * ICMP Type field. When the checksum is computed, the checksum field should first be cleared to 0.
 * When the data packet is transmitted, the checksum is computed and inserted into this field.
 * When the data packet is received, the checksum is again computed and verified against the
 * checksum field. If the two checksums do not match then an error has occurred.
 */
u16
checksum(
    u16* data,
    u32 bytes)
{
    u32 sum = 0;

    // sum data as words
    while (bytes > 1) {
        sum += *data++;
        bytes -= sizeof(u16);
    }

    // if odd, add the last byte
    if (bytes == 1) {
        sum += *(u8*)data;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    
    return (u16)(~sum);
}


const char*
controlMessageString(
    u16 icmpTypeAndCode)
{
    for(s32 c = 0;
        c < countof(ControlMessageStrings);
        ++c)
    {
        if (ControlMessageStrings[c].cm == icmpTypeAndCode) {
            return ControlMessageStrings[c].str;
        }
    }
    return "Unknown control message";
}


#endif
