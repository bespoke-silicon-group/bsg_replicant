#ifndef DEVICE_H
#define DEVICE_H
 
uint32_t axi4_size = 0;
uint32_t axi4_align = 128; 
const uint64_t CROSSBAR_M0 = 0x00000000;
const uint64_t CROSSBAR_M0_1 = 0x00000300;
const uint64_t CROSSBAR_M0_2 = 0x00000600;
const uint64_t CROSSBAR_M0_3 = 0x00000900;
const uint64_t CROSSBAR_M1 = 0x00001000;
const uint64_t CFG_REG = 0;
const uint64_t CNTL_REG = 0x8; 
const uint64_t WR_ADDR_LOW = 0x20;
const uint64_t WR_ADDR_HIGH = 0x24;
const uint64_t WR_HEAD = 0x28;
const uint64_t WR_LEN = 0x2c;
const uint64_t WR_BUF_SIZE = 0x30;
const uint64_t PKT_NUM = 0x60;
// modes
const uint32_t STOP = 0;
const uint32_t WR = 1;
const uint32_t RD = 2;
// hello world test
const uint64_t HELLO_WORLD = 0x500;

// fifo register
const uint64_t FIFO_IST = 0x00;
const uint64_t FIFO_IER = 0x04;
const uint64_t FIFO_TDFV = 0x0C;
const uint64_t FIFO_TDFD = 0x10;
const uint64_t FIFO_TLR = 0x14;
const uint64_t FIFO_RDFO = 0x1C;
const uint64_t FIFO_RDFD = 0x20;
const uint64_t FIFO_RLR = 0x24;

#endif
