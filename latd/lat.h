/* lat.h
 *
 */

// Our LAT version (5.2)

#define LAT_VERSION     5
#define LAT_VERSION_ECO 2

// Name of the /dev/lat directory for local "ports"
#define LAT_DIRECTORY "/dev/lat"

/* Command types */
// NOTE: Bit 0 is "Response Requested"
//       Bit 1 is "To Host"
#define LAT_CCMD_SREPLY   0x00 // From Host
#define LAT_CCMD_SDATA    0x01 // From Host: Response required
#define LAT_CCMD_SESSION  0x02 // To Host
#define LAT_CCMD_CONNECT  0x06
#define LAT_CCMD_CONREF   0x08 // Disconnect from Host end
#define LAT_CCMD_CONACK   0x04
#define LAT_CCMD_DISCON   0x0A
#define LAT_CCMD_SERVICE  0x28
#define LAT_CCMD_COMMAND  0x30 // Used by Queued Connect
#define LAT_CCMD_STATUS   0x34 // Status of Queued Connect
#define LAT_CCMD_ENQUIRE  0x38
#define LAT_CCMD_ENQREPLY 0x3C

/* Structure of a LAT header */
typedef struct
{
    unsigned char  cmd             __attribute__ ((packed));            
    unsigned char  num_slots       __attribute__ ((packed));
    unsigned short remote_connid   __attribute__ ((packed));
    unsigned short local_connid    __attribute__ ((packed));
    unsigned char  sequence_number __attribute__ ((packed));
    unsigned char  ack_number      __attribute__ ((packed));

} LAT_Header;

// Service Announcement message
typedef struct
{
  unsigned char  cmd             __attribute__ ((packed)); // always 0x28
  unsigned char  circuit_timer   __attribute__ ((packed)); // in 10s milliseconds
  unsigned char  hiver           __attribute__ ((packed)); // Highest protocol version
  unsigned char  lover           __attribute__ ((packed)); // Lowest protocol version
  unsigned char  latver          __attribute__ ((packed)); // LAT version No. (5)
  unsigned char  latver_eco      __attribute__ ((packed)); // LAT version No. (LSB)
  unsigned char  incarnation     __attribute__ ((packed)); // Message incarnation
  unsigned char  flags           __attribute__ ((packed)); // Change flags
  unsigned short mtu             __attribute__ ((packed)); // 1500
  unsigned char  multicast_timer __attribute__ ((packed)); // Multicast timer (seconds)
  unsigned char  node_status     __attribute__ ((packed)); // 2 (accepting connections)
  unsigned char  group_length    __attribute__ ((packed)); 

  // Following:
  // Node groups
  //...other stuff!
} LAT_ServiceAnnounce;


// LAT Start message
typedef struct
{
    LAT_Header header;
    unsigned short maxsize     __attribute__ ((packed)); // Max message size
    unsigned char  latver      __attribute__ ((packed)); // LAT version No. (5)
    unsigned char  latver_eco  __attribute__ ((packed)); // LAT version No. (LSB)
    unsigned char  maxsessions __attribute__ ((packed));
    unsigned char  exqueued    __attribute__ ((packed)); // Extra data link buffer queued
    unsigned char  circtimer   __attribute__ ((packed)); // in 10s of milliseconds
    unsigned char  keepalive   __attribute__ ((packed)); // in seconds
    unsigned short facility    __attribute__ ((packed));
    unsigned char  prodtype    __attribute__ ((packed));
    unsigned char  prodver     __attribute__ ((packed));

  // Following:
    // ASCIC Destination Service
    // ASCIC Source Node
} LAT_Start;

// LAT Connect message
typedef struct
{
    LAT_Header header;
    unsigned short maxsize     __attribute__ ((packed));
    unsigned char  latver      __attribute__ ((packed)); // LAT version No. (5)
    unsigned char  latver_eco  __attribute__ ((packed)); // LAT version No. (LSB)
    unsigned char  maxsessions __attribute__ ((packed));
    unsigned char  exqueued    __attribute__ ((packed));
    unsigned char  circtimer   __attribute__ ((packed));
    unsigned char  keepalive   __attribute__ ((packed));
    unsigned short facility    __attribute__ ((packed));
    unsigned char  prodtype    __attribute__ ((packed));
    unsigned char  prodver     __attribute__ ((packed));

    // Following:
    // ASCIC Destination Node
    // ASCIC Source Node
    // ASCIC Local description (NUL terminated)
} LAT_StartResponse;

// LAT Slot command - follows the header - may be more than one
// in a packet. Word-aligned
typedef struct
{
    unsigned char local_session  __attribute__ ((packed));
    unsigned char remote_session __attribute__ ((packed));
    unsigned char length         __attribute__ ((packed));
    unsigned char cmd            __attribute__ ((packed));
} LAT_SlotCmd;


typedef struct
{
    LAT_Header  header  __attribute__ ((packed));
    LAT_SlotCmd slot    __attribute__ ((packed));
} LAT_SessionCmd;

typedef struct
{
    LAT_Header    header        __attribute__ ((packed));
    LAT_SlotCmd   slot          __attribute__ ((packed));
    unsigned char serviceclass  __attribute__ ((packed));
    unsigned char attslotsize   __attribute__ ((packed));
    unsigned char dataslotsize  __attribute__ ((packed));
} LAT_SessionStartCmd;

typedef struct
{
    unsigned char  cmd           __attribute__ ((packed));
    unsigned char  format        __attribute__ ((packed));
    unsigned char  hiver         __attribute__ ((packed)); // Highest protocol version
    unsigned char  lover         __attribute__ ((packed)); // Lowest protocol version
    unsigned char  latver        __attribute__ ((packed)); // LAT version No. (5)
    unsigned char  latver_eco    __attribute__ ((packed)); // LAT version No. (LSB)
    unsigned short maxsize       __attribute__ ((packed));
    unsigned short request_id    __attribute__ ((packed));
    unsigned short entry_id      __attribute__ ((packed));
    unsigned char  opcode        __attribute__ ((packed));
    unsigned char  modifier      __attribute__ ((packed));     
    // ASCIC Destination node name
    // ASCIC Source node name
    // ASCIC Source node port name (usually NUL)
    // ASCIC Source description (usually NUL)
    // ASCIC Destination port name
    // Parameters
} LAT_Command;

// A Status message can contain one or more of these entries.
typedef struct
{
    unsigned short length        __attribute__ ((packed));
    unsigned short status        __attribute__ ((packed));
    unsigned short request_id    __attribute__ ((packed));
    unsigned short session_id    __attribute__ ((packed));
    unsigned short elapsed_time  __attribute__ ((packed)); // set to -1? seconds
    unsigned short min_que_pos   __attribute__ ((packed));
    unsigned short max_que_pos   __attribute__ ((packed));
    // ASCIC Service Name (usually NUL)
    // ASCIC Port Name
    // ASCIC Service description
    
} LAT_StatusEntry;

typedef struct
{
    unsigned char  cmd           __attribute__ ((packed));
    unsigned char  format        __attribute__ ((packed));
    unsigned char  hiver         __attribute__ ((packed)); // Highest protocol version
    unsigned char  lover         __attribute__ ((packed)); // Lowest protocol version
    unsigned char  latver        __attribute__ ((packed)); // LAT version No. (5)
    unsigned char  latver_eco    __attribute__ ((packed)); // LAT version No. (LSB)
    unsigned short maxsize       __attribute__ ((packed));
    unsigned short retrans_timer __attribute__ ((packed));
    unsigned char  entry_count   __attribute__ ((packed));
    //ASCIC Subject node name
    // Array of LAT_StatusEntry structs

} LAT_Status;

typedef LAT_SessionCmd LAT_SessionReply;
typedef LAT_SessionCmd LAT_SessionData; // Unsolicited output

// Slot disconnect reasons (0xd(x) & 0xc(x) slot messages):
// 0x0 Unknown
// 0x1 User requested disconnect
// 0x2 System shutdown in progress
// 0x3 Invalid slot received
// 0x4 Invalid service class
// 0x5 Insufficient resources
// 0x6 Service in use
// 0x7 No such service
// 0x8 Service is disabled
// 0x9 Service is not offered by requested port
// 0xa Port name is unknown
// 0xb Invalid password
// 0xc Entry is not in the queue
// 0xd Immediate access rejected
// 0xe Access denied
// 0xf Corrupted solicit request


// Circuit disconnect reasons (first byte of 0x0A/0x08 message)
// 0x00 Unknown
// 0x01 No more slots on circuit
// 0x02 Illegal message or slot format received
// 0x03 VC_Halt from user
// 0x04 No progress being made
// 0x05 Time limit expired
// 0x06 Retransmission limit reached
// 0x07 Insufficient resources to satisfy request
// 0x08 Server circuit timer out of range
// 0x09 Number of virtual circuits exceeded


