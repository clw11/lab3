/*-----------------------------------------------------------------------------
 * file:  sr_ospf.h 
 * date:  2025
 * Author: OSPF Implementation
 *
 * Description:
 *
 * OSPF (Open Shortest Path First) protocol implementation
 * Handles neighbor discovery, LSA flooding, and shortest path calculation
 *
 *---------------------------------------------------------------------------*/

#ifndef SR_OSPF_H
#define SR_OSPF_H

#ifdef _DARWIN_
#include <sys/types.h>
#endif

#include <netinet/in.h>
#include <time.h>

#include "sr_if.h"
#include "sr_protocol.h"
#include "sr_router.h"

/* OSPF Neighbor State */
struct sr_ospf_neighbor {
    uint32_t router_id;              /* Neighbor router ID */
    uint32_t ip_addr;                /* Neighbor IP address */
    char interface[sr_IFACE_NAMELEN]; /* Interface to neighbor */
    time_t last_hello;               /* Time of last Hello received */
    uint8_t alive;                   /* Is neighbor alive */
    struct sr_ospf_neighbor* next;
};

/* OSPF Link State Database Entry */
struct sr_ospf_lsdb_entry {
    uint32_t router_id;              /* Router that advertised this LSA */
    uint32_t subnet;                 /* Network subnet */
    uint32_t mask;                   /* Network mask */
    uint32_t seq;                    /* Sequence number */
    time_t timestamp;                /* Time when LSA was received */
    uint16_t age;                    /* LSA age */
    struct sr_ospf_lsdb_entry* next;
};

/* OSPF Instance Data */
struct sr_ospf_instance {
    uint32_t router_id;              /* This router's ID */
    uint32_t area_id;                /* Area ID (use 0 for backbone) */
    uint32_t seq_num;                /* Sequence number for LSAs */
    struct sr_ospf_neighbor* neighbors;    /* Neighbor table */
    struct sr_ospf_lsdb_entry* lsdb;       /* Link State Database */
    pthread_mutex_t ospf_lock;       /* Lock for OSPF data */
};

/* OSPF Function Declarations */

/* Initialization and cleanup */
void sr_ospf_init(struct sr_instance* sr);
void sr_ospf_destroy(struct sr_instance* sr);

/* Main OSPF timeout thread */
void* sr_ospf_timeout(void* sr_ptr);

/* Hello packet handling */
void sr_ospf_send_hello(struct sr_instance* sr);
void sr_ospf_handle_hello(struct sr_instance* sr, uint8_t* packet, 
                          unsigned int len, char* interface);

/* LSA handling */
void sr_ospf_generate_lsa(struct sr_instance* sr);
void sr_ospf_flood_lsa(struct sr_instance* sr);
void sr_ospf_handle_lsu(struct sr_instance* sr, uint8_t* packet,
                        unsigned int len, char* interface);

/* Neighbor management */
void sr_ospf_add_neighbor(struct sr_instance* sr, uint32_t router_id,
                          uint32_t ip_addr, char* interface);
void sr_ospf_update_neighbor(struct sr_instance* sr, uint32_t router_id);
void sr_ospf_check_neighbors(struct sr_instance* sr);
struct sr_ospf_neighbor* sr_ospf_find_neighbor(struct sr_instance* sr,
                                                uint32_t router_id);

/* LSDB management */
void sr_ospf_add_lsa(struct sr_instance* sr, struct sr_ospf_lsa* lsa);
void sr_ospf_update_lsdb(struct sr_instance* sr, struct sr_ospf_lsa* lsa);
void sr_ospf_age_lsdb(struct sr_instance* sr);
struct sr_ospf_lsdb_entry* sr_ospf_find_lsa(struct sr_instance* sr,
                                             uint32_t router_id, uint32_t subnet);

/* Dijkstra's algorithm and routing table update */
void sr_ospf_run_spf(struct sr_instance* sr);
void sr_ospf_update_routing_table(struct sr_instance* sr);

/* Utility functions */
uint16_t sr_ospf_checksum(void* data, int len);
void sr_ospf_print_neighbors(struct sr_instance* sr);
void sr_ospf_print_lsdb(struct sr_instance* sr);

#endif /* SR_OSPF_H */
