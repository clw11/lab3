/*
 *  Copyright (c) 1998, 1999, 2000 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * sr_protocol.h
 *
 */

#ifndef SR_PROTOCOL_H
#define SR_PROTOCOL_H

#ifdef _LINUX_
#include <stdint.h>
#endif /* _LINUX_ */

#include <sys/types.h>
#include <arpa/inet.h>


#ifndef IP_MAXPACKET
#define IP_MAXPACKET 65535
#endif

#ifndef MAX_NUM_ENTRIES
#define MAX_NUM_ENTRIES 25
#endif

/* FIXME
 * ohh how lame .. how very, very lame... how can I ever go out in public
 * again?! /mc
 */

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 2
#endif

#ifndef __BYTE_ORDER
  #ifdef _CYGWIN_
  #define __BYTE_ORDER __LITTLE_ENDIAN
  #endif
  #ifdef _LINUX_
  #define __BYTE_ORDER __LITTLE_ENDIAN
  #endif
  #ifdef _SOLARIS_
  #define __BYTE_ORDER __BIG_ENDIAN
  #endif
  #ifdef _DARWIN_
  #define __BYTE_ORDER __BIG_ENDIAN
  #endif
#endif
#define ICMP_DATA_SIZE 28

struct sr_rip_pkt {
  uint8_t command;
  uint8_t version;
  uint16_t unused;
  struct entry{
      uint16_t afi; /* Address Family Identifier */
      uint16_t tag; /*Route Tag */
      uint32_t address; /* IP Address */
      uint32_t mask; /* Subnet Mask */
      uint32_t next_hop; /* Next Hop */
      uint32_t metric; /* Metric */
    } entries[MAX_NUM_ENTRIES]; 
} __attribute__ ((packed)) ;
typedef struct sr_rip_pkt sr_rip_pkt_t;

/* OSPF Protocol Structures */
#define OSPF_HELLO_INTERVAL 5      /* Send Hello every 5 seconds */
#define OSPF_DEAD_INTERVAL 20      /* Neighbor timeout after 20 seconds */
#define OSPF_LSA_REFRESH 5         /* Refresh LSAs every 5 seconds */
#define OSPF_LSA_MAXAGE 20         /* LSA expires after 20 seconds */
#define OSPF_MAX_NEIGHBORS 10      /* Maximum number of neighbors */
#define OSPF_MAX_LSA 50            /* Maximum LSAs in database */

/* OSPF packet types */
#define OSPF_TYPE_HELLO 1
#define OSPF_TYPE_LSU   4          /* Link State Update */

/* OSPF Common Header */
struct sr_ospf_hdr {
  uint8_t version;                 /* OSPF version, should be 2 */
  uint8_t type;                    /* OSPF packet type */
  uint16_t len;                    /* Packet length including header */
  uint32_t router_id;              /* Router ID */
  uint32_t area_id;                /* Area ID, use 0 for backbone */
  uint16_t checksum;               /* OSPF checksum */
  uint16_t autype;                 /* Authentication type, use 0 for none */
  uint64_t authentication;         /* Authentication data */
} __attribute__ ((packed));
typedef struct sr_ospf_hdr sr_ospf_hdr_t;

/* OSPF Hello Packet */
struct sr_ospf_hello {
  struct sr_ospf_hdr header;
  uint32_t network_mask;           /* Network mask */
  uint16_t hello_interval;         /* Hello interval in seconds */
  uint8_t options;                 /* OSPF options */
  uint8_t priority;                /* Router priority */
  uint32_t dead_interval;          /* Router dead interval */
  uint32_t designated_router;      /* Designated router */
  uint32_t backup_router;          /* Backup designated router */
  uint32_t neighbor;               /* Neighbor router ID (only one for simplicity) */
} __attribute__ ((packed));
typedef struct sr_ospf_hello sr_ospf_hello_t;

/* OSPF Link State Advertisement */
struct sr_ospf_lsa {
  uint32_t router_id;              /* Advertising router */
  uint32_t subnet;                 /* Network subnet */
  uint32_t mask;                   /* Network mask */
  uint32_t seq;                    /* Sequence number */
  uint16_t age;                    /* LSA age in seconds */
  uint16_t num_links;              /* Number of links */
} __attribute__ ((packed));
typedef struct sr_ospf_lsa sr_ospf_lsa_t;

/* OSPF Link in LSA */
struct sr_ospf_link {
  uint32_t subnet;                 /* Link subnet */
  uint32_t mask;                   /* Link mask */
  uint32_t router_id;              /* Connected router ID */
} __attribute__ ((packed));
typedef struct sr_ospf_link sr_ospf_link_t;

/* OSPF Link State Update Packet */
struct sr_ospf_lsu {
  struct sr_ospf_hdr header;
  uint32_t num_lsas;               /* Number of LSAs */
  struct sr_ospf_lsa lsas[MAX_NUM_ENTRIES];  /* LSA entries */
} __attribute__ ((packed));
typedef struct sr_ospf_lsu sr_ospf_lsu_t;

struct sr_udp_hdr {
  uint16_t port_src, port_dst; /* source and dest port_number */
  uint16_t udp_len;			/* total length */
  uint16_t udp_sum;			/* checksum */
} __attribute__ ((packed)) ;
typedef struct sr_udp_hdr sr_udp_hdr_t;


/* Structure of a ICMP header
 */
struct sr_icmp_hdr {
  uint8_t icmp_type;
  uint8_t icmp_code;
  uint16_t icmp_sum;
  
} __attribute__ ((packed)) ;
typedef struct sr_icmp_hdr sr_icmp_hdr_t;

struct sr_icmp_t8_hdr {
  uint8_t icmp_type;
  uint8_t icmp_code;
  uint16_t icmp_sum;
  uint32_t unused;
  
} __attribute__ ((packed)) ;
typedef struct sr_icmp_t8_hdr sr_icmp_t8_hdr_t;

/* Structure of a type3 ICMP header
 */
struct sr_icmp_t3_hdr {
  uint8_t icmp_type;
  uint8_t icmp_code;
  uint16_t icmp_sum;
  uint16_t unused;
  uint16_t next_mtu;
  uint8_t data[ICMP_DATA_SIZE];

} __attribute__ ((packed)) ;
typedef struct sr_icmp_t3_hdr sr_icmp_t3_hdr_t;

/* Structure of a type11 ICMP header
 */
struct sr_icmp_t11_hdr {
  uint8_t icmp_type;
  uint8_t icmp_code;
  uint16_t icmp_sum;
  uint32_t unused;
  uint8_t data[ICMP_DATA_SIZE];

} __attribute__ ((packed)) ;
typedef struct sr_icmp_t11_hdr sr_icmp_t11_hdr_t;



/*
 * Structure of an internet header, naked of options.
 */
struct sr_ip_hdr
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ip_hl:4;		/* header length */
    unsigned int ip_v:4;		/* version */
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4;		/* version */
    unsigned int ip_hl:4;		/* header length */
#else
#error "Byte ordering ot specified " 
#endif 
    uint8_t ip_tos;			/* type of service */
    uint16_t ip_len;			/* total length */
    uint16_t ip_id;			/* identification */
    uint16_t ip_off;			/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
    uint8_t ip_ttl;			/* time to live */
    uint8_t ip_p;			/* protocol */
    uint16_t ip_sum;			/* checksum */
    uint32_t ip_src, ip_dst;	/* source and dest address */
  } __attribute__ ((packed)) ;
typedef struct sr_ip_hdr sr_ip_hdr_t;

/* 
 *  Ethernet packet header prototype.  Too many O/S's define this differently.
 *  Easy enough to solve that and define it here.
 */
struct sr_ethernet_hdr
{
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif
    uint8_t  ether_dhost[ETHER_ADDR_LEN];    /* destination ethernet address */
    uint8_t  ether_shost[ETHER_ADDR_LEN];    /* source ethernet address */
    uint16_t ether_type;                     /* packet type ID */
} __attribute__ ((packed)) ;
typedef struct sr_ethernet_hdr sr_ethernet_hdr_t;



enum sr_ip_protocol {
  ip_protocol_icmp = 0x0001,
  ip_protocol_udp = 0x0011,
  ip_protocol_ospf = 0x0059,
};

enum sr_ethertype {
  ethertype_arp = 0x0806,
  ethertype_ip = 0x0800,
};


enum sr_arp_opcode {
  arp_op_request = 0x0001,
  arp_op_reply = 0x0002,
};

enum sr_arp_hrd_fmt {
  arp_hrd_ethernet = 0x0001,
};


struct sr_arp_hdr
{
    unsigned short  ar_hrd;             /* format of hardware address   */
    unsigned short  ar_pro;             /* format of protocol address   */
    unsigned char   ar_hln;             /* length of hardware address   */
    unsigned char   ar_pln;             /* length of protocol address   */
    unsigned short  ar_op;              /* ARP opcode (command)         */
    unsigned char   ar_sha[ETHER_ADDR_LEN];   /* sender hardware address      */
    uint32_t        ar_sip;             /* sender IP address            */
    unsigned char   ar_tha[ETHER_ADDR_LEN];   /* target hardware address      */
    uint32_t        ar_tip;             /* target IP address            */
} __attribute__ ((packed)) ;
typedef struct sr_arp_hdr sr_arp_hdr_t;

#define sr_IFACE_NAMELEN 32

#endif /* -- SR_PROTOCOL_H -- */