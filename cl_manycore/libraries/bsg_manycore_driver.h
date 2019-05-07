#ifndef BSG_MANYCORE_DRIVER_H
#define BSG_MANYCORE_DRIVER_H

#ifndef COSIM
        #include <bsg_manycore_features.h>
        #include <bsg_manycore_errno.h>
	#include <bsg_manycore_bits.h>
#else
        #include "bsg_manycore_features.h"
        #include "bsg_manycore_errno.h"
	#include "bsg_manycore_bits.h"
#endif
#include <endian.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COSIM
static char *hb_mc_mmap_ocl (uint8_t fd);
#endif

/* hb_mc_fifo_rx_t identifies the type of a packet receive operation*/
typedef enum __hb_mc_direction_t {  /*  */
	HB_MC_MMIO_FIFO_MIN = 0,
	HB_MC_MMIO_FIFO_TO_DEVICE = 0,
	HB_MC_MMIO_FIFO_TO_HOST = 1,
	HB_MC_MMIO_FIFO_MAX = 1
} hb_mc_direction_t;

/* 
 * hb_mc_fifo_rx_t identifies the type of a packet receive operation. MIN & MAX
 * are not fifo directions, they are used for iteration over fifos. 
 */
typedef enum __hb_mc_fifo_rx_t {  
	HB_MC_FIFO_RX_RSP = HB_MC_MMIO_FIFO_TO_DEVICE,
	HB_MC_FIFO_RX_REQ = HB_MC_MMIO_FIFO_TO_HOST
} hb_mc_fifo_rx_t;


/**
 * The raw request packets that are used to initiate communications between the manycore and the host.
 * You should not read from any of the fields directly, instead use the accessor functions.
 */

typedef enum __hb_mc_packet_op_t {
	HB_MC_PACKET_OP_REMOTE_LOAD = 0,
	HB_MC_PACKET_OP_REMOTE_STORE = 1
} hb_mc_packet_op_t;

typedef enum __hb_mc_packet_mask_t {
	HB_MC_PACKET_REQUEST_MASK_BYTE = 0x1,
	HB_MC_PACKET_REQUEST_MASK_SHORT = 0x3,
	HB_MC_PACKET_REQUEST_MASK_WORD = 0xF
} hb_mc_packet_mask_t;

typedef struct request_packet {
        uint8_t  x_dst; //!< x coordinate of the responder
        uint8_t  y_dst; //!< y coordinate of the responder
        uint8_t  x_src; //!< x coordinate of the requester
        uint8_t  y_src; //!< y coordinate of the requester
        uint32_t data;  //!< packet's payload data
        uint8_t  mask;  //!< 4-bit byte mask 
        uint8_t  op;    //!< opcode 
        uint32_t addr;  //!< address field (EPA)       
        uint8_t  reserved[2];
}  __attribute__((packed)) hb_mc_request_packet_t;
        
/* 
 * * The raw response packets that are used to respond to requests.  You should not read from any of the fields directly, instead use the accessor functions.
 */
typedef struct response_packet {
        uint8_t  x_dst; //!< x coordinate of the requester
        uint8_t  y_dst; //!< y coordinate of the requester
        uint32_t  load_id; //!< unused 
        uint32_t data; //!< packet's payload data
        uint8_t  op;    //!< opcode
        uint8_t  reserved[5];
} __attribute__((packed)) hb_mc_response_packet_t;


typedef union packet {
	hb_mc_request_packet_t request; /**/
	hb_mc_response_packet_t response; /* from the Hammerblade Manycore */
	uint32_t words[4];
} hb_mc_packet_t;
        
/* 
 * hb_mc_fifo_tx_t identifies the type of a packet transmit operation. MIN & MAX
 * are not fifo directions, they are used for iteration over fifos. 
 */
typedef enum __hb_mc_fifo_tx_t {  /*  */
	HB_MC_FIFO_TX_REQ = HB_MC_MMIO_FIFO_TO_DEVICE,
	HB_MC_FIFO_TX_RSP = HB_MC_MMIO_FIFO_TO_HOST
} hb_mc_fifo_tx_t;

typedef enum __hb_mc_config_id_t {
	HB_MC_CONFIG_VERSION = 0,
	HB_MC_CONFIG_COMPLIATION_DATE = 1,
	HB_MC_CONFIG_NETWORK_ADDR_WIDTH = 2,
	HB_MC_CONFIG_NETWORK_DATA_WIDTH = 3,
	HB_MC_CONFIG_DEVICE_DIM_X = 4,
	HB_MC_CONFIG_DEVICE_DIM_Y = 5,
	HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X = 6,
	HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y = 7,
	HB_MC_CONFIG_NOT_IMPLEMENTED = 8,
	HB_MC_CONFIG_REPO_STL_HASH = 9,
	HB_MC_CONFIG_REPO_MANYCORE_HASH = 10,
	HB_MC_CONFIG_REPO_F1_HASH = 11,
	HB_MC_CONFIG_MAX = 12
} hb_mc_config_id_t;
        
inline hb_mc_direction_t hb_mc_get_rx_direction(hb_mc_fifo_rx_t d){
	return (hb_mc_direction_t)d;
}

inline hb_mc_direction_t hb_mc_get_tx_direction(hb_mc_fifo_tx_t d){
	return (hb_mc_direction_t)d;
}

int hb_mc_fifo_transmit (uint8_t fd, hb_mc_fifo_tx_t type, const hb_mc_packet_t *packet);
int hb_mc_fifo_receive (uint8_t fd, hb_mc_fifo_rx_t type, hb_mc_packet_t *packet);

int hb_mc_get_host_credits (uint8_t fd);
int hb_mc_all_host_req_complete(uint8_t fd);

int hb_mc_get_config(uint8_t fd, hb_mc_config_id_t id, uint32_t *cfg);
uint8_t hb_mc_get_manycore_dimension_x();
uint8_t hb_mc_get_manycore_dimension_y(); 

int hb_mc_fifo_finish(uint8_t fd);
int hb_mc_fifo_init(uint8_t *fd);
int hb_mc_fifo_check(uint8_t fd);

int hb_mc_get_recv_vacancy (uint8_t fd);

int hb_mc_tile_set_origin_registers(uint8_t fd, uint8_t x, uint8_t y, uint8_t origin_x, uint8_t origin_y);
int hb_mc_tile_set_origin_symbols(uint8_t fd, eva_id_t eva_id, char* elf, uint8_t x, uint8_t y, uint32_t origin_x, uint32_t origin_y);
int hb_mc_tile_set_coord_symbols(uint8_t fd, eva_id_t eva_id, char* elf, uint8_t x, uint8_t y, uint32_t cord_x, uint32_t cord_y);
int hb_mc_tile_set_id_symbol(uint8_t fd, eva_id_t eva_id, char* elf, uint8_t x, uint8_t y, uint32_t cord_x, uint32_t cord_y, uint32_t dim_x, uint32_t dim_y);

static uint8_t num_dev = 0;
static char *ocl_table[8] = {(char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0};

/**
 * Get the X coordinate of the requester from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the requester for packet
 */
static inline uint8_t hb_mc_request_packet_get_x_dst(const hb_mc_request_packet_t *packet)
{
        return packet->x_dst;
}

/**
 * Get the Y coordinate of the requester from a request packet
 * @param[in] packet a request packet
 * @return the Y coordinate of the requester for packet
 */
static inline uint8_t hb_mc_request_packet_get_y_dst(const hb_mc_request_packet_t *packet)
{
        return packet->y_dst;
}

/**
 * Get the X coordinate of the responder from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the responder for packet
 */
static inline uint8_t hb_mc_request_packet_get_x_src(const hb_mc_request_packet_t *packet)
{
        return packet->x_src;
}

/**
 * Get the X coordinate of the responder from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the responder for packet
 */
static inline uint8_t hb_mc_request_packet_get_y_src(const hb_mc_request_packet_t *packet)
{
        return packet->y_src;
}

/**
 * Get the extend opcode of a request packet
 * @param[in] packet a request packet
 * @return the extended opcode of packet
 */
static inline uint8_t hb_mc_request_packet_get_mask(const hb_mc_request_packet_t *packet)
{
        return packet->mask;
}

/**
 * Get the opcode of a request packet
 * @param[in] packet a request packet
 * @return the opcode of packet
 */
static inline uint8_t hb_mc_request_packet_get_op(const hb_mc_request_packet_t *packet)
{
        return packet->op;
}

/**
 * Get the address field of a request packet
 * @param[in] packet a request packet
 * @return the address field of packet
 */
static inline uint32_t hb_mc_request_packet_get_addr(const hb_mc_request_packet_t *packet)
{
        return le32toh(packet->addr);
}

/**
 * Get the data field of a request packet
 * @param[in] packet a request packet
 * @return the data field of packet
 */
static inline uint32_t hb_mc_request_packet_get_data(const hb_mc_request_packet_t *packet)
{
        return le32toh(packet->data); 
}

/**
 * Get the valid data of a request packet
 * @param[in] packet a request packet
 * @return the valid data field of packet
 */
static inline uint32_t hb_mc_request_packet_get_data_valid(const hb_mc_request_packet_t *packet)
{
	uint32_t valid = 0;
	for (int i = 0; i < 4; i++) { /* TODO: hardcoded */		
		if (hb_mc_get_bits(packet->mask, i, 1) == 1)
			valid |=  hb_mc_get_bits(packet->data, i*8, 8);
	}
        return le32toh(valid); 
	
}
/**
 * Set the X coordinate of the requester in a request packet
 * @param[in] packet a request packet
 * @param[in] x an X coordinate
 */
static inline void hb_mc_request_packet_set_x_dst(hb_mc_request_packet_t *packet, uint8_t x)
{
        packet->x_dst = x;
}

/**
 * Set the Y coordinate of the requester in a request packet
 * @param[in] packet a request packet
 * @param[in] y an Y coordinate
 */
static inline void hb_mc_request_packet_set_y_dst(hb_mc_request_packet_t *packet, uint8_t y)
{
        packet->y_dst = y;
}

/**
 * Set the X coordinate of the responder in a request packet
 * @param[in] packet a request packet
 * @param[in] x an X coordinate
 */
static inline void hb_mc_request_packet_set_x_src(hb_mc_request_packet_t *packet, uint8_t x)
{
        packet->x_src = x;
}

/**
 * Set the Y coordinate of the responder in a request packet
 * @param[in] packet a request packet
 * @param[in] y a Y coordinate
 */
static inline void hb_mc_request_packet_set_y_src(hb_mc_request_packet_t *packet, uint8_t y)
{
        packet->y_src = y;
}

/**
 * Set the data mask in a request packet
 * @param[in] packet a request packet
 * @param[in] mask a byte-mask value
 */
static inline void hb_mc_request_packet_set_mask(hb_mc_request_packet_t *packet, hb_mc_packet_mask_t mask)
{
        packet->mask = mask;
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] op an opcode
 */
static inline void hb_mc_request_packet_set_op(hb_mc_request_packet_t *packet, hb_mc_packet_op_t op)
{
        packet->op = op;
}

/**
 * Set the addess in a request packet
 * @param[in] packet a request packet
 * @param[in] addr a valid manycore end point address (EPA)
 */
static inline void hb_mc_request_packet_set_addr(hb_mc_request_packet_t *packet, uint32_t addr)
{
        packet->addr = htole32(addr);
}

/**
 * Set the data in a request packet
 * @param[in] packet a request packet
 * @param[in] data packet data
 */
static inline void hb_mc_request_packet_set_data(hb_mc_request_packet_t *packet, uint32_t data)
{
        packet->data = htole32(data); // TODO: byte mask?
}

/**
 * Checks if 2 request packets are the same.
 * @param[in] a a request packet
 * @param[in] b a request packet
 * @return HB_MC_SUCCESS if packets match and HB_MC_FAIL if packets do not match. In order to match, all of the non-data fields of a an b must be the same and the valid data must be the same. 
 */
static int hb_mc_request_packet_equals(const hb_mc_request_packet_t *a, const hb_mc_request_packet_t *b) {
	if (!a || !b) {
		return HB_MC_FAIL;
	}
	else if (hb_mc_request_packet_get_x_dst(a) != hb_mc_request_packet_get_x_dst(b)) {
		return HB_MC_FAIL;	
	}
	else if (hb_mc_request_packet_get_y_dst(a) != hb_mc_request_packet_get_y_dst(b)) {
		return HB_MC_FAIL;	
	}
	else if (hb_mc_request_packet_get_x_src(a) != hb_mc_request_packet_get_x_src(b)) {
		return HB_MC_FAIL;	
	}
	else if (hb_mc_request_packet_get_y_src(a) != hb_mc_request_packet_get_y_src(b)) {
		return HB_MC_FAIL;	
	}
	else if (hb_mc_request_packet_get_data_valid(a) != hb_mc_request_packet_get_data_valid(b)) {
		return HB_MC_FAIL;	
	}
	else if (hb_mc_request_packet_get_addr(a) != hb_mc_request_packet_get_addr(b)) {
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

/*
 * Formats a Manycore request packet.
 * @param packet packet struct that this function will populate. caller must allocate. 
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * assumes all fields are <= 32
 * */
static inline int hb_mc_format_request_packet(uint8_t fd, hb_mc_request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, hb_mc_packet_op_t opcode) {
        uint32_t intf_coord_x, intf_coord_y;
        
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &intf_coord_x) != HB_MC_SUCCESS ||
           hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &intf_coord_y) != HB_MC_SUCCESS)
                return HB_MC_FAIL;
        
	hb_mc_request_packet_set_x_dst(packet, x);
	hb_mc_request_packet_set_y_dst(packet, y);
	hb_mc_request_packet_set_x_src(packet, (uint8_t)intf_coord_x);
	hb_mc_request_packet_set_y_src(packet, (uint8_t)intf_coord_y);
	hb_mc_request_packet_set_data(packet, data);
	hb_mc_request_packet_set_mask(packet, HB_MC_PACKET_REQUEST_MASK_WORD);
	hb_mc_request_packet_set_op(packet, opcode);
	hb_mc_request_packet_set_addr(packet, addr);
}


/**
 * Get the X coordinate of the requester from a response packet
 * @param[in] packet a response packet
 * @return the X coordinate of the requester for packet
 */
static inline uint8_t hb_mc_response_packet_get_x_dst(const hb_mc_response_packet_t *packet)
{
        return packet->x_dst;
}

/**
 * Get the Y coordinate of the requester from a response packet
 * @param[in] packet a response packet
 * @return the Y coordinate of the requester for packet
 */
static inline uint8_t hb_mc_response_packet_get_y_dst(const hb_mc_response_packet_t *packet)
{
        return packet->y_dst;
}

/**
 * Get the load ID of the requester from a response packet
 * @param[in] packet a response packet
 * @return the load ID of the packet
 */
static inline uint32_t hb_mc_response_packet_get_load_id(const hb_mc_response_packet_t *packet)
{
        return packet->load_id;
}

/**
 * Get the payload data of the requester from a response packet
 * @param[in] packet a response packet
 * @return the packet's payload data
 */
static inline uint32_t hb_mc_response_packet_get_data(const hb_mc_response_packet_t *packet)
{
        return packet->data;
}

/**
 * Get the opcode of the requester from a response packet
 * @param[in] packet a response packet
 * @return the packet's opcode
 */
static inline uint8_t hb_mc_response_packet_get_op(const hb_mc_response_packet_t *packet)
{
        return packet->op;
}

/**
 * Set the x destination of the response packet
 * @param packet a response packet
 * @param x x destination
 */
static inline void hb_mc_response_packet_set_x_dst(hb_mc_response_packet_t *packet, uint8_t x)
{
        packet->x_dst = x;
}

/**
 * Set the y destination of the response packet
 * @param packet a response packet
 * @param y y destination
 */
static inline void hb_mc_response_packet_set_y_dst(hb_mc_response_packet_t *packet, uint8_t y)
{
        packet->y_dst = y;
}

/**
 * Set the data of the response packet
 * @param packet a response packet
 * @param data the data to send
 */
static inline void hb_mc_response_packet_set_data(hb_mc_response_packet_t *packet, uint32_t data)
{
        packet->data = data;
}

/**
 * Set the opcode of the response packet
 * @param packet a response packet
 * @param op the opcode to set
 */
static inline void hb_mc_response_packet_set_op(hb_mc_response_packet_t *packet, uint8_t op)
{
        packet->op = op;
}
        
/*
 * Formats a Manycore request packet.
 * @param packet packet struct that this function will populate. caller must allocate. 
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param data packet's data
 * @param opcode operation type (e.g load, store, etc.)
 * assumes all fields are <= 32
 * */
static inline void hb_mc_format_response_packet(hb_mc_response_packet_t *packet, uint8_t x, uint8_t y, uint32_t data, hb_mc_packet_op_t opcode) {
	hb_mc_response_packet_set_x_dst(packet, x);
	hb_mc_response_packet_set_y_dst(packet, y);
	hb_mc_response_packet_set_data(packet, data);
	hb_mc_response_packet_set_op(packet, opcode);
}
        

#ifdef __cplusplus
}
#endif
#endif
