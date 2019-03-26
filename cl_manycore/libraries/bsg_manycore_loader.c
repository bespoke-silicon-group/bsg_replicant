#ifndef COSIM
    #include <bsg_manycore_driver.h> /* TODO: should be angle brackets */ 
    #include <bsg_manycore_loader.h>
    #include <bsg_manycore_errno.h>
#else
    #include <utils/sh_dpi_tasks.h>
    #include "bsg_manycore_driver.h"
    #include "bsg_manycore_loader.h"
    #include "bsg_manycore_errno.h"
#endif

uint32_t DMEM_BASE = 0x1000;

/*!
 *  * writes the binary's instructions into (x,y)'s icache.
 *   * */
static int hb_mc_load_packets(uint8_t fd, uint8_t **pkts, uint32_t num_pkts) {

    if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
        printf("load_packets(): warning - device was never initialized.\n");
        return HB_MC_FAIL;
    }
    
    int status = HB_MC_SUCCESS;
    for (int i = 0; i < num_pkts; i++) {
        if (hb_mc_write_fifo(fd, 0, (uint32_t *) pkts[i]) != HB_MC_SUCCESS) {
            status = HB_MC_FAIL;
            break;
        }
    }
    if (status != HB_MC_SUCCESS)
        printf("load_packets(): load failed.\n");
    return status;
}

/*!
 * Sets a selected number of bytes of a Manycore packet to a desired value.
 * @param packet an array of bytes that form the Manycore packet.
 * @param byte_start the byte offset within the packet where the field starts.
 * @param size the size in bytes of the field.
 * @param val the value to set the selected bytes to.
 * */
static void hb_mc_set_field (uint8_t *packet, uint8_t byte_start, uint8_t size, uint32_t val) {
    if (size == WORD) {
        uint32_t *field = (uint32_t *) (packet + byte_start);
        *field = val;
    }
    else if (size == SHORT) {
        uint16_t *field = (uint16_t *) (packet + byte_start);
        *field = val;
    }
    else {
        uint8_t *field = (uint8_t *) (packet + byte_start);
        *field = val;
    }
}

/*!
 * Forms a Manycore packet.
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * @return array of bytes that form the Manycore packet.
 * assumes all fields are <= 32
 * */
uint8_t *hb_mc_get_pkt(uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode) {
    
    uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t));

    uint32_t byte_start = 0;

    hb_mc_set_field(packet, byte_start, X_BYTE, x); 
    byte_start += X_BYTE;

    hb_mc_set_field(packet, byte_start, Y_BYTE, y);
    byte_start += Y_BYTE;

    hb_mc_set_field(packet, byte_start, X_BYTE, MY_X);
    byte_start += X_BYTE;

    hb_mc_set_field(packet, byte_start, Y_BYTE, MY_Y);
    byte_start += Y_BYTE;

    hb_mc_set_field(packet, byte_start, DATA_BYTE, data);
    byte_start += DATA_BYTE;
    
    hb_mc_set_field(packet, byte_start, OP_EX_BYTE, 0xF);
    byte_start += OP_EX_BYTE;

    hb_mc_set_field(packet, byte_start, OP_BYTE, opcode);
    byte_start += OP_BYTE;
    
    hb_mc_set_field(packet, byte_start, ADDR_BYTE, addr);
    byte_start += ADDR_BYTE;

    return packet;
}

/*! 
 * Creates arrays of Manycore packets that contain the text and data segments of the binary. These arrays are saved in the global variables text_pkts and data_pkts.
 * @param filename the path to the binary.
 * */
static void hb_mc_parse_elf (char *filename, uint8_t x, uint8_t y, uint32_t *num_instr, uint32_t *data_size, uint8_t ***icache_pkts, uint8_t ***dram_pkts, uint8_t ***dmem_pkts, bool init_dram) {  
    int fd = open(filename, O_RDONLY);
    struct stat s;
    assert(fd != -1);
    if (fstat(fd, &s) < 0)
        abort();
    size_t size = s.st_size;

    uint8_t* buf = (uint8_t*) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(buf != MAP_FAILED);
    close(fd);

    assert(size >= sizeof(Elf64_Ehdr));
    
    Elf32_Ehdr* eh = (Elf32_Ehdr *) buf;                         
    Elf32_Phdr* ph = (Elf32_Phdr *) (buf + eh->e_phoff); 
    assert(size >= eh->e_phoff + eh->e_phnum*sizeof(*ph)); 
    
    *num_instr = 1 * (ph[TEXT].p_memsz / 4);
    if (init_dram)
        *dram_pkts = (uint8_t **) calloc(*num_instr, sizeof(uint8_t *));
    *icache_pkts = (uint8_t **) calloc(*num_instr, sizeof(uint8_t *));

    *data_size = (ph[DATA].p_memsz / 4); 
    *dmem_pkts = (uint8_t **) calloc(*data_size, sizeof(uint8_t *));
    
    for (unsigned i = 0; i < eh->e_phnum; i++) { 
        if(ph[i].p_type == PT_LOAD && ph[i].p_memsz) { 
            if (i == TEXT) {
                uint8_t *instructions = (uint8_t *) calloc(ph[i].p_memsz, sizeof(uint8_t));
                if (ph[i].p_filesz) { 
                        assert(size >= ph[i].p_offset + ph[i].p_filesz);  
                    for (int byte = 0; byte < ph[i].p_filesz; byte++)
                        instructions[byte] = buf[ph[i].p_offset + byte];
                }           
                for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
                    int32_t addr = (ofs) >> 2; 
                    uint32_t data = *((uint32_t *) (instructions + ofs));
                    if (init_dram) {
                        (*dram_pkts)[ofs/4] = hb_mc_get_pkt(addr, data, 0, NUM_Y+1, OP_REMOTE_STORE);
                    }
                    (*icache_pkts)[ofs/4] = hb_mc_get_pkt(addr | (1 << 22), data, x, y, OP_REMOTE_STORE); /*  send packet to tile (0, 0) */
                }
            }
            
            else if (i == DATA) { /* load to tile (0, 0) */
                uint8_t *data_dmem = (uint8_t *) calloc(ph[i].p_memsz, sizeof(uint8_t));
                if (ph[i].p_filesz) { 
                        assert(size >= ph[i].p_offset + ph[i].p_filesz);  
                    for (int byte = 0; byte < ph[i].p_filesz; byte++)
                        data_dmem[byte] = buf[ph[i].p_offset + byte];
                }       
                for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
                    uint32_t addr = (4096 + ofs) >> 2;
                    uint32_t data = *((uint32_t *) (data_dmem + ofs));
                    (*dmem_pkts)[ofs/4] = hb_mc_get_pkt(addr, data, x, y, OP_REMOTE_STORE);
                }
            }
        }
    }
    munmap(buf, size);
}

void hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size) {
    if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
        printf("hb_mc_load_binary(): warning - device was not initialized.\n");
        return;
    }
    
    if (!size)
        return; 
    
    for (int i = 0; i < size; i++) {
        uint8_t **icache_pkts, **dram_pkts, **dmem_pkts;
        uint32_t num_instr, data_size;
        bool init_dram = (i == 0) ? true : false;
        hb_mc_parse_elf(filename, x[i], y[i], &num_instr, &data_size, &icache_pkts, &dram_pkts, &dmem_pkts, init_dram);
        printf("Loading icache of tile (%d, %d)\n", x[i], y[i]);
        hb_mc_load_packets(fd, icache_pkts, num_instr);
        if (init_dram) {
            printf("Loading dram.\n");
            hb_mc_load_packets(fd, dram_pkts, num_instr);
        }
        printf("Loading dmem of tile (%d, %d)\n", x[i], y[i]);
        hb_mc_load_packets(fd, dmem_pkts, data_size);
    }
}

/*!
 * Returns an array of Manycore packets that should be used to freeze the needed tiles. Triggers a soft reset on the tile at (X, Y)
 * @return array of Manycore packets.
 * */

static uint8_t *hb_mc_get_freeze_pkt (uint8_t x, uint8_t y) {
    uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
    packet = hb_mc_get_pkt((1 << (EPA_BYTE_ADDR_WIDTH-3)), 1, x, y, OP_REMOTE_STORE);
    return packet;
}

/*!
 *  * freezes (x,y).
 *   * */
void hb_mc_freeze (uint8_t fd, uint8_t x, uint8_t y) {
    if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
        printf("freeze(): warning - device was not initialized.\n");
        return;
    }
        
    printf("Freezing tile (%d, %d).\n", x, y);
    uint8_t *freeze_pkt = hb_mc_get_freeze_pkt(x, y); 
    bool pass_freeze = true;
    if (hb_mc_write_fifo(fd, 0, (int *) freeze_pkt) != HB_MC_SUCCESS) {
        pass_freeze = false;
    }
    if (pass_freeze)
        printf("freeze finished.\n");
    else
        printf("freeze failed.\n"); 
}

/*!
* Returns an array of Manycore packets that should be used to unfreeze the needed tiles. Currently, on the tile at (X, Y) = (0, 0) is unfrozen.
* @return array of Manycore packets.
* */

static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y) {
    uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
    packet = hb_mc_get_pkt((1 << (EPA_BYTE_ADDR_WIDTH-3)), 0, x, y, OP_REMOTE_STORE);
    return packet;
}

/*!
*  * unfreezes (x,y).
 *   * */
void hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y) {
    if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
        printf("unfreeze(): warning - device was not initialized.\n");
        return;
    }
        
    printf("Unfreezing tile (%d, %d).\n", x, y);
    uint8_t *unfreeze_pkt = hb_mc_get_unfreeze_pkt(x, y); 
    int pass_unfreeze = HB_MC_SUCCESS;
    if (hb_mc_write_fifo(fd, 0, (int *) unfreeze_pkt) != HB_MC_SUCCESS) {
        pass_unfreeze = HB_MC_FAIL;
    }
    if (pass_unfreeze == HB_MC_SUCCESS)
        printf("unfreeze finished.\n");
    else
        printf("unfreeze failed.\n");   
}

/*!
 * Returns an array of Manycore configuration packets that should be used to set the tiles' group X coordinate.
 * @return array of Manycore packets.
 * */
static uint8_t *hb_mc_get_tile_group_origin_X_pkt (uint8_t x, uint8_t y, uint8_t x_cord) {
    uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
    packet = hb_mc_get_pkt((1 << (EPA_BYTE_ADDR_WIDTH-3)) + CSR_TGO_X, x_cord, x, y, OP_REMOTE_STORE);
    return packet;
}

/*!
 * Returns an array of Manycore configuration packets that should be used to set the tiles' group Y coordinate.
 * @return array of Manycore packets.
 * */
static uint8_t *hb_mc_get_tile_group_origin_Y_pkt (uint8_t x, uint8_t y, uint8_t y_cord) {
    uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
    packet = hb_mc_get_pkt((1 << (EPA_BYTE_ADDR_WIDTH-3)) + CSR_TGO_Y, y_cord, x, y, OP_REMOTE_STORE);
    return packet;
}

/*!
 * Set tile group coordinate (x_cord, y_cord) to tile (x, y)
 * */
void hb_mc_set_tile_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t x_cord, uint8_t y_cord) {
    if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
        printf("unfreeze(): warning - device was not initialized.\n");
        return;
    }
        
    printf("Set tile (%d, %d) with group origin (%d, %d).\n", x, y, x_cord, y_cord);
    uint8_t *tile_group_origin_X_pkt = hb_mc_get_tile_group_origin_X_pkt(x, y, x_cord); 
    uint8_t *tile_group_origin_Y_pkt = hb_mc_get_tile_group_origin_Y_pkt(x, y, y_cord); 
    // set X cord 
    if (hb_mc_write_fifo(fd, 0, (int *) tile_group_origin_X_pkt) != HB_MC_SUCCESS) {
        printf("set tile group origin X failed.\n");
        return;
    }
    printf("set tile group origin X finished.\n");
    // set Y cord   
    if (hb_mc_write_fifo(fd, 0, (int *) tile_group_origin_Y_pkt) != HB_MC_SUCCESS) {
        printf("set tile group origin Y failed.\n");    
        return;
    }
    printf("set tile group origin Y finished.\n");
}

/*!
 * Returns an array of tag empty packets that should be used to flush the fifo in the mancycore_link_to_cache.
 * @return array of tag init package.
 * */
static uint8_t *hb_mc_get_tag_pkt (uint8_t x, uint8_t y) {
    uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
    packet = hb_mc_get_pkt((1 << (EPA_TAG_ADDR_WIDTH-3)), 0, x, y, OP_REMOTE_STORE);
    return packet;
}

/*!
 * Store cache tag 4 times, to initalize the fifo value in the manycore_link_to_cache.
 *
 * */
void hb_mc_init_cache_tag(uint8_t fd, uint8_t x, uint8_t y) {

    if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
        printf("init cache tag(): warning - device was not initialized.\n");
        return;
    }
        
    printf("init cache (%d, %d)'s tag.\n", x, y);
    uint8_t *tag_pkt = hb_mc_get_tag_pkt(x, y); 
    int  pass_init_tag = HB_MC_SUCCESS;
    for (int i=0; i<4; i++) {
        sv_pause(1);
        if (hb_mc_write_fifo(fd, 0, (int *) tag_pkt) != HB_MC_SUCCESS) {    
        printf("fail %d\n", i);
        pass_init_tag= HB_MC_FAIL;
        }
    }
    if (pass_init_tag == HB_MC_SUCCESS)
        printf("init tag finished.\n");
    else
        printf("init tag failed.\n");
}
