#ifndef _rtp_h_
#define _rtp_h_

#if __BYTE_ORDER == __LITTLE_ENDIAN

typedef struct
{
   unsigned int cc:4;        // CSRC count 
   unsigned int x:1;         // header extension flag
   unsigned int p:1;         // padding flag 
   unsigned int version:2;   // protocol version
   unsigned int pt:7;        // payload type 
   unsigned int m:1;         // marker bit 
   unsigned int seq:16;      // sequence number 
   unsigned int ts;          // timestamp 
   unsigned int ssrc;        // synchronization source 
} rtp_h;

#endif

#if __BYTE_ORDER == __BIG_ENDIAN

typedef struct
{

   unsigned int version:2;   // protocol version
   unsigned int p:1;         // padding flag 
   unsigned int x:1;         // header extension flag
   unsigned int cc:4;        // CSRC count 
   unsigned int m:1;         // marker bit 
   unsigned int pt:7;        // payload type 
   unsigned int seq:16;      // sequence number 
   unsigned int ts;          // timestamp 
   unsigned int ssrc;        // synchronization source 
} rtp_h;

#endif

#define RTPSIZE sizeof(rtp_h) 

#endif
