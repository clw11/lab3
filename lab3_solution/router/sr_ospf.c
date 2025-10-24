/*-----------------------------------------------------------------------------
 * file:  sr_ospf.c
 * date:  2025
 * Author: OSPF Implementation
 *
 * Description:
 *
 * OSPF (Open Shortest Path First) protocol implementation
 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sr_ospf.h"
#include "sr_if.h"
#include "sr_rt.h"
#include "sr_utils.h"
#include "sr_router.h"
#include "sr_protocol.h"

/* Global OSPF instance pointer */
static struct sr_ospf_instance* g_ospf_instance = NULL;

/*---------------------------------------------------------------------
 * Method: sr_ospf_init()
 * @brief Initialize OSPF instance
 * @param sr: pointer to simple router state
 *---------------------------------------------------------------------*/
void sr_ospf_init(struct sr_instance* sr) {
    assert(sr);
    
    /* Allocate OSPF instance */
    g_ospf_instance = (struct sr_ospf_instance*)malloc(sizeof(struct sr_ospf_instance));
    assert(g_ospf_instance);
    memset(g_ospf_instance, 0, sizeof(struct sr_ospf_instance));
    
    /* Set router ID to the IP of the first interface */
    if (sr->if_list) {
        g_ospf_instance->router_id = sr->if_list->ip;
    }
    
    /* Initialize area ID (backbone area 0) */
    g_ospf_instance->area_id = 0;
    
    /* Initialize sequence number */
    g_ospf_instance->seq_num = 1;
    
    /* Initialize neighbors and LSDB */
    g_ospf_instance->neighbors = NULL;
    g_ospf_instance->lsdb = NULL;
    
    /* Initialize mutex */
    pthread_mutex_init(&(g_ospf_instance->ospf_lock), NULL);
    
    printf("OSPF initialized with Router ID: %s\n", 
           inet_ntoa(*(struct in_addr*)&g_ospf_instance->router_id));
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_destroy()
 * @brief Clean up OSPF instance
 * @param sr: pointer to simple router state
 *---------------------------------------------------------------------*/
void sr_ospf_destroy(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    pthread_mutex_lock(&(g_ospf_instance->ospf_lock));
    
    /* Free neighbors */
    struct sr_ospf_neighbor* neighbor = g_ospf_instance->neighbors;
    while (neighbor) {
        struct sr_ospf_neighbor* next = neighbor->next;
        free(neighbor);
        neighbor = next;
    }
    
    /* Free LSDB */
    struct sr_ospf_lsdb_entry* lsa = g_ospf_instance->lsdb;
    while (lsa) {
        struct sr_ospf_lsdb_entry* next = lsa->next;
        free(lsa);
        lsa = next;
    }
    
    pthread_mutex_unlock(&(g_ospf_instance->ospf_lock));
    pthread_mutex_destroy(&(g_ospf_instance->ospf_lock));
    
    free(g_ospf_instance);
    g_ospf_instance = NULL;
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_timeout()
 * @brief Main OSPF timeout thread
 * @param sr_ptr: pointer to simple router state
 *---------------------------------------------------------------------*/
void* sr_ospf_timeout(void* sr_ptr) {
    struct sr_instance* sr = sr_ptr;
    
    while (1) {
        sleep(OSPF_HELLO_INTERVAL);
        
        if (!g_ospf_instance) continue;
        
        pthread_mutex_lock(&(g_ospf_instance->ospf_lock));
        
        /* Send Hello packets */
        sr_ospf_send_hello(sr);
        
        /* Check neighbor timeouts */
        sr_ospf_check_neighbors(sr);
        
        /* Age LSAs in database */
        sr_ospf_age_lsdb(sr);
        
        /* Generate and flood LSAs */
        sr_ospf_generate_lsa(sr);
        sr_ospf_flood_lsa(sr);
        
        /* Run SPF and update routing table */
        sr_ospf_run_spf(sr);
        sr_ospf_update_routing_table(sr);
        
        /* Print routing table */
        sr_print_routing_table(sr);
        
        pthread_mutex_unlock(&(g_ospf_instance->ospf_lock));
    }
    
    return NULL;
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_send_hello()
 * @brief Send OSPF Hello packets on all interfaces
 * @param sr: pointer to simple router state
 *---------------------------------------------------------------------*/
void sr_ospf_send_hello(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    struct sr_if* interface = sr->if_list;
    
    /* Send Hello on each interface */
    while (interface != NULL) {
        unsigned int packet_len = sizeof(sr_ethernet_hdr_t) + 
                                  sizeof(sr_ip_hdr_t) + 
                                  sizeof(sr_ospf_hello_t);
        uint8_t* packet = (uint8_t*)malloc(packet_len);
        memset(packet, 0, packet_len);
        
        /* Ethernet header */
        sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*)packet;
        memcpy(eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
        memset(eth_hdr->ether_dhost, 0xff, ETHER_ADDR_LEN); /* Broadcast */
        eth_hdr->ether_type = htons(ethertype_ip);
        
        /* IP header */
        sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
        ip_hdr->ip_hl = 5;
        ip_hdr->ip_v = 4;
        ip_hdr->ip_tos = 0;
        ip_hdr->ip_len = htons(sizeof(sr_ip_hdr_t) + sizeof(sr_ospf_hello_t));
        ip_hdr->ip_id = 0;
        ip_hdr->ip_off = 0;
        ip_hdr->ip_ttl = 1;
        ip_hdr->ip_p = ip_protocol_ospf;
        ip_hdr->ip_sum = 0;
        ip_hdr->ip_src = interface->ip;
        ip_hdr->ip_dst = htonl(0xe0000005); /* AllSPFRouters multicast 224.0.0.5 */
        ip_hdr->ip_sum = cksum(ip_hdr, sizeof(sr_ip_hdr_t));
        
        /* OSPF Hello packet */
        sr_ospf_hello_t* hello = (sr_ospf_hello_t*)(packet + sizeof(sr_ethernet_hdr_t) + 
                                                     sizeof(sr_ip_hdr_t));
        hello->header.version = 2;
        hello->header.type = OSPF_TYPE_HELLO;
        hello->header.len = htons(sizeof(sr_ospf_hello_t));
        hello->header.router_id = g_ospf_instance->router_id;
        hello->header.area_id = g_ospf_instance->area_id;
        hello->header.autype = 0;
        hello->header.authentication = 0;
        
        hello->network_mask = interface->mask;
        hello->hello_interval = htons(OSPF_HELLO_INTERVAL);
        hello->options = 0;
        hello->priority = 1;
        hello->dead_interval = htonl(OSPF_DEAD_INTERVAL);
        hello->designated_router = 0;
        hello->backup_router = 0;
        hello->neighbor = 0;
        
        /* Calculate OSPF checksum */
        hello->header.checksum = 0;
        hello->header.checksum = sr_ospf_checksum(hello, sizeof(sr_ospf_hello_t));
        
        /* Send packet */
        sr_send_packet(sr, packet, packet_len, interface->name);
        free(packet);
        
        interface = interface->next;
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_handle_hello()
 * @brief Handle received OSPF Hello packet
 * @param sr: pointer to simple router state
 * @param packet: received packet
 * @param len: packet length
 * @param interface: receiving interface
 *---------------------------------------------------------------------*/
void sr_ospf_handle_hello(struct sr_instance* sr, uint8_t* packet,
                          unsigned int len, char* interface) {
    if (!g_ospf_instance) return;
    
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
    sr_ospf_hello_t* hello = (sr_ospf_hello_t*)(packet + sizeof(sr_ethernet_hdr_t) + 
                                                 sizeof(sr_ip_hdr_t));
    
    uint32_t neighbor_router_id = hello->header.router_id;
    uint32_t neighbor_ip = ip_hdr->ip_src;
    
    pthread_mutex_lock(&(g_ospf_instance->ospf_lock));
    
    /* Find or add neighbor */
    struct sr_ospf_neighbor* neighbor = sr_ospf_find_neighbor(sr, neighbor_router_id);
    
    if (neighbor) {
        /* Update existing neighbor */
        neighbor->last_hello = time(NULL);
        neighbor->alive = 1;
    } else {
        /* Add new neighbor */
        sr_ospf_add_neighbor(sr, neighbor_router_id, neighbor_ip, interface);
    }
    
    pthread_mutex_unlock(&(g_ospf_instance->ospf_lock));
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_add_neighbor()
 * @brief Add a new neighbor to the neighbor table
 *---------------------------------------------------------------------*/
void sr_ospf_add_neighbor(struct sr_instance* sr, uint32_t router_id,
                          uint32_t ip_addr, char* interface) {
    if (!g_ospf_instance) return;
    
    struct sr_ospf_neighbor* neighbor = (struct sr_ospf_neighbor*)malloc(
        sizeof(struct sr_ospf_neighbor));
    assert(neighbor);
    
    neighbor->router_id = router_id;
    neighbor->ip_addr = ip_addr;
    strncpy(neighbor->interface, interface, sr_IFACE_NAMELEN);
    neighbor->last_hello = time(NULL);
    neighbor->alive = 1;
    
    /* Add to front of list */
    neighbor->next = g_ospf_instance->neighbors;
    g_ospf_instance->neighbors = neighbor;
    
    printf("OSPF: Added neighbor %s on interface %s\n",
           inet_ntoa(*(struct in_addr*)&router_id), interface);
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_find_neighbor()
 * @brief Find a neighbor by router ID
 *---------------------------------------------------------------------*/
struct sr_ospf_neighbor* sr_ospf_find_neighbor(struct sr_instance* sr,
                                                uint32_t router_id) {
    if (!g_ospf_instance) return NULL;
    
    struct sr_ospf_neighbor* neighbor = g_ospf_instance->neighbors;
    while (neighbor) {
        if (neighbor->router_id == router_id) {
            return neighbor;
        }
        neighbor = neighbor->next;
    }
    
    return NULL;
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_check_neighbors()
 * @brief Check for neighbor timeouts
 *---------------------------------------------------------------------*/
void sr_ospf_check_neighbors(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    time_t now = time(NULL);
    struct sr_ospf_neighbor* neighbor = g_ospf_instance->neighbors;
    
    while (neighbor) {
        if (neighbor->alive && 
            difftime(now, neighbor->last_hello) > OSPF_DEAD_INTERVAL) {
            neighbor->alive = 0;
            printf("OSPF: Neighbor %s timed out\n",
                   inet_ntoa(*(struct in_addr*)&neighbor->router_id));
        }
        neighbor = neighbor->next;
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_generate_lsa()
 * @brief Generate LSAs for directly connected networks
 *---------------------------------------------------------------------*/
void sr_ospf_generate_lsa(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    struct sr_if* interface = sr->if_list;
    
    /* Generate LSA for each interface */
    while (interface != NULL) {
        /* Check if interface is up */
        if (sr_obtain_interface_status(sr, interface->name) != 0) {
            struct sr_ospf_lsa lsa;
            memset(&lsa, 0, sizeof(struct sr_ospf_lsa));
            
            lsa.router_id = g_ospf_instance->router_id;
            lsa.subnet = interface->ip & interface->mask;
            lsa.mask = interface->mask;
            lsa.seq = g_ospf_instance->seq_num++;
            lsa.age = 0;
            lsa.num_links = 0;
            
            /* Update or add to LSDB */
            sr_ospf_update_lsdb(sr, &lsa);
        }
        
        interface = interface->next;
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_flood_lsa()
 * @brief Flood LSAs to all neighbors
 *---------------------------------------------------------------------*/
void sr_ospf_flood_lsa(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    struct sr_if* interface = sr->if_list;
    
    /* Flood on each interface */
    while (interface != NULL) {
        unsigned int packet_len = sizeof(sr_ethernet_hdr_t) + 
                                  sizeof(sr_ip_hdr_t) + 
                                  sizeof(sr_ospf_lsu_t);
        uint8_t* packet = (uint8_t*)malloc(packet_len);
        memset(packet, 0, packet_len);
        
        /* Ethernet header */
        sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*)packet;
        memcpy(eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
        memset(eth_hdr->ether_dhost, 0xff, ETHER_ADDR_LEN);
        eth_hdr->ether_type = htons(ethertype_ip);
        
        /* IP header */
        sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
        ip_hdr->ip_hl = 5;
        ip_hdr->ip_v = 4;
        ip_hdr->ip_tos = 0;
        ip_hdr->ip_len = htons(sizeof(sr_ip_hdr_t) + sizeof(sr_ospf_lsu_t));
        ip_hdr->ip_id = 0;
        ip_hdr->ip_off = 0;
        ip_hdr->ip_ttl = 1;
        ip_hdr->ip_p = ip_protocol_ospf;
        ip_hdr->ip_sum = 0;
        ip_hdr->ip_src = interface->ip;
        ip_hdr->ip_dst = htonl(0xe0000005); /* AllSPFRouters */
        ip_hdr->ip_sum = cksum(ip_hdr, sizeof(sr_ip_hdr_t));
        
        /* OSPF LSU packet */
        sr_ospf_lsu_t* lsu = (sr_ospf_lsu_t*)(packet + sizeof(sr_ethernet_hdr_t) + 
                                              sizeof(sr_ip_hdr_t));
        lsu->header.version = 2;
        lsu->header.type = OSPF_TYPE_LSU;
        lsu->header.len = htons(sizeof(sr_ospf_lsu_t));
        lsu->header.router_id = g_ospf_instance->router_id;
        lsu->header.area_id = g_ospf_instance->area_id;
        lsu->header.autype = 0;
        lsu->header.authentication = 0;
        
        /* Copy LSAs from LSDB */
        int num_lsas = 0;
        struct sr_ospf_lsdb_entry* lsdb_entry = g_ospf_instance->lsdb;
        while (lsdb_entry && num_lsas < MAX_NUM_ENTRIES) {
            lsu->lsas[num_lsas].router_id = lsdb_entry->router_id;
            lsu->lsas[num_lsas].subnet = lsdb_entry->subnet;
            lsu->lsas[num_lsas].mask = lsdb_entry->mask;
            lsu->lsas[num_lsas].seq = lsdb_entry->seq;
            lsu->lsas[num_lsas].age = lsdb_entry->age;
            lsu->lsas[num_lsas].num_links = 0;
            
            num_lsas++;
            lsdb_entry = lsdb_entry->next;
        }
        lsu->num_lsas = htonl(num_lsas);
        
        /* Calculate checksum */
        lsu->header.checksum = 0;
        lsu->header.checksum = sr_ospf_checksum(lsu, sizeof(sr_ospf_lsu_t));
        
        /* Send packet */
        sr_send_packet(sr, packet, packet_len, interface->name);
        free(packet);
        
        interface = interface->next;
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_handle_lsu()
 * @brief Handle received OSPF Link State Update
 *---------------------------------------------------------------------*/
void sr_ospf_handle_lsu(struct sr_instance* sr, uint8_t* packet,
                        unsigned int len, char* interface) {
    if (!g_ospf_instance) return;
    
    sr_ospf_lsu_t* lsu = (sr_ospf_lsu_t*)(packet + sizeof(sr_ethernet_hdr_t) + 
                                          sizeof(sr_ip_hdr_t));
    
    pthread_mutex_lock(&(g_ospf_instance->ospf_lock));
    
    uint32_t num_lsas = ntohl(lsu->num_lsas);
    uint32_t i;
    
    /* Process each LSA */
    for (i = 0; i < num_lsas && i < MAX_NUM_ENTRIES; i++) {
        sr_ospf_update_lsdb(sr, &lsu->lsas[i]);
    }
    
    pthread_mutex_unlock(&(g_ospf_instance->ospf_lock));
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_update_lsdb()
 * @brief Update Link State Database with new LSA
 *---------------------------------------------------------------------*/
void sr_ospf_update_lsdb(struct sr_instance* sr, struct sr_ospf_lsa* lsa) {
    if (!g_ospf_instance) return;
    
    /* Find existing LSA */
    struct sr_ospf_lsdb_entry* existing = sr_ospf_find_lsa(sr, lsa->router_id, 
                                                           lsa->subnet);
    
    if (existing) {
        /* Update if newer sequence number */
        if (lsa->seq > existing->seq) {
            existing->seq = lsa->seq;
            existing->mask = lsa->mask;
            existing->timestamp = time(NULL);
            existing->age = lsa->age;
        }
    } else {
        /* Add new LSA */
        struct sr_ospf_lsdb_entry* new_entry = (struct sr_ospf_lsdb_entry*)malloc(
            sizeof(struct sr_ospf_lsdb_entry));
        assert(new_entry);
        
        new_entry->router_id = lsa->router_id;
        new_entry->subnet = lsa->subnet;
        new_entry->mask = lsa->mask;
        new_entry->seq = lsa->seq;
        new_entry->timestamp = time(NULL);
        new_entry->age = lsa->age;
        
        /* Add to front of list */
        new_entry->next = g_ospf_instance->lsdb;
        g_ospf_instance->lsdb = new_entry;
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_find_lsa()
 * @brief Find LSA in database
 *---------------------------------------------------------------------*/
struct sr_ospf_lsdb_entry* sr_ospf_find_lsa(struct sr_instance* sr,
                                             uint32_t router_id, uint32_t subnet) {
    if (!g_ospf_instance) return NULL;
    
    struct sr_ospf_lsdb_entry* lsa = g_ospf_instance->lsdb;
    while (lsa) {
        if (lsa->router_id == router_id && lsa->subnet == subnet) {
            return lsa;
        }
        lsa = lsa->next;
    }
    
    return NULL;
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_age_lsdb()
 * @brief Age and remove expired LSAs
 *---------------------------------------------------------------------*/
void sr_ospf_age_lsdb(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    time_t now = time(NULL);
    struct sr_ospf_lsdb_entry* lsa = g_ospf_instance->lsdb;
    struct sr_ospf_lsdb_entry* prev = NULL;
    
    while (lsa) {
        lsa->age = (uint16_t)difftime(now, lsa->timestamp);
        
        /* Remove expired LSAs */
        if (lsa->age >= OSPF_LSA_MAXAGE) {
            struct sr_ospf_lsdb_entry* next = lsa->next;
            
            if (prev) {
                prev->next = next;
            } else {
                g_ospf_instance->lsdb = next;
            }
            
            free(lsa);
            lsa = next;
        } else {
            prev = lsa;
            lsa = lsa->next;
        }
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_run_spf()
 * @brief Run Dijkstra's algorithm to compute shortest paths
 *---------------------------------------------------------------------*/
void sr_ospf_run_spf(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    /* This is a simplified SPF implementation
     * In a full implementation, you would build a graph from the LSDB
     * and run Dijkstra's algorithm
     * For this basic implementation, we directly use LSAs to update routes
     */
    
    /* The routing table will be updated in sr_ospf_update_routing_table() */
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_update_routing_table()
 * @brief Update routing table based on SPF results
 *---------------------------------------------------------------------*/
void sr_ospf_update_routing_table(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    pthread_mutex_lock(&(sr->rt_locker));
    
    /* First, add directly connected networks */
    struct sr_if* interface = sr->if_list;
    while (interface != NULL) {
        if (sr_obtain_interface_status(sr, interface->name) != 0) {
            struct in_addr dest, gw, mask;
            dest.s_addr = interface->ip & interface->mask;
            gw.s_addr = 0;
            mask.s_addr = interface->mask;
            
            /* Check if route already exists */
            struct sr_rt* rt_walker = sr->routing_table;
            bool found = false;
            while (rt_walker) {
                if (rt_walker->dest.s_addr == dest.s_addr &&
                    rt_walker->mask.s_addr == mask.s_addr) {
                    /* Update existing route */
                    rt_walker->metric = 0;
                    rt_walker->gw.s_addr = 0;
                    rt_walker->updated_time = time(NULL);
                    strcpy(rt_walker->interface, interface->name);
                    found = true;
                    break;
                }
                rt_walker = rt_walker->next;
            }
            
            if (!found) {
                sr_add_rt_entry(sr, dest, gw, mask, 0, interface->name);
            }
        }
        interface = interface->next;
    }
    
    /* Add routes learned from LSAs */
    struct sr_ospf_lsdb_entry* lsa = g_ospf_instance->lsdb;
    while (lsa) {
        /* Skip our own LSAs */
        if (lsa->router_id != g_ospf_instance->router_id) {
            /* Find neighbor that advertised this LSA */
            struct sr_ospf_neighbor* neighbor = sr_ospf_find_neighbor(sr, lsa->router_id);
            
            if (neighbor && neighbor->alive) {
                struct in_addr dest, gw, mask;
                dest.s_addr = lsa->subnet;
                gw.s_addr = neighbor->ip_addr;
                mask.s_addr = lsa->mask;
                
                /* Check if route exists */
                struct sr_rt* rt_walker = sr->routing_table;
                bool found = false;
                while (rt_walker) {
                    if (rt_walker->dest.s_addr == dest.s_addr &&
                        rt_walker->mask.s_addr == mask.s_addr) {
                        /* Update if metric is better (1 hop through neighbor) */
                        if (1 < rt_walker->metric || rt_walker->metric == 0) {
                            rt_walker->metric = 1;
                            rt_walker->gw.s_addr = gw.s_addr;
                            rt_walker->updated_time = time(NULL);
                            strcpy(rt_walker->interface, neighbor->interface);
                        }
                        found = true;
                        break;
                    }
                    rt_walker = rt_walker->next;
                }
                
                if (!found) {
                    sr_add_rt_entry(sr, dest, gw, mask, 1, neighbor->interface);
                }
            }
        }
        lsa = lsa->next;
    }
    
    /* Remove invalid routes */
    struct sr_rt* rt_walker = sr->routing_table;
    while (rt_walker) {
        /* Check if the route is still valid */
        if (rt_walker->metric > 0) {
            /* Check if we still have connectivity to the gateway */
            bool valid = false;
            struct sr_ospf_neighbor* neighbor = g_ospf_instance->neighbors;
            while (neighbor) {
                if (neighbor->alive && neighbor->ip_addr == rt_walker->gw.s_addr) {
                    valid = true;
                    break;
                }
                neighbor = neighbor->next;
            }
            
            if (!valid) {
                rt_walker->metric = INFINITY;
            }
        }
        rt_walker = rt_walker->next;
    }
    
    pthread_mutex_unlock(&(sr->rt_locker));
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_checksum()
 * @brief Calculate OSPF checksum (similar to IP checksum)
 *---------------------------------------------------------------------*/
uint16_t sr_ospf_checksum(void* data, int len) {
    return cksum(data, len);
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_print_neighbors()
 * @brief Print neighbor table
 *---------------------------------------------------------------------*/
void sr_ospf_print_neighbors(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    printf("  <---------- OSPF Neighbors ---------->\n");
    printf("Router ID\tIP Address\tInterface\tState\n");
    
    struct sr_ospf_neighbor* neighbor = g_ospf_instance->neighbors;
    while (neighbor) {
        printf("%s\t",inet_ntoa(*(struct in_addr*)&neighbor->router_id));
        printf("%s\t",inet_ntoa(*(struct in_addr*)&neighbor->ip_addr));
        printf("%s\t", neighbor->interface);
        printf("%s\n", neighbor->alive ? "FULL" : "DOWN");
        neighbor = neighbor->next;
    }
}

/*---------------------------------------------------------------------
 * Method: sr_ospf_print_lsdb()
 * @brief Print Link State Database
 *---------------------------------------------------------------------*/
void sr_ospf_print_lsdb(struct sr_instance* sr) {
    if (!g_ospf_instance) return;
    
    printf("  <---------- OSPF LSDB ---------->\n");
    printf("Router ID\tSubnet\t\tMask\t\tSeq\tAge\n");
    
    struct sr_ospf_lsdb_entry* lsa = g_ospf_instance->lsdb;
    while (lsa) {
        printf("%s\t",inet_ntoa(*(struct in_addr*)&lsa->router_id));
        printf("%s\t",inet_ntoa(*(struct in_addr*)&lsa->subnet));
        printf("%s\t",inet_ntoa(*(struct in_addr*)&lsa->mask));
        printf("%u\t", lsa->seq);
        printf("%u\n", lsa->age);
        lsa = lsa->next;
    }
}
